#pragma once
#include <Arduino.h>

/**
 * "Static" class. Provides a, somewhat useful, interface to the TIMER1 capabilities.
 *
 * Allows to use:
 *  - usage of both comparers A and B, each one separately or simultaneously
 *  - explicit error return codes for some frequent mis-handlings
 *      - especially useful is the usage of incompatible prescale value for the A, B
 *        comparers which both share the same (single) prescaler
 *  - fire-once, fire-multiple and indefinite-firing callback handling
 *
 * Notes about the prescaler:
 *  - On a standard 16 MHz Arduino (Uno/Nano/etc.), Timer1 is 16-bit, so the longest delay
 *    you can get in normal mode is when the counter runs from 0 to 65535 and overflows. 
 *  - This formula gives the maximum possible interval of a prescaler divisor mode:
 *      ```
 *      Max interval (ms) = ((prescaler * 65536) / 16000000) * 1000
 *      ```
 *  - Maximum delays possible with each prescaler before the timer overflows:
 *      ------------------------------
 *      Prescaler  | Max delay (ms)
 *      -----------+------------------
 *             1   |      4.096
 *             8   |     32.768
 *            64   |    262.144
 *           256   |   1048.576
 *          1024   |   4194.304
 *      ------------------------------
 *
 * Nice to have (but which we don't have yet):
 *  - possibility to hand over user data with the callback
 *  - accept a pattern of delays in the form of an array and have the routine fired at
 *    those specified delays (maybe even in a round-robin fashion)
 *  - support also the Overflow Interrupot of TIMER1 (TOIE1)
 *      - https://wolles-elektronikkiste.de/en/interrupts-part-3-timer-interrupts -
 *        see "Using multiple Timer1 interruputs" where all 3 can be used
 *
 * Caveats:
 *  - only tested on an Arduino UNO board with an ATmega328P
 *  - the class is garnished with all nuts&bolts; if some features are not needed (eg.
 *      the callback function `isDone` argument; or the recurrence calls) these can be removed
 *
 * Resources:
 *  - https://wolles-elektronikkiste.de/en/timer-and-pwm-part-2-16-bit-timer1
 *  - https://wolles-elektronikkiste.de/en/interrupts-part-3-timer-interrupts
 */
class Timer1Helper {
public:
    /**
     * @param isDone - true when the callback is the last in the recurrence series; if the
     *      timer is configured for indefinite execution, the parameter will always be false
     */
    typedef void (*Callback)(bool isDone);

    /**
     * no error
     */
    static const int8_t ER_OK = 0;
    /**
     * The delay is too large for any prescaler of TIMER1
     */
    static const int8_t ER_PRESCALER_OUT_OF_BOUND = 1;
    /**
     * An already existing prescaler value is set and a new calculated one is
     * incompatible (both A and B comparers share the single prescaler of TIMER1).
     */
    static const int8_t ER_INCOMPATIBLE_A_B_PRESCALE = 2;
    /**
     * Recurrence value must be a strictly positive integer.
     */
    static const int8_t ER_INVALID_RECURRENCE_VALUE = 3;
    /**
     * A callback cannot be null
     */
    static const int8_t ER_NULL_CALLBACK = 4;

public:
    Timer1Helper()
    {
    }

    /**
     * Reset/stop any working interrupts on both comparers.
     *
     * It is needed to call this function if it is needed to switch from one prescaler
     * range to a different one. Example:
     *  - one (or both) comparer channel(s) have been used in the delay range (263ms, 1048ms)
     *  - now it is needed to use one (or both) with delays higher than 1048ms
     *  - call @reset() to allow such a change
     */
    static void reset() {
        cli(); // CLear Interrupts

        // only if this is reset, it is allowed to go from one prescaler value to a different one
        Timer1Helper::prescaler_bits = 0;

        TCCR1A = 0;
        TCCR1B = 0; // Stop timer

        // NOTE: in TIMSK1 register, bits will be cleared for A, B Output Compare
        // Matches (OCIE1A, OCIE1B) but not for Timer Overflow (TOIE1). Thus, if that
        // one is used independently, it will not be affected (but, I suppose there are other
        // 'strings attached' - as the shared prescaler and its value).
        resetA();
        resetB();

        TCNT1 = 0; // counter

        sei(); // SEt Interrupts
    }

