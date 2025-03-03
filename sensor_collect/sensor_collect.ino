#include <Wire.h>

#define SENSOR_ADDRESS 0x28  // from datasheet

const int sampleTime = 5; // time between samples (ms)
unsigned long lastSample = 0;
float vol_flow, pressure, temperature;

float calculate_temperature(uint16_t raw) {
  // calculate temp from datasheet formula
  float temp = float(raw) * (200)/(2048 - 1) - 50;
  return temp;
}

float calculate_pressure(uint16_t raw) {  
  // subtract offset of 8192 counts and multiply Pa/count (2/13108)
  float pres = (float(raw) - 8192) * (2/float(13108)) * 1000;
  pres = pres - 3;  // offset 3
  return pres;
}

float calculate_volflow(float pres){
  // calculate vol. flow rate
  // Q = sqrt((2*pa)/(1.293 kg m-3 *(1/.000182 m2 ^2 - 1/.0013795 m2 ^2)
  if(pres < 0){
    vol_flow = 0;   // if pressure < 0, flow going wrong way and calculate as 0
  }
  else{
    vol_flow = sqrt((2*pres)/(1.293*((1/pow(0.000182,2))-(1/pow(0.0013795,2)))))*60000;
  }  
  return vol_flow;
}

void get_data() {
  uint32_t data = 0;

  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.endTransmission(false);
  
  Wire.requestFrom(SENSOR_ADDRESS, 4);    // request 4 bytes
  for(int i=0; i<4; i++)
  {
    data <<= 8;
    data |= Wire.read();
  }

  uint16_t rawPressure = (data >> 16) & 0x3FFF;   // get pressure from D16 to D29 (14 bits)
  uint16_t rawTemperature = (data >> 5) & 0x07FF; // get temperature from D5 to D15 (11 bits)
  
  pressure = calculate_pressure(rawPressure);
  temperature = calculate_temperature(rawTemperature);
  vol_flow = calculate_volflow(pressure);
}

void display_results(unsigned long currentTime){
  // print temperature results
  Serial.print("TEMPERATURE: ");
  Serial.print(temperature, 3);
  Serial.print(" C, ");

  // print pressure results
  Serial.print("DIFF PRESSURE: ");
  Serial.print(pressure, 3);
  Serial.print(" Pa, ");

  // print volumetric flow rate results
  Serial.print("VOL. FLOW RATE: ");
  Serial.print(vol_flow, 3);
  Serial.print(" L/min, ");
  Serial.println(currentTime);
}

void setup() {
  Wire.begin();        // join i2c bus
  Serial.begin(115200);  // start serial read
  delay(500);
}

void loop() {
  unsigned long currentTime = millis() ;

  if(currentTime - lastSample >= sampleTime){
    get_data();
    Serial.print(currentTime);
    Serial.print(",");
    Serial.print(vol_flow);
    Serial.print(",");
    Serial.println(pressure);
    lastSample = currentTime;
  }
}