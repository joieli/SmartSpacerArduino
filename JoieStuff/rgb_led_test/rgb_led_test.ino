const int dataPin = A1; //shiftRegister pin 14 (red wire)
const int enablePin = A2; //shiftRegister pin 13 (orange wire)
const int latchPin = A3; //shiftRegister pin 11(purple wire)
const int clockPin = A4; //shiftRegister pin 12 (blue wire)

uint16_t prevState = 0b0000000000000000;


// Number of LEDs (RGB LEDs)
int numLEDs = 4;  // Example: 3 RGB LEDs controlled by 3 shift registers

void setup() {
  // Set pins as OUTPUT
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(enablePin, OUTPUT);

  allLEDsOff();
  digitalWrite(enablePin, HIGH);

}

void loop() {
  setColor("yellow", 0);
  delay(1000);
  setColor("green", 1);
  delay(1000);
  setColor("red", 2);
  delay(1000);
  setColor("blue", 3);
  delay(1000);
  setColor("off", 0);
  delay(1000);
  setColor("off", 1);
  delay(1000);
  setColor("off", 2);
  delay(1000);
  setColor("off", 3);
  delay(1000);
}

// Color mapping function
void setColor(String color, int ledIndex) {
  byte red = 0, green = 0, blue = 0;
  
  Serial.print(F("led"));
  Serial.print(ledIndex);
  Serial.print(F("  "));
  
  Serial.print(color);
  Serial.print(F("  "));

  // Map color name to RGB values
  if (color == "red") {
    red = 1;
    green = 0;
    blue = 0;
  } else if (color == "green") {
    red = 0;
    green = 1;
    blue = 0;
  } else if (color == "blue") {
    red = 0;
    green = 0;
    blue = 1;
  } else if (color == "yellow") {
    red = 1;
    green = 1;
    blue = 0;
  } else if (color == "off") {
    red = 0;
    green = 0;
    blue = 0;
  }

  // Send color data to the correct LED
  setLEDColor(ledIndex, red, green, blue, prevState);
  Serial.println(prevState, BIN);
}

// Function to send the color to a specific LED
void setLEDColor(int ledIndex, byte red, byte green, byte blue, uint16_t &prevState) {
  uint16_t newState;
  if(red == 1){
    newState = bitSet(prevState, ledIndex*3);
  }
  else if(red == 0){
    newState = bitClear(prevState, ledIndex*3);
  }

  if(green == 1){
    newState = bitSet(prevState, ledIndex*3+2);
  }
  else if(green == 0){
    newState = bitClear(prevState, ledIndex*3+2);
  }

  if(blue == 1){
    newState = bitSet(prevState, ledIndex*3+1);
  }
  else if(blue == 0){
    newState = bitClear(prevState, ledIndex*3+1);
  }


  digitalWrite(enablePin, LOW);

  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, newState >> 8);
  shiftOut(dataPin, clockPin, MSBFIRST, newState);
  digitalWrite(latchPin, HIGH);

  prevState = newState;
}

void allLEDsOff(){
  for (int i=0; i<numLEDs; i++){
    setColor("off", i);
  }
  digitalWrite(enablePin, HIGH);
}