    /**
     * Reset/stop TIMER1 comparer A interrupt only; leave everything else as is
     */
    static void resetA() {
        TIMSK1 &= ~(1 << OCIE1A);

        Timer1Helper::cbA = nullptr;
        Timer1Helper::cbA_recurrence = 0;
        Timer1Helper::cbA_compare_value = 0;

        OCR1A = 0;
        TIFR1 &= ~(1 << OCF1A);
    }

    /**
     * Reset/stop TIMER1 comparer B interrupt only ; leave everything else as is/was
     */
    static void resetB() {
        TIMSK1 &= ~(1 << OCIE1B);

        Timer1Helper::cbB = nullptr;
        Timer1Helper::cbB_recurrence = 0;
        Timer1Helper::cbB_compare_value = 0;

        OCR1B = 0;
        TIFR1 &= ~(1 << OCF1B);
    }

    /**
     * Schedule TIMER1 comparer A to fire @cb callback once at @delay_ms millis from `now()`.
     *
     * @return error code (see class static constants)
     *
     * @param delay_ms - delay in millis
     * @param cb - pointer to the callback function
     */
    static int8_t scheduleAOnce(uint32_t delay_ms, Callback cb) {
        return doScheduleA(delay_ms, 1, cb);
    }

    /**
     * Schedule TIMER1 comparer A to fire @cb callback a @recurrence number of times (equidistant
     * in time) with first firing at @delay_ms millis from `now()`.
     *
     * @return error code (see class static constants)
     *
     * @param delay_ms - delay in millis
     * @param recurrence - the number of times to call the @cb callback function before de-activating
     * @param cb - pointer to the callback function
     */
    static int8_t scheduleARecurrent(uint32_t delay_ms, int16_t recurrence, Callback cb) {
        if (recurrence <= 0) {
            return ER_INVALID_RECURRENCE_VALUE;
        }
        return doScheduleA(delay_ms, recurrence, cb);
    }

    /**
     * Schedule TIMER1 comparer B to fire @cb callback an indefinite number of times (equidistant
     * in time) with first firing at @delay_ms millis from `now()`.
     *
     * @return error code (see class static constants)
     *
     * @param delay_ms - delay in millis
     * @param cb - pointer to the callback function
     */
    static int8_t scheduleAIndefinitely(uint32_t delay_ms, Callback cb) {
        return doScheduleA(delay_ms, -1, cb);
    }

    /**
     * Schedule TIMER1 comparer B to fire @cb callback once at @delay_ms millis from `now()`.
     *
     * @return error code (see class static constants)
     *
     * @param delay_ms - delay in millis
     * @param cb - pointer to the callback function
     */
    static int8_t scheduleBOnce(uint32_t delay_ms, Callback cb) {
        return doScheduleB(delay_ms, 1, cb);
    }

    /**
     * Schedule TIMER1 comparer B to fire @cb callback a @recurrence number of times (equidistant
     * in time) with first firing at @delay_ms millis from `now()`.
     *
     * @return error code (see class static constants)
     *
     * @param delay_ms - delay in millis
     * @param recurrence - the number of times to call the @cb callback function before de-activating
     * @param cb - pointer to the callback function
     */
    static int8_t scheduleBRecurrent(uint32_t delay_ms, int16_t recurrence, Callback cb) {
        if (recurrence <= 0) {
            return ER_INVALID_RECURRENCE_VALUE;
        }
        return doScheduleB(delay_ms, recurrence, cb);
    }

