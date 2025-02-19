//libraries
#include <Wire.h>
#include <PN532_I2C.h> //Install from: https://github.com/elechouse/PN532?tab=readme-ov-file
#include <PN532.h>
#include <NfcAdapter.h>
#include "RTClib.h" // RTCLib by Adafruit, documentation: https://github.com/adafruit/RTClib
#include <SPI.h>
#include <SD.h>  //SD by Arduino, SparkFun, documentation: https://docs.arduino.cc/libraries/sd/
#include <ArduinoJson.h> //ArduinoJson by Benoit Blanchon, documentation: https://arduinojson.org/v7/

/*
**Adjust the NDEF library**
You need to adjust the following line to make the code work because of conflicting definitions of NULL
Go into: c:\XXX\Arduino\libraries\NDEF\Ndef.h

replace:
#define NULL (void *)0

with:
#ifndef NULL
#define NULL (void *)0
#endif
*/

//pins etc.
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
RTC_PCF8523 rtc;
const int chipSelect = 10;
const int spacerPin = 6;
const int nextPin = 11;
const int redoPin = 12;

//classes and structs
class Inhaler {
  public:
    String type;
    String color;
    String commercialName;
    String medication;
    String startTime;

    void print() {
      Serial.println(F("Inhaler"));
      Serial.print(F("  type: "));
      Serial.println(type);
      Serial.print(F("  color: "));
      Serial.println(color);
      Serial.print(F("  commercialName: "));
      Serial.println(commercialName);
      Serial.print(F("  medication: "));
      Serial.println(medication);
      Serial.print(F("  startTime: "));
      Serial.println(startTime);
    }
};
Inhaler curInhaler; //intializing an inhaler object

class PeakFlowRateTest {
  public:
    int vals[4];
    int percents[4];
    int attemptNum[4];
    String timestamps[4];
    String startTime;

    void print() {
      Serial.println(F("PeakFlowTest"));
      Serial.print(F("  vals: ["));
      for (int i=0; i<4; i++){
        Serial.print(vals[i]+",");
      }
      Serial.println("]");
      
      Serial.print(F("  percents: ["));
      for (int i=0; i<4; i++){
        Serial.print(percents[i]+",");
      }
      Serial.println("]");
      
      Serial.print(F("  attemptNum: ["));
      for (int i=0; i<4; i++){
        Serial.print(attemptNum[i]+",");
      }
      Serial.println("]");
      Serial.print(F("  startTime: "));
      Serial.println(startTime);
    }
};
PeakFlowRateTest PEFRTest;

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

class Details{
  public:
    int trial;
    int attempt;
    int flowRate;
    String zone;
    int percentage;
    String timestamp;

    void print(){
      Serial.println(F("{"));
      Serial.print(F("  trial: "));
      Serial.println(trial);
      Serial.print(F("  attempt: "));
      Serial.println(attempt);
      Serial.print(F("  flowRate: "));
      Serial.println(flowRate);
      Serial.print(F("  zone: "));
      Serial.println(zone);
      Serial.print(F("  percentage: "));
      Serial.println(percentage);
      Serial.print(F("  timestamp: "));
      Serial.println(timestamp);
      Serial.println(F("}"));
    }
};

//mode vars
bool hasInhaler = false;
bool hasConfig = false;
bool spacerAttached = false;

int PEFRTrial = -1;
int PEFRAttempt = 1;
bool inPEFRMode = false;

bool nextPressed = false;
bool redoPressed = false;
bool nextButtonHeld = false;
bool nextWasHeld = false;
unsigned long nextLastPressTime = 0;
unsigned long redoLastPressTime = 0;
unsigned long nextButtonPressStartTime = 0;

//file vars
Sd2Card card;
SdVolume volume;
SdFile root;
String configFileName = "CONFIG.TXT";
String eventFileName = "UNKNOWN.TXT";
JsonDocument headerDoc;
JsonObject header;

//Misc vars
const int debounceDelay = 95; 
const unsigned long longPressThreshold = 3000;


