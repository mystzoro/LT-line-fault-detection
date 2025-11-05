/* Pole2 Node - ESP32 + SCT-013-030 using EmonLib
   - Measures RMS current
   - Sends to Pole1 hub via UART
*/

#include <Arduino.h>
#include "EmonLib.h"

#define ADC_PIN 34        // CT input
#define UART_TX 17        // TX to Pole1 RX
#define NOISE_THRESHOLD 1.5

EnergyMonitor emon2;

void setup() {
  Serial.begin(115200);       // For debugging
  Serial1.begin(9600, SERIAL_8N1, -1, UART_TX); // Only TX

  emon2.current(ADC_PIN, 30.0);  // SCT-013-030 calibration
}

void loop() {
  // Measure RMS
  float irms = emon2.calcIrms(1480);
  if (irms < NOISE_THRESHOLD) irms = 0.0;

  // Send via UART to Pole1
  Serial1.println(irms); 
  Serial.printf("Current in Pole2 : %.2f\n", irms);

  delay(200);
}
