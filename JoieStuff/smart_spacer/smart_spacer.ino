//works on uno

//libraries
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
#include "RTClib.h"

//pins etc.
RTC_DS3231 rtc;
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);

//classes
class Inhaler {
  public:
    String type;
    String color;
    String commercialName;
    String medication;

    void print() {
      Serial.println("Inhaler");
      Serial.print("  type: ");
      Serial.println(type);
      Serial.print("  color: ");
      Serial.println(color);
      Serial.print("  commercialName: ");
      Serial.println(commercialName);
      Serial.print("  medication: ");
      Serial.println(medication);
    }
};

//vars of inhaler detection
bool hasInhaler = false;
Inhaler curInhaler;

//vars for RTC
volatile unsigned long lastMillis = 0;  // Time of the last interrupt in millis()

//SETUP BLOCK==================================================================================================================
void setup(void) {
  Serial.begin(115200);

  while (!Serial); // wait for serial port to connect. Needed for native USB

  Serial.println("=========================START============================================");
  Serial.println("Searching for NDEF Reader ");
  nfc.begin();
  Serial.println();

  //RTC Stuff
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
        Serial.println("INHALER FOUND---------------------"); 
        
        //ToDo: Save curInhaler data in the pMDI connected event
        Serial.println("Saving the following data:");
        Serial.print("Time: ");
        Serial.println(getTime());
        curInhaler.print();
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
      Serial.print(getTime());
      Serial.print(" -> ");
      Serial.println("Do Stuff");
    }
    else
    {
      Serial.println("Connect Inhaler");
    }
    delay(100); //adjust the delay if necessary -- NOTE THIS DELAY REALLY ONLY WORKS IN THE HAS INHALER MODE FOR SOME REASON
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