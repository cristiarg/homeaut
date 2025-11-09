const int input4 = 4; // RF remote button -> digital input 4
const int input5 = 5; // RF remote button -> digital input 5
const int input6 = 6; // RF remote button -> digital input 6

const int relay1 = 8;
const int relay2 = 9;
const int relay3 = 10;
const int relay4 = 11;

void setup() {
  pinMode(input4, INPUT);
  pinMode(input5, INPUT);
  pinMode(input6, INPUT);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  digitalWrite(relay1, HIGH); // HIGH == OFF
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);
}

void loop() {
  if (digitalRead(input4) == HIGH) {
    // first off all outputs
    digitalWrite(relay1, HIGH);
    delay(25);
    digitalWrite(relay2, HIGH);
    delay(25);
    digitalWrite(relay3, HIGH);
    delay(25);
    digitalWrite(relay4, HIGH);
    delay(25);
    // TODO: maybe sleep here? to allow levels to settle?
    digitalWrite(relay1, LOW); // ON
    digitalWrite(relay2, LOW); // ON
  }
  else if (digitalRead(input5) == HIGH) {
    // first off all outputs
    digitalWrite(relay1, HIGH);
    delay(25);
    digitalWrite(relay2, HIGH);
    delay(25);
    digitalWrite(relay3, HIGH);
    delay(25);
    digitalWrite(relay4, HIGH);
    delay(25);
    // TODO: maybe sleep here? to allow levels to settle?
    digitalWrite(relay3, LOW);
    digitalWrite(relay4, LOW);
  }
  else if (digitalRead(input6) == HIGH) {
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
    digitalWrite(relay3, HIGH);
    digitalWrite(relay4, HIGH);
  }

  delay(100);
}
