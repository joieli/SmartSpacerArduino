#define NUM_LEDS     8

// duration to pause
int delayTime = 1000;

const int dataPin = A1; //shiftRegister pin 14 (red wire)
const int enablePin = A2; //shiftRegister pin 13 (orange wire)
const int clockPin = A4; //shiftRegister pin 12 (blue wire)
const int latchPin = A3; //shiftRegister pin 11(purple wire)

void setup ()
{
  // Setup pins as Outputs
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  digitalWrite(enablePin, HIGH);
}
 
void loop() {
  // Count from 0 to 255 and display in binary
  digitalWrite(enablePin, LOW);
  digitalWrite(latchPin, LOW);

  // Shift out the bits
  shiftOut(dataPin, clockPin, MSBFIRST, 0x00);

  // ST_CP HIGH change LEDs
  digitalWrite(latchPin, HIGH);
  delay(1000);

  for (int numberToDisplay = 0; numberToDisplay < 256; numberToDisplay++) {
    
    // ST_CP LOW to keep LEDs from changing while reading serial data
    digitalWrite(enablePin, LOW);
    digitalWrite(latchPin, LOW);
 
    // Shift out the bits
    shiftOut(dataPin, clockPin, MSBFIRST, numberToDisplay);
 
    // ST_CP HIGH change LEDs
    digitalWrite(latchPin, HIGH);
 
    delay(500);
  }
}