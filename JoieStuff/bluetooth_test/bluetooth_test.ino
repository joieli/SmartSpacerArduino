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

bool hasFilesToSend = true;
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

}

void loop() {
  // put your main code here, to run repeatedly:

  if(ble.isConnected()){
    delay(3000); //ToDo: delay just so i can open up thing, remove later
    if(hasFilesToSend==true){
      Serial.println(F("BLUETOOTH CONNECTED"));
      sendAndDeleteAllFilesBT();
    }
    else{
      Serial.println(F("BLUETOOTH CONNECTED - no files to send"));
    }
    receiveContentsBT();
    sendBatteryBT();
  }
  else{
    Serial.println("waiting for bluetooth connection...");
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

void sendContentsAndRemoveBT(String fileName)
{
  String buffer = "";
  Serial.print(F("  Sending: "));
  Serial.print(fileName);

  File file = SD.open(fileName);
  int totalBytes = file.size();
  if (file) {
    ble.print(F("***Beginning of File: "));
    ble.print(fileName);
    ble.println(F("***"));
    while (file.available()) {
      for (int lastPos = 0; lastPos <= totalBytes; lastPos++){
        if(ble.isConnected()){
          char character = (char)file.read();
          receiveContentsBT();
          buffer += character;
          if(character==10){ //ASCII code for new line
            ble.print(buffer);
            buffer = "";
          }          
        }
        else{
          Serial.println();
          Serial.println(F("ERROR BLUETOOTH DISCONNECTED - CANCELING FILE SEND"));
          file.close();
          return;
        }
      }
    }

    if(ble.isConnected()){
      ble.print(F("***End of File: "));
      ble.print(fileName);
      ble.println(F("***"));
      file.close();

      SD.remove(fileName); //remove file after sending
      Serial.println(F(" (deleted)"));
    }
  }
  else{
    Serial.print(F("    Error opening:  "));
    Serial.println(fileName);
  }
}

void sendAndDeleteAllFilesBT(){
  Serial.println(F("SENDING CONTENTS AND DELETING FOLLOWING FILES: "));
  File dir = SD.open("/");

  while (ble.isConnected()) {
    File entry =  dir.openNextFile();
    if (! entry) {
      hasFilesToSend = false;
      break; // no more files
    }

    String fileName = entry.name();
    if (fileName != configFileName && (fileName.startsWith("M00") || fileName.startsWith("E00"))){
      sendContentsAndRemoveBT(fileName);
    }
    entry.close();
  }
  dir.close();
}

void receiveContentsBT(){
  while(ble.available()){
    char character = (char)ble.read();
    jsonDataBT += character;
    if(character == 125) {
      Serial.println();
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

int countFiles() {
  int count = 0;

  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory.");
    return 0;
  }

  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }

    // Only count files, ignore directories
    if (!entry.isDirectory()) {
      count++;
    }
    entry.close();
  }
  root.close();
  return count;
}

void sendBatteryBT(){

}