//SETUP BLOCK==================================================================================================================
void setup(void) {
  Serial.begin(115200);

  while (!Serial); // wait for serial port to connect. Needed for native USB

  Serial.println("=========================START============================================");
  //Initializing RFID reader
  Serial.println("Searching for NDEF Reader ");
  nfc.begin();
  Serial.println();

  //Initializing RTC
  Serial.println("Search for RTC Module");
  if (! rtc.begin()) 
  {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  Serial.println("RTC Found");

  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); //sets the current time to the time in which the the code was compiled
  }
  Serial.println();

  //Initializing SD card
  Serial.println(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Initialization failed!"));
    while (true);
  }

  Serial.println(F("Initialization done. SD Card found."));
  card.init(SPI_HALF_SPEED, chipSelect);
  // print the type of card
  Serial.print(F("    Card type:         "));
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println(F("SD1"));
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println(F("SD2"));
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println(F("SDHC"));
      break;
    default:
      Serial.println(F("Unknown"));
  }

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
  else 
  {
    //ToDo: Get rid of this eventually when we have a way to recieve this from bluetooth, move to the loop block
    Serial.println(F("Creating CONFIG.TXT---"));
    config.userID = "123456789";
    config.spacerUDI = "SPACER1234567890";
    config.PEFRActive = true;
    config.baseline = 522;

    rewriteConfigFile(configFileName, config);
    hasConfig=true;
  }

  // initializing pins:
  pinMode(spacerPin, INPUT);
  pinMode(nextPin, INPUT_PULLUP);
  pinMode(redoPin, INPUT_PULLUP);

  // Attach interrupts to the buttons
  attachInterrupt(digitalPinToInterrupt(nextPin), nextButtonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(redoPin), redoButtonISR, FALLING);
}

