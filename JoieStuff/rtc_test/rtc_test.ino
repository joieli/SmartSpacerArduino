// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

RTC_DS3231 rtc;
// Define the interrupt pin  (MUST BE 2 OR 3)
const int interruptPin = 2; 

//vars for millisecond tracking
volatile unsigned long lastMillis = 0;  // Time of the last interrupt in millis()
unsigned long nowMillis = 0;      // Milliseconds since the last interrupt (interrupts occur ever second)
String nowString;



void setup () {
  Serial.begin(9600);

#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  Serial.println("=========================START============================================");
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //sets the current time to the time in which the the code was compiled
  }

  // Configure the interrupt pin (pin 2 in this case)
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), updateLastMillis, FALLING);  // Trigger on falling edge

  // Configure RTC to trigger interrupt every 1 second
  rtc.writeSqwPinMode(DS3231_SquareWave1Hz);  // Set the square wave to 1Hz (1 second interval)
}

void loop () {
    DateTime now = rtc.now();
    unsigned long currentMillis = millis();
    nowMillis = currentMillis%1000;



    char buffer[] = "YYYY-MM-DD hh:mm:ss";
    //changing format to 000 for milliseconds
    if (nowMillis >= 10 && nowMillis < 100)
    {
      nowString = (String)now.toString(buffer) + ".0" + (String)nowMillis;
    }
    else if (nowMillis < 10)
    {
      nowString = (String)now.toString(buffer) + ".00" + (String)nowMillis;
    }
    else
    {
      nowString = (String)now.toString(buffer) + "." + (String)nowMillis;
    }


    Serial.println(nowString);
    Serial.println();

    delay(100);
}

// Interrupt Service Routine (ISR) to increment millisCounter
void updateLastMillis() {
  // Update lastInterruptMillis for the next interrupt
  lastMillis = millis();
}