    /**
     * Schedule TIMER1 comparer B to fire @cb callback an indefinite number of times (equidistant
     * in time) with first firing at @delay_ms millis from `now()`.
     *
     * @return error code (see class static constants)
     *
     * @param delay_ms - delay in millis
     * @param cb - pointer to the callback function
     */
    static int8_t scheduleBIndefinitely(uint32_t delay_ms, Callback cb) {
        return doScheduleB(delay_ms, -1, cb);
    }

private:
    static int8_t doScheduleA(uint32_t delay_ms, int16_t recurrence, Callback cb) {
        if (cb == NULL) {
            return ER_NULL_CALLBACK;
        }

        // save state and disable A comparer to eliminate the possibility of race conditions
        // while all configs are done (ie. A comparer settings are inconsistent)
        const uint8_t output_compare_set_before = TIMSK1 & (1 << OCIE1A);
        TIMSK1 &= ~(1 << OCIE1A);

        const int8_t err_code = setupCompare(delay_ms, A);
        if (err_code == ER_OK) {
            Timer1Helper::cbA_recurrence = recurrence;
            Timer1Helper::cbA = cb;

            TIMSK1 |= (1 << OCIE1A);
        } else {
            // if current request failed, leave as before
            TIMSK1 |= output_compare_set_before;
        }
        return err_code;
    }

    static int8_t doScheduleB(uint32_t delay_ms, int16_t recurrence, Callback cb) {
        if (cb == NULL) {
            return ER_NULL_CALLBACK;
        }

        // save state and disable B comparer to eliminate the possibility of race conditions
        // while all configs are done (ie. B comparer settings are inconsistent)
        const uint8_t output_compare_set_before = TIMSK1 & (1 << OCIE1B);
        TIMSK1 &= ~(1 << OCIE1B);

        const int8_t err_code = setupCompare(delay_ms, B);
        if (err_code == ER_OK) {
            Timer1Helper::cbB_recurrence = recurrence;
            Timer1Helper::cbB = cb;

            // all set successfully, only now do set the flag to activate output comparer B
            TIMSK1 |= (1 << OCIE1B);
        } else {
            // if current request failed, leave as before
            TIMSK1 |= output_compare_set_before;
        }
        return err_code;
    }

private:
    // which comparers to activate
    // TODO: also add overflow TO1E1, apparently all can work together - see
    // https://wolles-elektronikkiste.de/en/interrupts-part-3-timer-interrupts - Using multiple Timer1 interrupts
    enum Channel {
        A = 0,
        B = 1
    };

public: // statics
    /**
     * the prescaler is shared between the two comparers; this static is a
     * mechanism to ensure that the two delays are not imcompatible when different delay
     * intervals are requested
     */
    static uint8_t prescaler_bits;
    static volatile Callback cbA;
    /**
     * < 0 : infinite recurrence
     * == 0: invalid value; it is actually the idle value
     * > 0 : specifies the number of recurrences
     */
    static volatile int16_t cbA_recurrence;
    static volatile uint16_t cbA_compare_value;

    static volatile Callback cbB;
    static volatile int16_t cbB_recurrence;
    static volatile uint16_t cbB_compare_value;

private:
    static int8_t setupCompare(uint32_t delay_ms, Channel ch) {
        // Convert ms to timer ticks and choose prescaler
        const uint32_t ticks = (F_CPU / 1000UL) * delay_ms; // raw ticks - the range in ticks that we need to have in the prescaler to be able to accomodate the required delay
        uint8_t prescaleBits = 0;
        uint16_t compareValue = 0;

        if (choosePrescaler(ticks, prescaleBits, &compareValue)) {
            if (Timer1Helper::prescaler_bits != 0 && Timer1Helper::prescaler_bits != prescaleBits) {
                return ER_INCOMPATIBLE_A_B_PRESCALE;
            }
            // TODO: for now, the prescaler is only reset with the reset function; it would be
            // possible to implement a RAII-type usage of the prescaler (this would be useful with the
            // fire-once working method) but first need to find out about //#include <util/atomic.h>
            Timer1Helper::prescaler_bits = prescaleBits;

            const uint16_t now = TCNT1;
            const uint16_t target = now + compareValue;
            if (ch == A) {
                Timer1Helper::cbA_compare_value = compareValue;
                OCR1A = target;
                // output comparer A is only activated after all relevant configs are done
                // in order to limit race conditions as much as possible
                //TIMSK1 |= (1 << OCIE1A);
            } else {
                Timer1Helper::cbB_compare_value = compareValue;
                OCR1B = target;
                // output comparer B is only activated after all relevant configs are done
                // in order to limit race conditions as much as possible
                //TIMSK1 |= (1 << OCIE1B);
            }
            // set value for single prescaler shared between A and B comparers
            TCCR1B |= prescaleBits;
            // NOTE: TCCR1B |= (1 << WGM12) sets CTC mode (Clear Timer on Compare Mode 4)
            // which results in the timer counter only counting up to OCR1A and then
            // is deactivated - https://wolles-elektronikkiste.de/wp-content/uploads/2020/01/WGM1.png ;
            // anyway, the result is that only the A routine would be fired and not both

            return ER_OK;
        } else {
            // Too long delay, cap it or ignore
            return ER_PRESCALER_OUT_OF_BOUND;
        }
    }

