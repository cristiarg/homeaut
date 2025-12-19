
#include <avr/sleep.h>
#include <avr/power.h>
#include "types.h"
#include "timer1_helper.h"

/**
 * these defines function as flags between a 'debug' and a 'release build
 */
//#define SERIAL_DEBUG_INPUT
//#define SERIAL_DEBUG_WORK
//#define SERIAL_DEBUG_WORK_LED
//#define SERIAL_DEBUG_MEASURE_DELAY

static Timer1Helper timer1;
 
///////////////////////////////////////////////////////////////////////////////////////////////////
// the actual work to be done
//

/**
 * the global work state; every action and transition revolves around this;
 * approx state machine:
 * 
 *  ┌──────────────────────────┬────────────────────────────┐
 *  │(error)                   │                     (error)│
 *  │             comm DOWN    ▼      comm UP               │
 *  │          ┌──────────────INVALID────────────────┐      │
 *  │          ▼               │                     ▼      │
 *  │    force relays          │             force relays   │
 *  │       to OFF             │                to OFF      │
 *  │          │               │                     │      │
 *  │          │(short wait)   │        (short delay)│      │
 *  │          ▼               │                     ▼      │
 *  └─────────DOWN             │                    UP──────┘
 *             │               │                     │       
 *             │(specific      │           (specific │       
 *             │  delay)       ▼             delay)  │       
 *             └────────────► IDLE ◄─────────────────┘       
 */
volatile work_state_t work_state = STATE_INVALID;

static const uint8_t PIN_OUT_DIG13_LED = 13; // TODO: delete, only for testing

// output relays pins
static const uint8_t PIN_OUT_DIG08_RELAY1 = 8;
static const uint8_t PIN_OUT_DIG09_RELAY2 = 9;
static const uint8_t PIN_OUT_DIG10_RELAY3 = 10;
static const uint8_t PIN_OUT_DIG11_RELAY4 = 11;

void set_relay_ALL_off()
{
  digitalWrite(PIN_OUT_DIG08_RELAY1, HIGH); // HIGH == OFF
  digitalWrite(PIN_OUT_DIG09_RELAY2, HIGH);
  digitalWrite(PIN_OUT_DIG10_RELAY3, HIGH);
  digitalWrite(PIN_OUT_DIG11_RELAY4, HIGH);
}

void set_relay_UP_on()
{
  digitalWrite(PIN_OUT_DIG08_RELAY1, LOW);
  digitalWrite(PIN_OUT_DIG09_RELAY2, LOW);
}

void set_relay_DOWN_on()
{
  digitalWrite(PIN_OUT_DIG10_RELAY3, LOW);
  digitalWrite(PIN_OUT_DIG11_RELAY4, LOW);
}


#if defined(SERIAL_DEBUG_WORK)
/*void work_print_state() {
    switch(work_state) {
      case STATE_INVALID: {
        break;
      }        
      case STATE_IDLE: {
        break;
      }        
      case STATE_UP:
      case STATE_DOWN: {
        break;
      }        
      case STATE_CHANGING: {
        Serial.println("-ch-");
        // previous command still executing
        // don't take another command
        break;
      }
      default: {
        break;
      }        
    }
}*/
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
// TIMER1 interrupts based state machine
//

/**
 * callback after commanding the relays to OFF to bring the state to IDLE
 *
 * This is to ensure a so-called debounce delay
 */
void _timer_cb_A_after_relays_off(bool is_done) {
  if (is_done) {
    timer1.resetA();
    work_state = STATE_IDLE;
#if defined(SERIAL_DEBUG_WORK)
    Serial.print("WK:IDLE:");
    Serial.println(millis());
#endif
  } else {
    //work_state = STATE_CHANGING;
  }
}

void work_emergency_all_off_and_reset() {
  set_relay_ALL_off();
  timer1.reset(); // clean before
  // invalid state because the only command considered in this state is "all relays off"
  work_state = STATE_INVALID;

#if defined(SERIAL_DEBUG_INPUT) || defined(SERIAL_DEBUG_WORK)
  Serial.println("WARN:E-R");
#endif
}

