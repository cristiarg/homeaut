#define SERIAL_DEBUG_INPUT
#define SERIAL_DEBUG_TIMER
 
const byte remctrl_portD2 = 2; // remote button C
const byte remctrl_portD3 = 3; // remote button A
const byte remctrl_portD4 = 4; // remote button D
const byte remctrl_portD5 = 5; // remote button B
 
///////////////////////////////////////////////////////////////////////////////////////////////////
// the actual work to be done
//

// Boolean to represent toggle state
volatile bool toggleState = false;

const byte ledPin = 13; // TODO: delete, only for testing

void work_toggle()
{
  // Change state of toggle
  toggleState = !toggleState;
  // Indicate state on LED
  digitalWrite(ledPin, toggleState);
}

void work_start()
{
  toggleState = true;
  digitalWrite(ledPin, toggleState);
}

void work_stop()
{
  toggleState = false;
  digitalWrite(ledPin, toggleState);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// mechanism to force a non-zero interval between any commands
// handled from the input remote control
//

// volatile because it is accessed from an interrupt handler
volatile unsigned long lastToggleTime = 0;
const unsigned long DEBOUNCE_DELAY = 100;

// update when the most recent command has been validly handled
void last_command_update(unsigned long ms_now)
{
  lastToggleTime = ms_now;
}

// introduce a minimum "debounce" delay between commands to reduce possible noice
// NOTE: I think this is related to how the button levels of the remote translate
// to input pin levels; in practice this works well enough and avoids "noise" (ie: very
// quick on-offs of the output which could cause mechanical failures in the output relays)
bool last_command_can_handle(unsigned long millis_now)
{
  return (lastToggleTime == 0)
      || ((millis_now - lastToggleTime) > DEBOUNCE_DELAY)
      || ((long)(millis_now - lastToggleTime) < 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// use timer 1 to force a long delay for a proper timeout when
// the relays are stopped
//

#if defined(SERIAL_DEBUG_TIMER)
volatile unsigned long timer1_start_millis = 0;
#endif
volatile int timer1_iteration = 0;
const int TIMER1_TIMEOUT_MULTIPLIER_FACTOR = 10; // * 500 ms = 14500 ms

// programm timer 1 to fire every 500 ms using comp register A
void timer1_setup_start_compA_500ms()
{
#if defined(SERIAL_DEBUG_TIMER)
  Serial.println("T1:start");
  timer1_start_millis = millis();
#endif

  timer1_iteration = 0;

  noInterrupts(); // disable all interrupts
  TCCR1A = 0; // reset timer1 control register A
  TCCR1B = 0; // reset timer1 control register B
  // Input:
  //  - Uno's oscillator freq 16MHz
  //  - prescaler (divider) factor /256
  // Output:
  //  - ==> 62500 Hz (triggers per second, based on divider)
  //  - therefore, to trigger every half second, 31250 is the value needed in the comparer
  TCCR1B |= B00000100; // prescaler factor /256
  OCR1A = 31250; // timer 1 output compare register A
  TIMSK1 |= (1 << OCIE1A); // enable interrupt use of timer 1 compare register A (value: B00000010)
  TCNT1 = 0; // when timer is started, it starts counting from whatever value is in the counter
            // so, sto start from a consistent value, reset everytime
  interrupts(); // re-enable all interrupts
}

void timer1_stop_and_reset()
{
#if defined(SERIAL_DEBUG_TIMER)
  Serial.println("T1:reset");
#endif

  timer1_iteration = 0;

  noInterrupts(); // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 &= ~(1 << OCIE1A); // turn off the specific timer interupt that we enabled
  TCNT1 = 0; // important for a clean slate; which, in turn, results in accurate firings of the handler
  interrupts(); // re-enable all interrupts
}

// timer 1 ISR for compare register A
ISR(TIMER1_COMPA_vect)
{
#if 1
  timer1_iteration += 1;
  if (timer1_iteration >= TIMER1_TIMEOUT_MULTIPLIER_FACTOR)
  {
    timer1_stop_and_reset();
    work_stop();

#if defined(SERIAL_DEBUG_TIMER)
    Serial.print("T1:stop:");
    Serial.println(millis() - timer1_start_millis);
#endif

    timer1_iteration = 0;
  }
  else
  {
    // advance the compare register with half a second
    OCR1A += 31250;
    // or advance the counter

  }
#else
# if defined(SERIAL_DEBUG_TIMER)
  Serial.print("T1:stop:");
  Serial.println(millis() - timer1_start_millis);
# endif
  timer1_stop_and_reset();
  work_stop();
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Interrupt Service Request for port D pins
//

volatile bool stillHandling= false;

// interruput service request for port D (pins D0 - D7)
ISR(PCINT2_vect) {
  if (stillHandling) {
    return;
  }
  stillHandling = true;

  const unsigned long millis_now = millis();

  //if (lastToggleTime == 0 || ((millis_now - lastToggleTime) > DEBOUNCE_DELAY) || ((long)(millis_now - lastToggleTime) < 0))
  if (last_command_can_handle(millis_now))
  {
    // check which button was pressed
    bool d2 = false;
    if (digitalRead(remctrl_portD2) == HIGH) {
      d2 = true;
#if defined(SERIAL_DEBUG_INPUT)
      Serial.println("d2-C");
#endif
    }

    bool d3 = false;
    if (!d2 && digitalRead(remctrl_portD3) == HIGH) {
      d3 = true;
#if defined(SERIAL_DEBUG_INPUT)
      Serial.println("d3-A");
#endif
    }

    bool d4 = false;
    if (!(d2 || d3) && digitalRead(remctrl_portD4) == HIGH) {
      d4 = true;
#if defined(SERIAL_DEBUG_INPUT)
      Serial.println("d4-D");
#endif
    }

    bool d5 = false;
    if (!(d2 || d3 || d4) && digitalRead(remctrl_portD5) == HIGH) {
      d5 = true;
#if defined(SERIAL_DEBUG_INPUT)
      Serial.println("d5-B");
#endif
    }

    if (d2 || d3 || d4 || d5) {
      work_start();
      timer1_setup_start_compA_500ms();
    }
    last_command_update(millis_now);
  }

  stillHandling = false;
}

void setup_remctrl_input()
{
  // Set switch pin as INPUT with pullup
#if 0
  pinMode(remctrl_portD2, INPUT_PULLUP);
  pinMode(remctrl_portD3, INPUT_PULLUP);
  pinMode(remctrl_portD4, INPUT_PULLUP);
  pinMode(remctrl_portD5, INPUT_PULLUP);
#else
  pinMode(remctrl_portD2, INPUT);
  pinMode(remctrl_portD3, INPUT);
  pinMode(remctrl_portD4, INPUT);
  pinMode(remctrl_portD5, INPUT);
#endif

  // working mode: pin change interrupts
  //
  // enable port D (pins D0..D7)
  PCICR |= B00000100;
  // from port D enable pins: 2 (PCINT18), 3(PCINT19), 4 (PCINT20), 5 (PCINT21)
  PCMSK2 |= B00111100;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// main
//
 
void setup() {
  // Set LED pin as output
  pinMode(ledPin, OUTPUT); // TODO: delete/change, only for testing

#if defined(SERIAL_DEBUG_INPUT) || defined(SERIAL_DEBUG_TIMER)
  Serial.begin(9600);
#endif

  setup_remctrl_input();
  // important to reset here so, upon first firing, the timer 1 values start from a controlled state
  timer1_stop_and_reset();

#if defined(SERIAL_DEBUG_INPUT) || defined(SERIAL_DEBUG_TIMER)
  Serial.println("init");
#endif
}
 
void loop() {
  // nop
}
