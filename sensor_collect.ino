#include <Wire.h>

#define SENSOR_ADDRESS 0x28  // from datasheet

unsigned long time;
int incomingChar;

float get_temperature(uint32_t data) {
  // get temperature from D5 to D15 (11 bits)
  unsigned int temperature = (data >> 5) & 0x07FF;
  
  // calculate temp from datasheet formula
  float temp_C = temperature * ((float)200)/(2048 - 1) - 50;
  
  // print temperature results
  Serial.print("TEMPERATURE: ");
  Serial.print(temp_C, 3);
  Serial.println(" C");
  
  return temp_C;
}

float get_pressure(uint32_t data) {
  // get pressure from D16 to D29 (14 bits)
  unsigned int pressure = (data >> 16) & 0x3FFF;
  
  // subtract offset of 8192 counts and multiply Pa/count (2/13108)
  float pres_Pa = ((int)pressure - 8192) * (2/(float)13108) * 1000;
  pres_Pa = pres_Pa - 3;
  
  // print pressure results
  /*Serial.print("DIFF PRESSURE: ");
  Serial.print(pres_Pa, 3);
  Serial.println(" Pa");*/

  float vol_flow;
  // calculate vol. flow rate
  // Q = sqrt((2*pres_Pa)/(1.293 kg m-3 *(1/.000182 m2 ^2 - 1/.0013795 m2 ^2)
  if(pres_Pa < 0){
    vol_flow = 0;
  }
  else{
    vol_flow = sqrt((2*pres_Pa)/(1.293*((1/pow(0.000182,2))-(1/pow(0.0013795,2)))))*60000;
  }
  //Serial.print("VOL. FLOW RATE: ");
  //Serial.println(vol_flow, 3);
  //Serial.println(" L/min");
  //Serial.println();
  
  return vol_flow;
}

uint32_t store_data() {
  uint32_t data = 0;
  Wire.requestFrom(SENSOR_ADDRESS, 4);    // request 4 bytes
  for(int i=0; i<4; i++)
  {
    data <<= 8;
    data |= Wire.read();
  }
  return data;
}

void setup() {
  Wire.begin();        // join i2c bus
  Serial.begin(9600);  // start serial read
  delay(500);
  time = millis();
}

void loop() {
    while(time < 300000){
      uint32_t data = store_data();
      float Pa = get_pressure(data);
      Serial.println(Pa, 3);
    }
    Serial.println("stop");
}
