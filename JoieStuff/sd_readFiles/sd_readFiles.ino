
#include <SD.h>

const int chipSelect = 10;
File root;
String fileName;

void setup() {
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // wait for Serial Monitor to connect. Needed for native USB port boards only:
  while (!Serial);

  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed.");
    while (true);
  }

  Serial.println("initialization done.");

  Serial.println("LIST OF DOCUMENTS=========");

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println();
  Serial.println("Which file do you want to read?");
}

void loop() {
  if(Serial.available()){
    fileName = Serial.readStringUntil('\n');
    Serial.println("READING " + fileName + "================================");
    printContents(fileName);
    Serial.println("END OF DOCUMENT=========================================");
  }
  
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

void printContents(String fileName)
{
  File file = SD.open(fileName);
  if (file) {
    while (file.available()) {
      Serial.write(file.read());
    }
  file.close();
  }
  else{
    Serial.println("    error opening " + fileName);
  }
}
