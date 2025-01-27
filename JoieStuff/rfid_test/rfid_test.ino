#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

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

//vars
bool hasInhaler = false;
Inhaler curInhaler;

void setup(void) {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("====================CODE BEGINS===========");
    Serial.println("NDEF Reader");
    nfc.begin();
    Serial.println();
}

void loop(void) {
    //toggle between hasInhaler and does not have inhlaer modes
    bool hasTag = nfc.tagPresent(10);
    if (hasTag && hasInhaler == false)
    {
      NfcTag tag = nfc.read();
      Serial.println(tag.getTagType());
      Serial.print("UID: ");
      Serial.println(tag.getUidString());

      if (tag.hasNdefMessage()) // every tag won't have a message
      {

        NdefMessage message = tag.getNdefMessage();
        Serial.print("\nThis NFC Tag contains an NDEF Message with ");
        Serial.print(message.getRecordCount());
        Serial.print(" NDEF Record");
        if (message.getRecordCount() != 1) {
          Serial.print("s");
        }
        Serial.println(".");

        // cycle through the records, printing some info from each
        int recordCount = message.getRecordCount();
        for (int i = 0; i < recordCount; i++)
        {
          Serial.print("\nNDEF Record ");Serial.println(i+1);
          NdefRecord record = message.getRecord(i);

          // The TNF and Type should be used to determine how your application processes the payload
          // There's no generic processing for the payload, it's returned as a byte[]
          int payloadLength = record.getPayloadLength();
          byte payload[payloadLength];
          record.getPayload(payload);

          // Print the Hex and Printable Characters
          Serial.print("  Payload (HEX): ");
          PrintHexChar(payload, payloadLength);

          // Force the data into a String (might work depending on the content)
          // Real code should use smarter processing
          String payloadAsString = "";
          for (int c = 0; c < payloadLength; c++) {
            payloadAsString += (char)payload[c];
          }
          Serial.print("  Payload (as String): ");
          Serial.println(payloadAsString);

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

          // id is probably blank and will return ""
          String uid = record.getId();
          if (uid != "") {
            Serial.print("  ID: ");Serial.println(uid);
          }
        }

        curInhaler.print();
      }

      if (curInhaler.type.length() != 0)
      {
        hasInhaler = true;
        Serial.println("INHALER FOUND---------------------"); 
      }
    }
    else if (hasTag == false && hasInhaler == true)
    {
      hasInhaler = false;
      curInhaler = Inhaler();
      Serial.println("INHALER REMOVED---------------------"); 
    }

  //Actions in modes
    if(hasInhaler == true)
    {
      Serial.println("Do Stuff");
    }
    else
    {
      Serial.println("Connect Inhaler");
      printState();
    }
    delay(100);
}

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