/**
 * command the relays to OFF and schedule timeout to set the state to IDLE
 */
void work_relays_off() {
  // set state, reset any executing timers, and set state again to avoid race conditions
  work_state = STATE_CHANGING;
  timer1.reset();
  work_state = STATE_CHANGING;
#if defined(SERIAL_DEBUG_WORK)
  Serial.print("WK:IDLE:ch:");
  Serial.println(millis());
#endif

  // stop relays and allow some timeout for actual switching before
  // setting the proper idle state to allow considering another command
#if defined(SERIAL_DEBUG_WORK_LED)
  digitalWrite(PIN_OUT_DIG13_LED, LOW);
#endif
  set_relay_ALL_off();

  const uint32_t delay_ms = 50;
  const int16_t recurrence = 8;
  const int8_t err_code = timer1.scheduleARecurrent(delay_ms, recurrence, &_timer_cb_A_after_relays_off);
  if (err_code != Timer1Helper::ER_OK) {
    work_emergency_all_off_and_reset();
#if defined(SERIAL_DEBUG_WORK)
    Serial.print("ERR:idle1:");
    Serial.println(err_code);
#endif
  }
}

#if defined(SERIAL_DEBUG_MEASURE_DELAY)
volatile unsigned long start_up;
#endif
/**
 * callback after commanding the relays to UP/DOWN, command to bring the state to IDLE by setting all relays to OFF
 */
void _timer_cb_B_after_schedule_up_or_down(bool is_done) {
  if (is_done) {
    timer1.resetB();

#if defined(SERIAL_DEBUG_MEASURE_DELAY)
    Serial.print("WK:temp:");
    Serial.println((unsigned long)(millis() - start_up));
#endif

    // TODO: after UP/DOWN make the transition to IDLE a bit longer - for extra safety
    work_relays_off();
  }
}

/**
 * callback to command the relays to UP
 */
void _timer_cb_A_after_relays_off_and_schedule_up(bool is_done) {
  if (is_done) {
    timer1.resetA();

    // set to final state to allow taking an emergency command to stop
    work_state = STATE_UP;
#if defined(SERIAL_DEBUG_WORK_LED)
    digitalWrite(PIN_OUT_DIG13_LED, HIGH);
#endif
    set_relay_UP_on();

#if defined(SERIAL_DEBUG_WORK)
    Serial.print("WK:UP:");
    Serial.println(millis());
#endif

    timer1.resetB();
#if defined(SERIAL_DEBUG_MEASURE_DELAY)
    start_up = millis();
#endif
    // actually, there is a bug in the Timer1Helper class - or in the way I understood
    // to implement the class in which, upon schduling the timer, the callback routine
    // is fired right away - even if I took all precautions to disable the respective
    // comparer of TIMER1; thus, the timeouts below account for this -1 recurrence
    const uint32_t delay_ms = 50;
    const int16_t recurrence = 290;
    const int8_t err_code = timer1.scheduleBRecurrent(delay_ms, recurrence, &_timer_cb_B_after_schedule_up_or_down);
    if (err_code != Timer1Helper::ER_OK) {
      work_emergency_all_off_and_reset();
#if defined(SERIAL_DEBUG_WORK)
      Serial.print("ERR:up2:");
      Serial.println(err_code);
#endif
    }
  }
}

/**
 * initiate the TIMER1 interrupts driven sequence to command the relays to UP then OFF:
 *  - first make sure all relays are OFF:
 *    - set all to OFF
 *    - and wait a little bit via a timer callback
 *  - then the main part:
 *    - set the relays for UP
 *    - and wait a specific time deduced from actual measurements of the pulley mechanism
 *  - after the specific time has passed, bring the state to IDLE, also via a timer callback
 */
