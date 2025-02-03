//libraries
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

//pins etc.
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
RTC_PCF8523 rtc;
const int chipSelect = 10;

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

//classes and structs
class PeakFlowRateTest {
  public:
    int PEFRs[4];
    String startTime;

    void print() {
      Serial.println(F("PeakFlowTest"));
      Serial.print(F("  PEFRs: ["));
      for (int i=0; i<4; i++){
        Serial.print(PEFRs[i]+",");
      }
      Serial.println("]");
      Serial.print(F("  startTime: "));
      Serial.println(startTime);
    }
};

struct Config{
  String userID;
  String spacerUDI;
  bool PEFRActive;
  int baseline;
};

//vars of inhaler detection
bool hasInhaler = false;
Inhaler curInhaler;

//file vars
Sd2Card card;
SdVolume volume;
SdFile root;
String configFileName = "CONFIG.TXT";
String eventFileName = "UNKNOWN.TXT";
JsonDocument headerDoc;
JsonObject header;


Config config; //initalizing a config object

//vars for RTC
volatile unsigned long lastMillis = 0;  // Time of the last interrupt in millis()

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
  config.userID = "123456789";
  config.spacerUDI = "SPACER1234567890";
  config.PEFRActive = true;
  config.baseline = 522;

  rewriteConfigFile(configFileName, config);

}

//LOOP BLOCK====================================================================================================
void loop(void) {
    //toggle between hasInhaler and does not have inhlaer modes
    bool hasTag = nfc.tagPresent(10);
    if (hasTag && hasInhaler == false)
    {
      NfcTag tag = nfc.read();

      if (tag.hasNdefMessage()) // every tag won't have a message
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
        hasInhaler = true;
        int fileCount= countFiles();
        curInhaler.startTime = getTime();
        char tempFileName[12];
        sprintf(tempFileName, "M%07d.TXT", fileCount);
        eventFileName = (String)tempFileName;
        Serial.println("INHALER FOUND---------------------"); 
        
        //ToDo: Save curInhaler data in the pMDI connected event
        Serial.print(F("Saving the following data to: "));
        Serial.println(eventFileName);
        logMedicationHeader(eventFileName, curInhaler);
        Serial.println();
      }
    }
    else if (hasTag == false && hasInhaler == true)
    {
      hasInhaler = false;
      curInhaler = Inhaler();
      //ToDo: log the pMDI disconnected event
      Serial.println("INHALER REMOVED---------------------"); 
      Serial.print("Time: ");
      Serial.println(getTime());
    }

  //Actions in modes
    if(hasInhaler == true)
    {
      int flowRate = 77; //ToDo: insert code to get the flow rate
      //adjust the threshold below
      if(flowRate > 10){
        //insert code for lights
        String startTime = getTime();

        JsonDocument detailsDoc;
        JsonObject details = detailsDoc.to<JsonObject>();
        details[F("jsonContent")] = F("details");
        details[F("value")] = String(flowRate) + " L/s";
        details[F("zone")] = "green"; //insert code for determining zone based on flow rate
        details[F("timestamp")] = startTime;

        Serial.println(F("Saving the following data to: "));
        Serial.println(eventFileName);
        serializeJsonPretty(details, Serial);
        Serial.println();
        appendJson(eventFileName, details);
        detailsDoc.clear();
      }
      
      Serial.println()
    }
    else
    {
      // add another block for the exhalation mode
      // add in another block for the bluetooth mode
      Serial.println("Connect Inhaler");
    }
    delay(100); //adjust the delay if necessary
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
  header[F("medicationType")] = curInhaler.color;

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