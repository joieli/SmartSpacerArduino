//Adafruit Bluefruit LE nRF51 https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
#include <SD.h>
#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
//hardware SPI setup (using SCK/MOSI/MISO)
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

const int chipSelect = 10;
File root;
String fileName;

bool endOfFile = false;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialise the  bluetooth module
  Serial.print(F("Initialising the Bluetooth:"));

  if ( !ble.begin(VERBOSE_MODE) ){
    error(F("Couldn't find Bluefruit"));
  }

  ble.echo(false); //diable echo frm bluetooth
  ble.sendCommandCheckOK("AT+HWModeLED=MODE"); //LED for bluetooth functionality
  Serial.println(F("Bluefruit info:"));
  ble.info();
  ble.verbose(false); //easier for debugging
  Serial.println( F("Switching to bluetooth to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed.");
    while (true);
  }
  Serial.println("initialization done.");

  Serial.println("LIST OF DOCUMENTS=========");

  root = SD.open("/");

  printDirectory(root, 0);
  root.close();

  Serial.println();
  Serial.println("Which file do you want to read?");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()){
    fileName = Serial.readStringUntil('\n');

    if(ble.isConnected() && endOfFile == false){
      Serial.println("SENDING FOLLOWING CONTENTS OF" + fileName + "================================");
      sendContentsBT(fileName); //ToDo change me
      Serial.println("END OF DOCUMENT=========================================");
      endOfFile = true;
    } 
  }


}

//FUNCTIONs=============================
//helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

//SD
void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print("  ");
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void sendContentsBT(String fileName)
{
  String buffer = "";
  
  File file = SD.open(fileName);
  int totalBytes = file.size();
  if (file) {
    ble.print(F("***Beginning of File: "));
    ble.print(fileName);
    ble.println(F("***"));
    while (file.available()) {
      for (int lastPos = 0; lastPos <= totalBytes; lastPos++){
        char character = (char)file.read();
        buffer += character;
        if(character==10){ //ASCII code for new line
          Serial.print(buffer);
          ble.print(buffer);
          buffer = "";
        }
      }
    }
    ble.print(F("***End of File: "));
    ble.print(fileName);
    ble.println(F("***"));
    file.close();
  }
  else{
    Serial.print(F("    error opening "));
    Serial.println(fileName);
  }
}


