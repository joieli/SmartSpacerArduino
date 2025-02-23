//Adafruit Bluefruit LE nRF51 https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
#include <SD.h>
#include <Arduino.h>
#include <SPI.h>
#include <ArduinoJson.h> //ArduinoJson by Benoit Blanchon, documentation: https://arduinojson.org/v7/
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
String configFileName = "CONFIG.TXT";
String jsonDataBT = "";

bool endOfFile = false;
bool hasConfig = false;

class Config{
  public:
    String userID;
    String spacerUDI;
    bool PEFRActive;
    int baseline;

    void print(){
      Serial.println(F("{"));
      Serial.print(F("  userID: "));
      Serial.println(userID);
      Serial.print(F("  spacerUDI: "));
      Serial.println(spacerUDI);
      Serial.print(F("  PEFRActive: "));
      Serial.println(PEFRActive);
      Serial.print(F("  baseline: "));
      Serial.println(baseline);
      Serial.println(F("}"));
    }
};
Config config; //initalizing a config object

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

  //setting up the config data
  Serial.println();
  if (SD.exists(configFileName)) {
    Serial.println(F("CONFIG.TXT exists, printing contents..."));

    File configFile = SD.open(configFileName);
    if (!configFile) {
      Serial.println(F("Failed to open CONFIG.TXT"));
    }
    
    // Read the JSON data from the file
    String jsonData = "";
    while (configFile.available()) {
      jsonData += (char)configFile.read();
    }
    configFile.close();

    // Create a JSON document to parse the data
    StaticJsonDocument<200> doc;
    
    // Deserialize the JSON data
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.f_str());
      return;
    }

    config.userID = doc["userID"].as<String>();
    config.spacerUDI = doc["spacerUDI"].as<String>();
    config.PEFRActive = doc["PEFRActive"].as<bool>();
    config.baseline = doc["baseline"].as<int>();
    config.print();

    hasConfig = true;
  }

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

    if(ble.isConnected()){
      Serial.println(F("BLUETOOTH CONNECTED"));
      if(endOfFile==false){
        Serial.println(F("SENDING FOLLOWING CONTENTS OF FOLLOWING FILES: "));
        sendContentsBT(fileName);
        endOfFile = true;
      }
    }  
  }

  if(ble.isConnected()){
    Serial.println(F("BLUETOOTH CONNECTED"));
    receiveContentsBT();
  }
  delay(500);

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
          ble.print(buffer);
          buffer = "";
        }
      }
    }
    ble.print(F("***End of File: "));
    ble.print(fileName);
    ble.println(F("***"));
    file.close();
    Serial.print(F("  "));
    Serial.println(fileName);
  }
  else{
    Serial.print(F("    error opening "));
    Serial.println(fileName);
  }
}

void receiveContentsBT(){
  while(ble.available()){
    char character = (char)ble.read();
    jsonDataBT += character;
    if(character == 125) {
      Serial.println(F("Recieved following JSON from BT:"));
      Serial.println(jsonDataBT);
      Serial.println();
      // Create a JSON document to parse the data
      StaticJsonDocument<200> doc;
      
      // Deserialize the JSON data
      DeserializationError error = deserializeJson(doc, jsonDataBT);
      if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.f_str());
        return;
      }

      if(config.spacerUDI == doc["spacerUDI"].as<String>()){
        config.userID = doc["userID"].as<String>();
        config.PEFRActive = doc["PEFRActive"].as<bool>();
        config.baseline = doc["baseline"].as<int>();

        rewriteConfigFile(configFileName, config);
      }
      jsonDataBT = ""; //reset jsonData
    }
  }
}

void rewriteConfigFile(String filename, Config config){
  JsonDocument doc;
  JsonObject root= doc.to<JsonObject>();

  // Set the values in the document
  root["userID"] = config.userID;
  root["spacerUDI"] = config.spacerUDI;
  root["PEFRActive"] = config.PEFRActive;
  root["baseline"] = config.baseline;
  
  //remove exisitng config file to rewrite
  SD.remove(filename);
  
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.print(F("Failed to create file: "));
    Serial.println(filename);
    return;
  }

  // Serialize JSON to file
  if (serializeJsonPretty(root, file) == 0) {
    Serial.print(F("  Failed to write to "));
    Serial.println(filename);
  }
  else{
    Serial.print(F("   Updated "));
    Serial.println(filename);
  }
  file.println();

  // Close the file
  file.close();
}

void printContents(String fileName){
  File file = SD.open(fileName);
  if (file) {
    Serial.print(F("BEGINNING OF "));
    Serial.print(fileName);
    Serial.println(F("========================="));
    while (file.available()) {
      Serial.write(file.read());
    }
  file.close();
  Serial.println(F("END OF DOCUMENT========================="));
  }
  else{
    Serial.print(F("    error opening: "));
    Serial.println(fileName);
  }
}

