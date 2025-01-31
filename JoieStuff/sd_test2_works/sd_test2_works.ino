/*
  The circuit:
    SD card attached to SPI bus as follows:
 ** MISO - pin 11 on Arduino Uno/Duemilanove/Diecimila
 ** MOSI - pin 12 on Arduino Uno/Duemilanove/Diecimila
 ** SCK - pin 13 on Arduino Uno/Duemilanove/Diecimila
 ** CS -  pin 10 depends on your SD card shield or module.
*/
// include the SD library:
#include <SPI.h>
#include <SD.h>
//incude json parsing
#include <ArduinoJson.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;
String dataFileName = "data.txt";
String configFileName = "config.txt";

JsonDocument recordDoc;
JsonObject record;
JsonArray details;

//fileContent
struct Config{
  String userID;
  String spacerUDI;
  bool PEFRActive;
  int baseline;
};

Config config;

const int chipSelect = 10;

int i = 0;

void setup() {  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial){;}

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

  //printing directory
  File root = SD.open("/");
  printDirectory(root, 0);
  root.close();

  //read data.txt if it exists
  Serial.println();
  Serial.print(F("Checking for the existence of "));
  Serial.println(dataFileName);
  if (SD.exists(dataFileName)){
    Serial.print(F("    ")); 
    Serial.print(dataFileName);
    Serial.println(F(" already exists"));
    Serial.println(F("Printing existing content..."));

    printContents(dataFileName);
  }
  else{
    Serial.print(F("    ")); 
    Serial.print(dataFileName);
    Serial.println(F(" does not exist"));
  }

  //read config.txt if it exists
  Serial.println();
  Serial.print(F("Checking for the existence of "));
  Serial.println(configFileName);
  if (SD.exists(configFileName)){
    Serial.print(F("    ")); 
    Serial.print(configFileName);
    Serial.println(F(" already exists"));
    Serial.println(F("Printing existing content..."));

    printContents(configFileName);
  }
  else{
    Serial.print(F("    ")); 
    Serial.print(configFileName);
    Serial.println(F(" does not exist"));
  }

  config.userID = "123456780";
  config.spacerUDI = "SPACER123456789";

  delay(2000);
}

void loop(void) {
  delay(1000);
  Serial.println("Iteration" + String(i) + "==============================================================================================================");
  if(i%5 == 0){
    //change config file
    config.userID = "00000" + String(i);
    config.spacerUDI = "SPACER12345";
    config.PEFRActive = true;
    config.baseline = 522;

    rewriteConfigFile(configFileName, config);

    //get inhaler data
      record = recordDoc.to<JsonObject>();
      details = record.createNestedArray("details");
      
      record["userID"] = "123456789";
      record["spacerUDI"] = "SPACER12345";
      record["dataType"] = "medication";
      record["startTime"] = "1111-11-11";
      record["medicationType"] = "blue";
      serializeJsonPretty(record, Serial);
      Serial.println();
  }
  else if (i%5 == 4){
    //saving data entry into SD card
    appendJson(dataFileName, record);
    recordDoc.clear();
  }
  else{
    //logging flow data
    JsonObject entry = details.createNestedObject();
    entry["value"] = "value" + i;
    entry["zone"] = "green " + i;
    entry["timestamp"] = "YYYY-MM-DD";
    
    serializeJsonPretty(record, Serial);
    Serial.println();
  }
  printContents(dataFileName);
  printContents(configFileName);
  i++;
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
    Serial.print(F("    error opening "));
    Serial.println(fileName);
  }
}

void rewriteConfigFile(String filename, Config config){
  //remove exisitng config file to rewrite
  SD.remove(filename);
  
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.print(F("Failed to create file: "));
    Serial.println(filename);
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  JsonDocument doc;
  JsonObject root= doc.to<JsonObject>();

  // Set the values in the document
  root["userID"] = config.userID;
  root["spacerUDI"] = config.spacerUDI;
  root["PEFRActive"] = config.PEFRActive;
  root["baseline"] = config.baseline;

  // Serialize JSON to file
  if (serializeJsonPretty(root, file) == 0) {
    Serial.print(F("  Failed to write to "));
    Serial.println(filename);
  }
  else{
    //Serial.println("   Updated " + String(filename));
  }
  file.println();

  // Close the file
  file.close();
}

void appendJson(String filename, JsonObject record){
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.print(F("Failed to create file: "));
    Serial.println(filename);
    return;
  }

  // Serialize JSON to file
  if (serializeJsonPretty(record, file) == 0) {
    Serial.print(F("  Failed to write to "));
    Serial.println(filename);
  }
  else{
    //Serial.println("   Updated " + String(filename));
  }
  file.println();

  // Close the file
  file.close();
}

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
    Serial.print(F("  "));
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println(F("/"));
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print(F("\t\t"));
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}