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
const char *dataFileName = "data.txt";
const char *configFileName = "config.txt";

//fileContent
struct Config{
  String userID;
  String spacerUDI;
};

Config config;

JsonDocument recordDoc;
JsonObject record;
JsonArray details;

const int chipSelect = 10;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial){;}

  Serial.println("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("Initialization failed!");
    while (true);
  }

  Serial.println("Initialization done. SD Card found.");

  card.init(SPI_HALF_SPEED, chipSelect);
  // print the type of card
  Serial.print("    Card type:         ");
  switch (card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  //printing directory
  File root = SD.open("/");
  printDirectory(root, 0);

  //read data.txt if it exists
  Serial.println();
  Serial.println("Checking for the existence of " + String(dataFileName));
  if (SD.exists(dataFileName)){
    Serial.println("    " + String(dataFileName) + " already exists");
    Serial.println("Printing existing content...");

    printContents(dataFileName);
  }
  else{
    Serial.println("    " + String(dataFileName) + " does not exist");
  }

  //read config.txt if it exists
  Serial.println();
  Serial.println("Checking for the existence of " + String(configFileName));
  if (SD.exists(configFileName)){
    Serial.println("    "+ String(configFileName) +" already exists");
    Serial.println("Printing existing content...");

    printContents(configFileName);
  }
  else{
    Serial.println("    "+ String(configFileName) +" does not exist");
  }
}

void loop(void) {
  delay(2000);
  for(int i = 1; i <= 10; i++ ){
    Serial.println("Iteration" + String(i) + "==============================================================================================================");
    //Test code
    /*
    if (i == 1){
      //beginning of inhalation
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
    else if (i>1 && i<=4){
      //adding inhalation data
      JsonObject entry = details.createNestedObject();
      entry["value"] = "value" + i;
      entry["zone"] = "green " + i;
      entry["timestamp"] = "YYYY-MM-DD";
      
      serializeJsonPretty(record, Serial);
      Serial.println();
    }
    else if (i==5){
      //end of inhalation
      appendJson(dataFileName, record);
      recordDoc.clear();
    }
    else if (i ==6){
      //beginning of exhalation

    }
    else if (i>6 && i<10){
      //adding exhalation data

    }
    else if (i==10){
      //end of exhalation
    }
    */

    appendToFile(dataFileName, "Happy " + String(i));
    delay(1000);

    config.userID = "00000" + String(i);
    config.spacerUDI = "SPACER12345";
    rewriteConfigFile(configFileName, config);


    printContents(dataFileName);
    printContents(configFileName);
    delay(1000);
  }
}

void printContents(const char *fileName){
  File file = SD.open(fileName);
  if (file) {
    Serial.println("BEGINNING OF " + String(fileName) + "=========================");
    while (file.available()) {
      Serial.write(file.read());
    }
  file.close();
  Serial.println("END OF DOCUMENT=========================");
  }
  else{
    Serial.println("    error opening " + String(fileName));
  }
}

void appendToFile(const char *filename, String content){
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file: " + String(filename));
    return;
  }

  Serial.println("  content appended to " + String(filename));
  file.println(content);
  file.close();
}

void rewriteConfigFile(const char *filename, Config config){
  //remove exisitng config file to rewrite
  SD.remove(filename);
  
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file: " + String(filename));
    return;
  }

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["userID"] = config.userID;
  doc["spacerUDI"] = config.spacerUDI;

  // Serialize JSON to file
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("  Failed to write to " + String(filename));
  }
  else{
    Serial.println("   Updated " + String(filename));
  }
  file.println();

  // Close the file
  file.close();
}

void appendJson(const char *filename, JsonObject record){
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file: " + String(filename));
    return;
  }

  // Serialize JSON to file
  if (serializeJsonPretty(record, file) == 0) {
    Serial.println("  Failed to write to " + String(filename));
  }
  else{
    Serial.println("   Updated " + String(filename));
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
