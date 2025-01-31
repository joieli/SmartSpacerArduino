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
String configFileName = "config.txt";
String eventFileName = "YYYY.txt";

JsonDocument headerDoc;
JsonObject header;

//fileContent
struct Config{
  String userID;
  String spacerUDI;
  bool PEFRActive;
  int baseline;
};


int PEFRvals[4] = {111,222,333,444};

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

  delay(5000);
}

void loop(void) {
  delay(1000);
  Serial.println("Iteration" + String(i) + "==============================================================================================================");
  if(i%5 == 0){
    //change config file
    config.userID = "11223300000" + String(i);
    config.spacerUDI = F("SPACER12345ABC222");
    config.PEFRActive = true;
    config.baseline = 522;

    rewriteConfigFile(configFileName, config);

    //get inhaler data
    header = headerDoc.to<JsonObject>();
    header[F("jsonContent")] = F("header");
    header[F("userID")] = config.userID;
    header[F("spacerUDI")] = config.spacerUDI;
    header[F("dataType")] = F("medication");
    header[F("startTime")] = F("1111-11-1111");
    header[F("medicationType")] = F("blue");

    eventFileName = "YYYY_" + String(i) + ".txt";
    Serial.print(F("============FILENAME: "));
    Serial.println(eventFileName);
  }
  else if (i%5 == 4){
    
    //creating writing PEFRS header entry
    JsonArray PEFRs = header.createNestedArray("PEFRs");
    for(int index = 0; index < 4; index++){
      JsonObject entry = PEFRs.createNestedObject();
      entry[F("trial")] = index+1;
      entry[F("value")] = "value " + String(PEFRvals[index]);
      entry[F("percentage")] = F("78%");
      entry[F("timestamp")] = F("YYYY-MM-DD");
    }

    //creating avePEFR header entry
    JsonObject avePEFR = header.createNestedObject("avePEFR");
    avePEFR[F("value")] = "value" + String(i);
    avePEFR[F("percentage")] = F("78%");

    //saving header into SD card
    serializeJsonPretty(header,Serial);
    Serial.println();

    appendJson(eventFileName, header);
    headerDoc.clear();
  }
  else{
    //logging flow data
    JsonDocument detailsDoc;
    JsonObject details = detailsDoc.to<JsonObject>();
    details[F("jsonContent")] = F("details");
    details[F("trial")] = i;
    details[F("value")] = "value" + String(i);
    details[F("percentage")] = F("78%");
    details[F("zone")] = "green " + String(i);
    details[F("timestamp")] = F("YYYY-MM-DD");
    
    serializeJsonPretty(details, Serial);
    Serial.println();
    appendJson(eventFileName, details);
    detailsDoc.clear();
  }
  printContents(eventFileName);
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