//LOOP BLOCK====================================================================================================
void loop(void) {
  spacerAttached = digitalRead(spacerPin);
  spacerAttached = true; //ToDo: Get rid of this line in the actual code, this is toggeled tru just to make tessting easier
  if(spacerAttached == true && hasConfig == true)
  {
    //toggle operation mode bools on and off
    bool hasTag = nfc.tagPresent(10);
    //First priority toggle medication mode on
    if (hasTag && hasInhaler == false && inPEFRMode == false)
    {
      NfcTag tag = nfc.read();
      if (tag.hasNdefMessage())
      {

        NdefMessage message = tag.getNdefMessage();
        // cycle through the records in the tag and puting the data into the curInhaler object
        int recordCount = message.getRecordCount();
        for (int i = 0; i < recordCount; i++)
        {
          NdefRecord record = message.getRecord(i);
          int payloadLength = record.getPayloadLength();
          byte payload[payloadLength];
          record.getPayload(payload);

          // Force the data into a String (might work depending on the content)
          String payloadAsString = "";
          for (int c = 0; c < payloadLength; c++) {
            payloadAsString += (char)payload[c];
          }

          //creating curInhaler
          if(i == 0){
            curInhaler.type = extractValue(payloadAsString, ":");
          }
          else if (i == 1){
            curInhaler.color = extractValue(payloadAsString, ":");
          }
          else if (i == 2){
            curInhaler.commercialName = extractValue(payloadAsString, ":");
          }
          else if (i == 3){
            curInhaler.medication = extractValue(payloadAsString, ":");
          }
        }
      }

      if (curInhaler.type.length() != 0)
      {
        //inhaler has been attached
        hasInhaler = true;
        curInhaler.startTime = getTime();
        //ToDo: light up LEDs based on which inhaler has been attached

        //making file name
        
        
        int fileCount= countFiles();
        char tempFileName[12];
        sprintf(tempFileName, "M%07d.TXT", fileCount);
        eventFileName = (String)tempFileName;
        
        //log the header of the medication event
        Serial.println("INHALER FOUND---------------------"); 
        Serial.print(F("Saving the following data to: "));
        Serial.println(eventFileName);
        logMedicationHeader(eventFileName, curInhaler);
        Serial.println();
      }
    }
    //second priority toggle medication mode off
    else if (hasTag == false && hasInhaler == true)
    {
      //inhaler has been disconnecteed
      hasInhaler = false;
      eventFileName = "UNKNOWN.TXT"; //reset eventFileName
      curInhaler = Inhaler(); //clear the inhaler object
      Serial.println("INHALER REMOVED---------------------"); 
      Serial.print("Time: ");
      Serial.println(getTime());
    }
    else if(config.PEFRActive == true && hasInhaler == false && hasTag == false)  //third priority is toggling the isPEFRMode on and off/moving to next trial
    {
      checkNextHold(); //check if the next button is being held

      //third priority toggle exhalation mode on when next is held
      if(inPEFRMode == false && nextButtonHeld == true)
      {
        nextButtonHeld = false; //reset the nextButtonHeld flag
        inPEFRMode = true;

        int fileCount= countFiles();
        char tempFileName[12];
        sprintf(tempFileName, "E%07d.TXT", fileCount);
        eventFileName = (String)tempFileName;

        PEFRTest.startTime = getTime();
        
        //initialize PEFR value and attempt values]
        for (int i = 0; i < 4; i++) {
          PEFRTest.vals[i] = 0; //initialize PEFR values for each trial to 0
          PEFRTest.attemptNum[i] = 1; //initalize PEFR attempt num to 1
        }

        Serial.println("PEFR TEST INITIATED---------------------"); 
        Serial.print("Time: ");
        Serial.println(PEFRTest.startTime);
      }

      //fourth priority deactivate exhalation mode
      else if(inPEFRMode == true && PEFRTrial == 3 && nextPressed == true)
      {
        nextPressed = false; // reset the nextPressed flag

        //complete the header
        Serial.println("PEFR TEST COMPLETE---------------------"); 
        Serial.print(F("Saving the following data to: "));
        Serial.println(eventFileName);
        logExhalationHeader(eventFileName, PEFRTest);
        Serial.println();

        //Reset
        inPEFRMode = false;
        PEFRTrial = 0;
        PEFRAttempt = 1;
        eventFileName = "UNKNOWN.TXT";
        PEFRTest = PeakFlowRateTest(); 
      }
    }


    //Actions while in modes
    if(hasInhaler == true)
    {
      int flowRate = 77; //ToDo: insert code to get the flow rate
      if(flowRate > 10)//adjust the threshold
      {
        Details medicationDetail;
        medicationDetail.timestamp = getTime();
        medicationDetail.flowRate = flowRate;
        medicationDetail.zone = "green"; // ToDo: insert code for zone color determination based on flow rate (lights stuff)
        //ToDo: light up LEDs based on zone

        JsonDocument detailsDoc;
        JsonObject details = detailsDoc.to<JsonObject>();
        details[F("jsonContent")] = F("details");
        details[F("flowRate")] = String(medicationDetail.flowRate) + " L/min";
        details[F("zone")] = medicationDetail.zone; 
        details[F("timestamp")] = medicationDetail.timestamp;

        Serial.println(F("Saving the following data to: "));
        Serial.println(eventFileName);
        serializeJsonPretty(details, Serial);
        Serial.println();
        appendJson(eventFileName, details);
        detailsDoc.clear();
      }
      Serial.println();
    }
    else if (inPEFRMode == true)
    {
      //ToDo: blink LED based on trial number
      if (nextPressed == true)
      {
        nextPressed = false; //reset nextPressed flag

        PEFRTrial += 1; //move onto the next trial
        PEFRAttempt = 1; //reset attempts
        Serial.print(F("TRIAL "));
        Serial.print(String(PEFRTrial+1));
        Serial.print(F(", ATTEMPT: "));
        Serial.print(PEFRAttempt);
        Serial.println(F("----------------------------------------------------"));
      }
      else if (redoPressed == true)
      {
        redoPressed = false; //reset the redoPressed flag

        PEFRAttempt += 1;
        PEFRTest.attemptNum[PEFRTrial] = PEFRAttempt;
        Serial.println("REDO MEEEEE");
      }

      //logging flow rate
      //ToDo: light up LEDs code
      int flowRate = (PEFRTrial+1) * 100; //ToDo: flowRate measuringcode
      if(flowRate > 50) //adjust the threshold
      {
        Details exhalationDetail;
        exhalationDetail.flowRate = flowRate;
        exhalationDetail.trial = PEFRTrial + 1; //PEFRTrial is 0-indexed, add 1
        exhalationDetail.attempt = PEFRAttempt;
        exhalationDetail.timestamp = getTime();
        exhalationDetail.percentage = flowRate*100/config.baseline; //ToDo: find percentage based on flowrate and baseline

        //If we reached a new peak
        if (flowRate > PEFRTest.vals[PEFRTrial]){
          PEFRTest.vals[PEFRTrial] = flowRate;
          PEFRTest.percents[PEFRTrial] = exhalationDetail.percentage;
          PEFRTest.timestamps[PEFRTrial] = exhalationDetail.timestamp;
        }

        JsonDocument detailsDoc;
        JsonObject details = detailsDoc.to<JsonObject>();
        details[F("jsonContent")] = F("details");
        details[F("trial")] = exhalationDetail.trial; 
        details[F("attempt")] = exhalationDetail.attempt; 
        details[F("flowRate")] = String(exhalationDetail.flowRate) + " L/min";
        details[F("percentage")] = String(exhalationDetail.percentage) + "%";
        details[F("timestamp")] = exhalationDetail.timestamp;

        Serial.println(F("Saving the following data to: "));
        Serial.println(eventFileName);
        serializeJsonPretty(details, Serial);
        Serial.println();
        appendJson(eventFileName, details);
        detailsDoc.clear();
      }
    }
    else 
    {
      // Todo: If you are not in any of the modes, then search for a bluetooth connection
      Serial.println(F("Connect Inhaler or Activate PEFR"));
    }
    delay(100); //adjust the delay if necessary
  }
  else
  {
    //ToDo: blutooth searching at a slower rate
    Serial.println(F("CONNECT SPACER"));
    delay(5000); //adjust delay if necessary
  }
}


