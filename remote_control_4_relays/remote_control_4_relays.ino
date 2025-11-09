/**
 * - take input from a YK04 remote control module which has 4 buttons
 * - map the actions to start/stop the relays on a 4 relay module (such as
 *    https://docs.arduino.cc/tutorials/4-relays-shield/4-relay-shield-basics/
 *    or some clone of it)
 * - the logic switches the relays in 2 pairs of 2 to control some external
 *    electrical motor with 2 rotation directions (it is intended to replace
 *    a 6-pin rocker switch that drives the motor in one direction of the other)
 * - basic logic based on a loop - no interrupts, no timers involved - which
 *    takes care of timeouts for level stabilization and make sure to never
 *    allow changes from one direction to the other because this causes short
 *    circuits (ie: high current way above the 10 amps that the relays are
 *    supposed to be able to handle)
 */
#include <avr/sleep.h>
#include <avr/power.h>

// define this if you need some debugging via serial port
//#define SERIAL_DEBUG

// output relays pins
const int relay1 = 8;
const int relay2 = 9;
const int relay3 = 10;
const int relay4 = 11;

// input from RF remote pins
const int inputB = 3; // D0 on the remote receiver
const int inputD = 4; // D1
const int inputA = 5; // D2
const int inputC = 6; // D3

// internal state machine:
//  - states (arbitrary values)
const int STATE_INVALID = 0;
const int STATE_ALL_OFF = 101;
const int STATE_B_UP = 303;
const int STATE_D_DOWN = 404;
//  - current state
volatile int state = STATE_INVALID;

const unsigned long delay_off = 500;
const unsigned long delay_on = 100;

// millis when the UP/DOWN state was set
unsigned long start_active_state_millis = 0;
const unsigned long ACTIVE_STATE_MAX_DURATION_MILLIS = 14500;

const unsigned long MAX_UNSIGNED_LONG = 4294967295;

void set_relay_ALL_off()
{
  digitalWrite(relay1, HIGH); // HIGH == OFF
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);
}

void set_relay_UP_on()
{
  digitalWrite(relay1, LOW);
  digitalWrite(relay2, LOW);
}

void set_relay_DOWN_on()
{
  digitalWrite(relay3, LOW);
  digitalWrite(relay4, LOW);
}

void stop_ALL_w_waits()
{
  set_relay_ALL_off();
  delay(delay_off); // leave more time for the levels to settle
}

void move_UP_w_waits()
{
  set_relay_ALL_off();
  // leave time for the relays to mechanically couple/coule
  delay(delay_off);
  set_relay_UP_on();
  delay(delay_on); // leave more time for the levels to settle
}

void move_DOWN_w_waits()
{
  set_relay_ALL_off();
  delay(delay_off); // leave more time for the levels to settle
  set_relay_DOWN_on();
  delay(delay_on); // leave more time for the levels to settle
}

void setup() {
  // try to disable some unused components to try and consume less energy
  // (not very helpful as long as we're running everything in a loop)
  //
  ADCSRA = 0; // disable ADC
  //power_all_disable (); // turn off various modules

  set_sleep_mode (SLEEP_MODE_IDLE);
  sleep_enable();
  sleep_cpu ();

#if defined(SERIAL_DEBUG)
  Serial.begin(9600);           //  setup serial
#endif

  pinMode(inputB, INPUT);
  pinMode(inputD, INPUT);
  pinMode(inputA, INPUT);
  pinMode(inputC, INPUT);
  
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  set_relay_ALL_off();
  // start with an invalid state to force the "state machine" to enter
  // a consistent state upon first loop execution
  state = STATE_INVALID;

#if defined(SERIAL_DEBUG)
  Serial.println("off-i");
#endif
}

void loop() {
  switch(state) {
    case STATE_ALL_OFF:
      // check the stop inputs first; for safety
      if (digitalRead(inputA) == HIGH || digitalRead(inputC) == HIGH)
      {
        state = STATE_ALL_OFF;
        stop_ALL_w_waits();

#if defined(SERIAL_DEBUG)
        Serial.println("iA_or_iC - OFF - double ckeck");
#endif
      }
      else if (digitalRead(inputD) == HIGH)
      {
        state = STATE_D_DOWN;
        move_DOWN_w_waits();
        start_active_state_millis = millis();

#if defined(SERIAL_DEBUG)
        Serial.println("iD - DOWN");
#endif
      }
      else if (digitalRead(inputB) == HIGH)
      {
        state = STATE_B_UP;
        move_UP_w_waits();
        start_active_state_millis = millis();

#if defined(SERIAL_DEBUG)
        Serial.println("iB - UP");
#endif
      }
      break;
    case STATE_B_UP:
    case STATE_D_DOWN:
      // check the stop inputs first; for safety
      if (digitalRead(inputA) == HIGH || digitalRead(inputC) == HIGH)
      {
        state = STATE_ALL_OFF;
        stop_ALL_w_waits();

#if defined(SERIAL_DEBUG)
        Serial.println("iA_or_iC - OFF");
#endif
      }
      else
      {
        const unsigned long now_millis = millis();
        if (now_millis < start_active_state_millis)
        {
          // overflow of millis (happens every 50 days or so according to docs)
          // immediately reset all to off
          state = STATE_ALL_OFF;
          stop_ALL_w_waits();

#if defined(SERIAL_DEBUG)
          Serial.println("OVERFLOW - OFF");
#endif
        }
        else if ((now_millis - start_active_state_millis) >= ACTIVE_STATE_MAX_DURATION_MILLIS)
        {
          state = STATE_ALL_OFF;
          stop_ALL_w_waits();
          // set to max to make sure that, should any inconsistency happen, the state will
          // be reset to all off upon the next timeout
          start_active_state_millis = MAX_UNSIGNED_LONG;

#if defined(SERIAL_DEBUG)
          Serial.println("timeout - OFF");
#endif
        }
      }
      break;
    default:
      state = STATE_ALL_OFF;
      stop_ALL_w_waits();

#if defined(SERIAL_DEBUG)
      Serial.println("ERR: UNK state -> ALL OFF");
#endif
  }

  // differentiated wait depending on new state
  switch(state) {
    case STATE_ALL_OFF:
      // wait more before picking up a new command
      delay(700);
      break;
    case STATE_B_UP:
    case STATE_D_DOWN:
      // be more swift in picking up a new command
      delay(100);
      break;
    default:
      delay(100);
      break;
  }
}

