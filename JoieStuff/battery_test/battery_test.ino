// Define the pin for the battery voltage monitoring (usually pin A7 on Feather M0)
const int batteryPin = A7;
int prevBattery = 0;

void setup() {
  Serial.begin(115200);
  while(!Serial); 
}

void loop() {
  float batteryVoltage = analogRead(batteryPin)*2*3.3/1024; //voltage divider divided by 2, multiple by 3.3 for ref voltage
  int batteryPercentage = map(batteryVoltage*1000, 3200, 4200, 0, 100); //map needs integer arguements
  
  if(prevBattery != batteryPercentage){
    Serial.print("Sending Battery: ");
    Serial.print(batteryPercentage);
    Serial.print(" ");
    Serial.println(prevBattery);

    prevBattery = batteryPercentage;
  }
  
  
  delay(1000);  // Update every second
}