void work_relays_up() {
  work_state = STATE_CHANGING;
#if defined(SERIAL_DEBUG_WORK)
  Serial.print("WK:UP:ch:");
  Serial.println(millis());
#endif

  // call first a routine to make sure all relays are in the off state
  // then issue the actual up command
#if defined(SERIAL_DEBUG_WORK_LED)
  digitalWrite(PIN_OUT_DIG13_LED, LOW);
#endif
  set_relay_ALL_off();

  timer1.resetA();
  const uint32_t delay_ms = 50;
  const int16_t recurrence = 8;
  const int8_t err_code = timer1.scheduleARecurrent(delay_ms, recurrence, &_timer_cb_A_after_relays_off_and_schedule_up);
  if (err_code != Timer1Helper::ER_OK) {
    work_emergency_all_off_and_reset();
#if defined(SERIAL_DEBUG_WORK)
    Serial.print("ERR:up1:");
    Serial.println(err_code);
#endif
  }
}

/**
 * callback to command the relays to DOWN
 */
void _timer_cb_A_after_relays_off_and_schedule_down(bool is_done) {
  if (is_done) {
    timer1.resetA();

    // set to final state to allow taking an emergency command to stop
    work_state = STATE_DOWN;
#if defined(SERIAL_DEBUG_WORK_LED)
    digitalWrite(PIN_OUT_DIG13_LED, HIGH);
#endif
    set_relay_DOWN_on();

#if defined(SERIAL_DEBUG_WORK)
    Serial.print("WK:DW:");
    Serial.println(millis());
#endif

    timer1.resetB();
#if defined(SERIAL_DEBUG_MEASURE_DELAY)
    start_up = millis();    
#endif
    const uint32_t delay_ms = 50;
    const int16_t recurrence = 290;
    const int8_t err_code = timer1.scheduleBRecurrent(delay_ms, recurrence, &_timer_cb_B_after_schedule_up_or_down);
    if (err_code != Timer1Helper::ER_OK) {
      work_emergency_all_off_and_reset();
#if defined(SERIAL_DEBUG_WORK)
      Serial.print("ERR:dw2:");
      Serial.println(err_code);
#endif
    }
  }
}

/**
 * initiate the TIMER1 interrupts driven sequence to command the relays to DOWN then OFF:
 *  - first make sure all relays are OFF:
 *    - set all to OFF
 *    - and wait a little bit via a timer callback
 *  - then the main part:
 *    - set the relays for DOWN
 *    - and wait a specific time deduced from actual measurements of the pulley mechanism
 *  - after the specific time has passed, bring the state to IDLE, also via a timer callback
 */