//FUNCTIONS==============================================================================
void printState(void)
{
  Serial.println("---------------------------------------------");

  Serial.print("HasInhaler:");
  Serial.println(hasInhaler);
  Serial.println();

  curInhaler.print();
  Serial.println();
}

String extractValue(String str, String delimiter){
    String value;
    int delimiterPos = str.indexOf(delimiter);  // Returns the index of the first occurrence

    // Extract the value after the delimiter
    if (delimiterPos != -1) {  // Check if delimiter is found
      value = str.substring(delimiterPos + 1);  // Get the substring after the delimiter
    } else {
      Serial.println("Delimiter not found!");
  }
  return value;
}

String getTime(){
  DateTime now = rtc.now();
  unsigned long currentMillis = millis();
  long nowMillis  = currentMillis % 1000;     // Milliseconds since last second

  String nowString;
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

  return nowString;
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

void appendJson(String filename, JsonObject json){
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.print(F("Failed to create file: "));
    Serial.println(filename);
    return;
  }

  // Serialize JSON to file
  if (serializeJsonPretty(json, file) == 0) {
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

void logMedicationHeader(String filename, Inhaler curInhaler){
  header = headerDoc.to<JsonObject>();
  header[F("jsonContent")] = F("header");
  header[F("userID")] = config.userID;
  header[F("spacerUDI")] = config.spacerUDI;
  header[F("dataType")] = F("medication");
  header[F("startTime")] = curInhaler.startTime;

  JsonObject medicationType = header.createNestedObject(F("medicationType"));
  medicationType[F("type")] = curInhaler.type;
  medicationType[F("color")] = curInhaler.color;
  medicationType[F("commercialName")] = curInhaler.commercialName;
  medicationType[F("medication")] = curInhaler.medication;

  serializeJsonPretty(header,Serial);
  Serial.println();

  appendJson(filename, header);
  headerDoc.clear();
  return;
}

void logExhalationHeader(String filename, PeakFlowRateTest PEFRTest){
  header = headerDoc.to<JsonObject>();
  header[F("jsonContent")] = F("header");
  header[F("userID")] = config.userID;
  header[F("spacerUDI")] = config.spacerUDI;
  header[F("dataType")] = F("exhalation");
  header[F("startTime")] = PEFRTest.startTime;

  JsonArray PEFRs = header.createNestedArray(F("PEFRs"));
  for(int index = 0; index < 4; index++){
    JsonObject entry = PEFRs.createNestedObject();
    entry[F("trial")] = index+1;
    entry[F("attempt")] = PEFRTest.attemptNum[index];
    entry[F("flowRate")] = String(PEFRTest.vals[index]) + " L/min";
    entry[F("percentage")] = String(PEFRTest.percents[index]) + "%";
    entry[F("timestamp")] = PEFRTest.timestamps[index];
  }

  //ToDo: Potentially do some calculations to determine avePEFR on the arduino

  serializeJsonPretty(header,Serial);
  Serial.println();

  appendJson(filename, header);
  headerDoc.clear();
  return;
}

int countFiles() {
  int count = 0;

  // Open the root directory
  File root = SD.open("/");
  // Check if the directory opened correctly
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


void nextButtonISR() {
  if (millis() - nextLastPressTime > debounceDelay) {
    if(nextWasHeld){
      nextWasHeld = false;
    }
    else{
      nextPressed = true;
      nextLastPressTime = millis(); 
    }
  }
}

void redoButtonISR() {
  if (millis() - redoLastPressTime > debounceDelay) {
    redoPressed = true;
    redoLastPressTime = millis();
  }
}

void checkNextHold(){
  // Check if the next button has been held for more than 3 seconds
  bool pinVal = digitalRead(nextPin);
  if (pinVal==HIGH && (millis() - nextButtonPressStartTime) > longPressThreshold && nextWasHeld==false){
    nextButtonHeld = true;
    nextWasHeld = true;
    delay(50);
  }
  else if (pinVal == LOW){
    nextButtonPressStartTime = millis();
  }
}