    static bool choosePrescaler(uint32_t ticks, uint8_t &bits, uint16_t *compare) {
        static const struct {
          uint8_t bits;
          uint16_t div;
        } prescale_arr[] = {
          {1, 1},
          {2, 8},
          {3, 64},
          {4, 256},
          {5, 1024}
        };
        for (auto &prescale : prescale_arr) {
            const uint32_t t = ticks / prescale.div;
            if (t <= 65535) { // TIMER1 is 16 bits
                bits = prescale.bits;
                *compare = (uint16_t)t;
                return true;
            }
        }
        return false;
    }
};

// static members
uint8_t Timer1Helper::prescaler_bits = 0;

volatile Timer1Helper::Callback Timer1Helper::cbA = nullptr;
volatile int16_t Timer1Helper::cbA_recurrence = 0;
volatile uint16_t Timer1Helper::cbA_compare_value = 0;

volatile Timer1Helper::Callback Timer1Helper::cbB = nullptr;
volatile int16_t Timer1Helper::cbB_recurrence = 0;
volatile uint16_t Timer1Helper::cbB_compare_value = 0;

// ISRs
ISR(TIMER1_COMPA_vect) {
    // NOTE: any of the static values corresponding to a comparer can be used
    // as a flag/marker (callback, recurrence value) and we chose it to be the
    // recurrence value.
    if (/*Timer1Helper::cbA != NULL &&*/ Timer1Helper::cbA_recurrence != 0) {
        auto fn = Timer1Helper::cbA;

        if (Timer1Helper::cbA_recurrence > 0) {
            Timer1Helper::cbA_recurrence -= 1;
        }

        if (Timer1Helper::cbA_recurrence == 0) {
            TIMSK1 &= ~(1 << OCIE1A);
            Timer1Helper::cbA = nullptr;
            Timer1Helper::cbA_compare_value = 0;

            fn(true);
        } else {
            // adjust Output Compare Register A for correct next firing according to calculated delay
            // (makes heavy use of math overflow of the 16 bit register)
            const uint16_t now = TCNT1;
            const uint16_t target = now + Timer1Helper::cbA_compare_value;
            OCR1A = target;

            fn(false);
        }
    } else {
        // (should be) unreachable
    }
}

ISR(TIMER1_COMPB_vect) {
    if (/*Timer1Helper::cbB != NULL &&*/ Timer1Helper::cbB_recurrence != 0) {
        auto fn = Timer1Helper::cbB;

        if (Timer1Helper::cbB_recurrence > 0) {
            Timer1Helper::cbB_recurrence -= 1;
        }

        if (Timer1Helper::cbB_recurrence == 0) {
            TIMSK1 &= ~(1 << OCIE1B);
            Timer1Helper::cbB = nullptr;
            Timer1Helper::cbB_compare_value = 0;

            fn(true);
        } else {
            // adjust Output Compare Register B for correct next firing according to calculated delay
            // (makes heavy use of math overflow of the 16 bit register)
            const uint16_t now = TCNT1;
            const uint16_t target = now + Timer1Helper::cbB_compare_value;
            OCR1B = target;

            fn(false);
        }

    } else {
        // (should be) unreachable
    }
}
