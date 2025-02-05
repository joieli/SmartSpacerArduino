const int led = 13;
const int redoPin = 12;
const int nextPin = 11;
boolean lastNextState = LOW;
boolean lastRedoState = LOW;
boolean ledOn = false;
const int debounceDelay = 50;

void setup() {
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);
  pinMode(nextPin, INPUT);
  pinMode(redoPin, INPUT);
}

void loop() {
  bool nextPressed = debounce(nextPin, lastNextState);
  bool redoPressed = debounce(redoPin, lastRedoState);
  // put your main code here, to run repeatedly:
  if (nextPressed==true || redoPressed==true){
    ledOn = !ledOn;
  }

  digitalWrite(led, ledOn);
}

boolean debounce(int buttonPin, bool &last){
  boolean current = digitalRead(buttonPin);
  if(last != current){
    delay(debounceDelay);
    current = digitalRead(buttonPin);
  }

  if (last == false && current == true){
    last = current;
    return true;
  }
  last = current;
  return false;
}
