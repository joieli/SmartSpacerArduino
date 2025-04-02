#include <Wire.h>

#define SENSOR_ADDRESS 0x28  // i2c address from datasheet
#define CPU_HZ 48000000
#define TIMER_PRESCALER_DIV 64

void startTimer(int frequencyHz);
void setTimerFrequency(int frequencyHz);
void TC3_Handler();

volatile bool sensorReadNeeded = false;
volatile uint32_t totalElapsedUs = 0; // Free-running microseconds counter
volatile uint32_t overflowCount = 0;  // For TC4 overflow tracking
volatile uint16_t lastCount = 0;     // For TC4 overflow detection
volatile uint32_t interruptTime = 0;

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
  Wire.endTransmission(false);  // keep i2c bus active
  
  Wire.requestFrom(SENSOR_ADDRESS, 4, false);    // request 4 bytes
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
  Serial.begin(9600);  // start serial read
  startTimer(200);     // 200 Hz = 5ms
  startFreeRunningTimer();
  while(!Serial);
}

void loop() {
  if (sensorReadNeeded){
    sensorReadNeeded = false;
    uint32_t timestamp = getTimeUs(); // Timestamp when ISR fired
    //uint32_t timestamp = getTimeUs();
    uint32_t now = getMicroseconds();
    get_data(); // Takes 900µs–1330µs
    Serial.print(now);
    Serial.print(",");
    Serial.print(interruptTime);
    Serial.print(",");
    Serial.print(temperature);
    Serial.print(",");
    Serial.print(vol_flow);
    Serial.print(",");
    Serial.print(pressure);
    uint32_t readDuration = getTimeUs() - timestamp;
    Serial.print(",");
    Serial.println(readDuration);

    // Safety check (warn if sensor read took > 4ms)
    if (readDuration > 4000) {
      Serial.print("Sensor read too slow! Took ");
      Serial.print(readDuration);
      Serial.println("µs");
    }
  }
}

// Returns current time in microseconds (free-running)
uint32_t getTimeUs() {
  noInterrupts(); // Prevent ISR from updating totalElapsedUs mid-read
  TcCount16* TC = (TcCount16*) TC3;
  uint32_t ticks = TC->COUNT.reg;
  uint32_t elapsed = totalElapsedUs + (ticks * 1000000UL) / (CPU_HZ / TIMER_PRESCALER_DIV);
  interrupts();
  return elapsed;
}

// Timer Configuration
void setTimerFrequency(int frequencyHz) {
  int compareValue = (CPU_HZ / (TIMER_PRESCALER_DIV * frequencyHz)) - 1;
  TcCount16* TC = (TcCount16*) TC3;
  // Make sure the count is in a proportional position to where it was
  // to prevent any jitter or disconnect when changing the compare value.
  TC->COUNT.reg = map(TC->COUNT.reg, 0, TC->CC[0].reg, 0, compareValue);
  TC->CC[0].reg = compareValue;
  // Serial.println(TC->COUNT.reg);
  // Serial.println(TC->CC[0].reg);
  while (TC->STATUS.bit.SYNCBUSY == 1);
}

void startTimer(int frequencyHz) {
  REG_GCLK_CLKCTRL = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TCC2_TC3) ;
  while ( GCLK->STATUS.bit.SYNCBUSY == 1 ); // wait for sync

  TcCount16* TC = (TcCount16*) TC3;

  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use the 16-bit timer
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Use match mode so that the timer counter resets when the count matches the compare register
  TC->CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  // Set prescaler to 64
  TC->CTRLA.reg |= TC_CTRLA_PRESCALER_DIV64;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync

  setTimerFrequency(frequencyHz);

  // Enable the compare interrupt
  TC->INTENSET.reg = 0;
  TC->INTENSET.bit.MC0 = 1;

  NVIC_EnableIRQ(TC3_IRQn);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY == 1); // wait for sync
}

void TC3_Handler() {
  TcCount16* TC = (TcCount16*) TC3;
  if (TC->INTFLAG.bit.MC0 == 1) {
    TC->INTFLAG.bit.MC0 = 1;
    // Write callback here!!!
    interruptTime = getMicroseconds();
    sensorReadNeeded = true;
  }
}

void startFreeRunningTimer() {
  REG_GCLK_CLKCTRL = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID_TC4_TC5);
  while (GCLK->STATUS.bit.SYNCBUSY);

  TcCount16* TC = (TcCount16*) TC4;
  TC->CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC->STATUS.bit.SYNCBUSY);

  // Configure TC4 in 16-bit NFRQ mode (no reset)
  TC->CTRLA.reg |= TC_CTRLA_MODE_COUNT16 | TC_CTRLA_WAVEGEN_NFRQ | TC_CTRLA_PRESCALER_DIV64;
  while (TC->STATUS.bit.SYNCBUSY);

  TC->CTRLA.reg |= TC_CTRLA_ENABLE;  // Start timer
  while (TC->STATUS.bit.SYNCBUSY);
}

uint32_t getMicroseconds() {
  //return ((TcCount16*)TC4)->COUNT.reg * (1000000UL / (CPU_HZ / TIMER_PRESCALER_DIV));
  noInterrupts();
  TcCount16* TC = (TcCount16*) TC4;
  uint16_t currentCount = TC->COUNT.reg;
  uint32_t overflowSnapshot = overflowCount;
  interrupts();

  // Detect overflow (if timer wrapped around)
  if (currentCount < lastCount) {
    overflowSnapshot++;
  }
  lastCount = currentCount;

  // Calculate total µs
  uint64_t totalTicks = (uint64_t)overflowSnapshot * 65536 + currentCount;
  return (totalTicks * 1000000UL) / (CPU_HZ / TIMER_PRESCALER_DIV);
}