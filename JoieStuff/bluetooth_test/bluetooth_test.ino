#include <SD.h> //SD by Arduino, SparkFun, documentation: https://docs.arduino.cc/libraries/sd/
#include <SPI.h>
#include <ArduinoJson.h> //ArduinoJson by Benoit Blanchon, documentation: https://arduinojson.org/v7/

#include "Adafruit_BLE.h" //Adafruit Bluefruit LE nRF51 https://github.com/adafruit/Adafruit_BluefruitLE_nRF51
#include <Arduino.h>
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
//hardware SPI setup (using SCK/MOSI/MISO)
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

bool hasFilesToSend = true;
const int batteryPin = A7;
int prevBattery = 0;

unsigned long lastFileSend = 0; //NEWWWW


//Exiting stuff
const int chipSelect = 10;
File root;
String fileName;
String configFileName = "CONFIG.TXT";
String jsonDataBT = "";

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

//SETUP--------------------------------------------------------
void setup() {
  Serial.begin(115200);
  //while (!Serial);

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
  else{
    Serial.println(F("CONFIG.TXT does not exist"));
    config.spacerUDI = "SPACE166";
  }

  Serial.println("LIST OF DOCUMENTS=========");
  root = SD.open("/");

  printDirectory(root, 0);
  root.close();

}

void loop() {
  // put your main code here, to run repeatedly:

  if(ble.isConnected()){
    //delay(5000); //ToDo: delay just so i can open up thing, remove later
    receiveContentsBT();
    sendBatteryBT();
    if(hasFilesToSend==true){
      Serial.println(F("BLUETOOTH CONNECTED"));
      /*
      sendSingleFile("E0000014.TXT"); //short exhalation file
      delay(1000);
      //sendSingleFile("E0000014.TXT"); //short exhalation file
      //sendSingleFile("M0000018.TXT"); //short medication file
      //sendSingleFile("M0000019.TXT"); //long medication file
      hasFilesToSend = false;
      */
      sendAndDeleteAllFilesBT();
    }
    else{
      Serial.println(F("BLUETOOTH CONNECTED - no files to send"));
    }
  }
  else{
    Serial.println("waiting for bluetooth connection...");
    delay(3000);
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
    if(fileName.startsWith("M00")){
      ble.println("#INHALATION[");
    }
    else if(fileName.startsWith("E00")){
      ble.println("#EXHALATION");
    }
    while (file.available()) {
      for (int lastPos = 0; lastPos <= totalBytes; lastPos++){
        if(ble.isConnected()){
          char character = (char)file.read();
          receiveContentsBT();
          loop_inner();
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
      if(fileName.startsWith("M00")){
        ble.print("]");
      }
      ble.println("@"); //flag for end file transfer
      file.close();

      //SD.remove(fileName); //remove file after sending
      Serial.println(F(" (done)"));
    }
    else{
      Serial.println();
      Serial.println(F("ERROR BLUETOOTH DISCONNECTED - CANCELING FILE SEND"));
      file.close();
      return;
    }
  }
  else{
    Serial.print(F("    Error opening:  "));
    Serial.println(fileName);
  }
}

void sendSingleFile(String targetFileName){
  Serial.println(F("SENDING CONTENTS AND DELETING FOLLOWING FILES: "));
  File dir = SD.open("/");

  while(ble.isConnected()) {
    File entry =  dir.openNextFile();
    if (! entry) {
      hasFilesToSend = false;
      break;
    }

    String fileName = entry.name();
    if (fileName == targetFileName){
      sendContentsBT(fileName);
      break;
    }
    entry.close();
  }
  dir.close();
}

void sendContentsBT(String fileName)
{
  String buffer = "";
  Serial.print(F("  Sending: "));
  Serial.print(fileName);

  File file = SD.open(fileName);
  int totalBytes = file.size();
  if (file) {
    if(fileName.startsWith("M00")){
      ble.println("#INHALATION["); //inhalation data sends as a list
    }
    else if(fileName.startsWith("E00")){
      ble.println("#EXHALATION");
    }
    while (file.available()) {
      for (int lastPos = 0; lastPos <= totalBytes; lastPos++){
        if(ble.isConnected()){
          char character = (char)file.read();
          receiveContentsBT();
          loop_inner();
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
      if(fileName.startsWith("M00")){
        ble.print("]"); //close off the list for inhalation data
      }
      ble.println("@"); //flag for end file transfer
      file.close();

      Serial.println(F("(done)"));
    }
    else{
      Serial.println();
      Serial.println(F("ERROR BLUETOOTH DISCONNECTED - CANCELING FILE SEND"));
      file.close();
      return;
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
    unsigned long currentTime = millis();
    if(currentTime - lastFileSend > 1000){
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
      lastFileSend = millis(); //update the file send time once you have fully sent the file
    }
    else{
      Serial.println("WAITTTTTTTT");
      loop_inner(); //if not time to send the file yet, just send the next file
    }
  }
  dir.close();
}

void receiveContentsBT(){
  while(ble.available()){
    char character = (char)ble.read();
    jsonDataBT += character;
    if(character == 125) { //125 = }
      //case where we receive a json
      Serial.println();
      Serial.println(F("Recieved following JSON from BT:"));
      Serial.println(jsonDataBT);
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
  hasConfig = true;
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
  float batteryVoltage = analogRead(batteryPin)*2*3.3/1024; //voltage divider divided by 2, multiple by 3.3 for ref voltage
  int batteryPercentage = map(batteryVoltage*1000, 3200, 4200, 0, 100); //map needs integer arguements
  
  if(prevBattery != batteryPercentage){
    unsigned long currentTime = millis();
    if(currentTime - lastFileSend>1000){
      Serial.print(F("Sending Battery: "));
      if(batteryPercentage > 100){
        Serial.println("Charging");
        ble.println(F("#BATTERY{"));
        ble.println(F("\t\"batteryPercentage\": \"Charging\""));
        ble.println(F("}@"));

        prevBattery = batteryPercentage;
        lastFileSend = currentTime;
      }
      else{
        Serial.println(batteryPercentage);
        ble.println(F("#BATTERY{"));
        ble.print("\t\"batteryPercentage\": \"");
        ble.print(batteryPercentage);
        ble.println(F("%\""));
        ble.println(F("}@"));

        prevBattery = batteryPercentage;
        lastFileSend = currentTime;
      }
    }
  }
}

void loop_inner(){
  Serial.println("INNNER================================================");
  delay(100);
}