void work_relays_down() {
#if defined(SERIAL_DEBUG_WORK)
  Serial.print("WK:DW:ch:");
  Serial.println(millis());
#endif
  work_state = STATE_CHANGING;

  // call first a routine to make sure all relays are in the off state
  // then issue the actual down command
#if defined(SERIAL_DEBUG_WORK_LED)
  digitalWrite(PIN_OUT_DIG13_LED, LOW);
#endif
  set_relay_ALL_off();

  timer1.resetA();
  const uint32_t delay_ms = 50;
  const int16_t recurrence = 8;
  const int8_t err_code = timer1.scheduleARecurrent(delay_ms, recurrence, &_timer_cb_A_after_relays_off_and_schedule_down);
  if (err_code != Timer1Helper::ER_OK) {
    work_emergency_all_off_and_reset();
#if defined(SERIAL_DEBUG_WORK)
    Serial.print("ERR:dw1:");
    Serial.println(err_code);
#endif
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// mechanism to force a non-zero interval between any commands considered
// handled from the input remote control
//

volatile unsigned long lastToggleTime = 0; // volatile because it is accessed from an interrupt handler
const unsigned long DEBOUNCE_DELAY_MS = 200;

// update when the most recent command has been validly handled
void last_command_update(unsigned long ms_now) {
  lastToggleTime = ms_now;
}

/**
 * introduce a minimum "debounce" delay between commands to reduce possible noice
 * NOTE:
 *  - I think this is related to how the button levels of the remote translate
 *  to input pin levels; in practice this works well enough and avoids "noise" (ie: very
 *  quick on-offs of the output which could cause mechanical failures in the output relays)
 *  - This mechanism is additional to the STATE_CHANGING transient state during which the
 *  logic does "not" accept additional commands 
 */
bool last_command_can_handle(unsigned long millis_now) {
  return (lastToggleTime == 0)
      || ((millis_now - lastToggleTime) > DEBOUNCE_DELAY_MS)
      || ((long)(millis_now - lastToggleTime) < 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt Service Request for port D pins (input from remote module)
//

static const uint8_t PIN_IN_DIG02_REMOTE_C = 2; // remote button C - STOP
static const uint8_t PIN_IN_DIG03_REMOTE_A = 3; // remote button A - STOP
static const uint8_t PIN_IN_DIG04_REMOTE_D = 4; // remote button D - DOWN
static const uint8_t PIN_IN_DIG05_REMOTE_B = 5; // remote button B - UP

volatile remctrl_command_t remctrl_command = COMMAND_NA;

remctrl_command_t remctrl_get_command() {
  // for safety, check the stop buttons first
  if (digitalRead(PIN_IN_DIG02_REMOTE_C) == HIGH) {
#if defined(SERIAL_DEBUG_INPUT)
    // log: (port pin that sourced action)-(remote control button letter)-(consequential command)
    // these all have to correspond in order to not make any mistakes in wiring, logic, ..
    Serial.println("RM:d2-C-ST"); 
#endif
    return COMMAND_STOP;
  }

  if (digitalRead(PIN_IN_DIG03_REMOTE_A) == HIGH) {
#if defined(SERIAL_DEBUG_INPUT)
    Serial.println("RM:d3-A-ST");
#endif
    return COMMAND_STOP;
  }

  if (digitalRead(PIN_IN_DIG04_REMOTE_D) == HIGH) {
#if defined(SERIAL_DEBUG_INPUT)
    Serial.println("RM:d4-D-DW");
#endif
    return COMMAND_DOWN;
  }

  if (digitalRead(PIN_IN_DIG05_REMOTE_B) == HIGH) {
#if defined(SERIAL_DEBUG_INPUT)
    Serial.println("RM:d5-B-UP");
#endif
    return COMMAND_UP;
  }

  // TODO: when long press on a remote button, upon release we get here; why?
  return COMMAND_NA;
}

// interruput service request for port D (pins D0 - D7)
ISR(PCINT2_vect) {
  const unsigned long millis_now = millis();
  if (last_command_can_handle(millis_now))
  {
    const remctrl_command_t comm = remctrl_get_command();

    switch(work_state) {
      case STATE_INVALID: {
        // regardless of command, we stop
        work_relays_off();
        break;
      }        
      case STATE_IDLE: {
        if (comm == COMMAND_STOP) {
          work_relays_off();
        } else if (comm == COMMAND_UP) {
          work_relays_up();
        } else if (comm == COMMAND_DOWN) {
          work_relays_down();
        }
        break;
      }        
      case STATE_UP:
      case STATE_DOWN: {
        // any other command is intentionally ignored
        if (comm == COMMAND_STOP) {
          work_relays_off();
        }
        break;
      }        
      case STATE_CHANGING: {
#if defined(SERIAL_DEBUG_INPUT)
        Serial.println("IN:-ch-");
#endif
        // previous command still executing
        // don't take another command
        // TODO: what if we get into an inconsistent state and never exit from the changing state?
        break;
      }
      default: {
        // fail-safe
#if defined(SERIAL_DEBUG_INPUT)
        Serial.println("IN:-df-");
#endif
        work_relays_off();
        break;
      }        
    }
    // timer1_setup_start_compA_500ms();

    last_command_update(millis_now);
  }
}

void setup_remctrl_input() {
  // Set switch pin as INPUT with pullup
#if 0
  pinMode(PIN_IN_DIG02_REMOTE_C, INPUT_PULLUP);
  pinMode(PIN_IN_DIG03_REMOTE_A, INPUT_PULLUP);
  pinMode(PIN_IN_DIG04_REMOTE_D, INPUT_PULLUP);
  pinMode(PIN_IN_DIG05_REMOTE_B, INPUT_PULLUP);
#else
  pinMode(PIN_IN_DIG02_REMOTE_C, INPUT);
  pinMode(PIN_IN_DIG03_REMOTE_A, INPUT);
  pinMode(PIN_IN_DIG04_REMOTE_D, INPUT);
  pinMode(PIN_IN_DIG05_REMOTE_B, INPUT);
#endif

  // working mode: pin change interrupts
  //
  // enable port D (pins D0..D7)
  PCICR |= _BV(PCIE2); // B00000100
  // from port D enable pins: 2 (PCINT18), 3(PCINT19), 4 (PCINT20), 5 (PCINT21) - aka B00111100
  PCMSK2 |= _BV(PCINT18);
  PCMSK2 |= _BV(PCINT19);
  PCMSK2 |= _BV(PCINT20);
  PCMSK2 |= _BV(PCINT21);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// main
//
 
void setup() {
  //
  // minimal power optimizations - as per https://www.gammon.com.au/power
  //

  // disable ADC
  ADCSRA = 0;

  // set pins as output and low for minimal power consumption
  for (byte i = 0; i <= A5; ++i) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  // turn off various modules
  power_adc_disable();
  power_spi_disable();
#if (!defined(SERIAL_DEBUG_INPUT) && !defined(SERIAL_DEBUG_WORK) && !defined(SERIAL_DEBUG_MEASURE_DELAY))
  // when not using serial debug, the serial module ca
  power_usart0_disable();
#endif
  //power_timer0_disable(); // TIMER0 - needed for us, apparently
  //power_timer1_disable(); // TIMER1 - definitely needed (Timer1Helper is based on it)
  power_timer2_disable();
  power_twi_disable();
  // NOTE: You must use the PRR after setting ADCSRA to zero, otherwise the ADC is "frozen" in an active state.

  // SLEEP_MODE_PWR_DOWN - nothing works
  // SLEEP_MODE_STANDBY - nothing works
  // SLEEP_MODE_EXT_STANDBY - nothing works
  // SLEEP_MODE_PWR_SAVE - nothing works
  // SLEEP_MODE_ADC - nothing works
  set_sleep_mode(SLEEP_MODE_IDLE);
  cli(); // CLear Interrupts

  sleep_enable();

  // turn off brown-out enable in software
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS);

  sei(); // SEt Interrupts - guarantees that next instruction executed
  sleep_cpu(); // sleep within 3 clock cycles of above

  //
  // our own setup
  //

  pinMode(PIN_OUT_DIG13_LED, OUTPUT);
  digitalWrite(PIN_OUT_DIG13_LED, LOW);

  pinMode(PIN_OUT_DIG08_RELAY1, OUTPUT);
  pinMode(PIN_OUT_DIG09_RELAY2, OUTPUT);
  pinMode(PIN_OUT_DIG10_RELAY3, OUTPUT);
  pinMode(PIN_OUT_DIG11_RELAY4, OUTPUT);

#if defined(SERIAL_DEBUG_INPUT) || defined(SERIAL_DEBUG_WORK) || defined(SERIAL_DEBUG_MEASURE_DELAY)
  Serial.begin(9600);
#endif

  timer1.reset();

  setup_remctrl_input();

#if defined(SERIAL_DEBUG_INPUT) || defined(SERIAL_DEBUG_WORK) || defined(SERIAL_DEBUG_MEASURE_DELAY)
  Serial.println("init");
#endif

  // issue a safety command to off all relays and get to a consistent state
  work_relays_off();
}
 
void loop() {
  // nop
}
