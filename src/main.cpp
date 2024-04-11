/* Kitty detector
 
   Uses a VL53L0X laser range finder to detect objects (in this case we are 
   looking for a trespassing kitty on the counter)...

   The detection range is controlled with a potentiometer.  Range can be 0-1023mm.
   You could actually extend this to about 2000mm (the range of the laser ranger) 
   but I was being lazy and feeding the range pot into one of the ADC inputs and 
   reading the ADC count raw.

   The Arduino Pro Mini 3.3v version was used for its small size and reasonably low
   power.  I'm using the LowPower lib to put the Arduino to sleep for a second, then
   wake up, do a range check (and buzz the alarm if necessary), then back to sleep.   
   Average current is about 2mA which should result in about a month battery life with 
   one 18650 cell.

   dlf 3/16/2024
*/

#include <Arduino.h>
#include "Adafruit_VL53L0X.h"
#include <LowPower.h>


// Range definitions
VL53L0X_RangingMeasurementData_t measureLaserRange;
Adafruit_VL53L0X laserRanger = Adafruit_VL53L0X();
int distanceToObject = 5000;   // Set to big default number in case we want to test for out of range
int alarmDist;

// I/O Pin Definitions
const uint8_t BUZZER = 3;     // Buzzer + pin
const uint8_t LED = 5;        // Low batter LED

// Timer vars
const int WARNING_BUZZ_DURATION = 2000;   // How long (ms) to buzz the buzzer to warn of proximity violation
const int CHIRP_DURATION= 100;            // How long to chirp when the battery is low (in ms)
const long CHIRP_INTERVAL= 60000;         // How long between chirps (in ms)
const int LED_DURATION= 500;              // How long to flash LED (in ms)
const int LED_INTERVAL= 5000;             // How long between flashes (in ms)

// ADC reference adjust.  Set as a percent by measuring battery and comparing to calculated value
const float ADC_REF_ADJUST = 1.08;

// Low limit (in millivolts) before signaling low battery (with chirps)
const int BATTERY_LOW_LIMIT = 3500;  
int batteryVoltage;

// Prototypes
int checkForObject();

//***************************************************
//  Run once setup section
//***************************************************
void setup() {
  // Turn on serial monitor
  Serial.begin(115200);
  delay(1000);

  // Pot to set the alarm range.  Sets the alarmDist.  
  pinMode(A0, INPUT);  //Range potentiometer
  pinMode(BUZZER,OUTPUT);
  pinMode(LED,OUTPUT);

  // Use 1.1v bandgap as reference for ADC's
  analogReference(INTERNAL);
  
  digitalWrite(BUZZER,0);
  digitalWrite(LED,0);

  if (!laserRanger.begin()) {
    Serial.println(F("Failed to boot laserRanger VL53L0X"));
    while(1);
  }
}


//***************************************************
//  Main loop section
//***************************************************
void loop() {
  // Enter power down state for 1 second with ADC and BOD module disabled.
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  
  
  // Waking up.  Go check the sensor

  // First check the range Pot.  No mapping necessary as we are using 1023mm as our max distance.
  // So, just read the 0-1023 value and use as our alarm range.  
  alarmDist = analogRead(A0);
  distanceToObject = checkForObject();

  // Get the current battery voltage in millivolts. vbat = ((ADC/1023) * 1.1v)/(R1/(R1+R2))
  // We use a voltage divider R2 over R1 to drop 4.2v down to 1.1v for the ADC as we're using the internal 1.1v bandgap
  batteryVoltage = analogRead(A1) * 4.1 * ADC_REF_ADJUST;

  // If the battery is drained to the limit, go into chirping loop
  if(batteryVoltage <= BATTERY_LOW_LIMIT) {
    while(1) {
      digitalWrite(LED,1);
      delay(LED_DURATION);
      digitalWrite(LED,0);
      delay(LED_INTERVAL);
    }
  }

  // If Kitty getting close, buzz the warning buzzer
  if(distanceToObject <= alarmDist) {
    digitalWrite(BUZZER,1);
    delay(WARNING_BUZZ_DURATION);
    digitalWrite(BUZZER,0);
  } 
}

//##########################################################################
// Functions
//##########################################################################

// Check for any object, if we find one within range, check again.  If successive results are within 50mm 
// of previous, then return the average distance (in mm), else return out-of-range value (5000)
int checkForObject() {
  int curMeasurement;
  int aveMeasurement = 0;

  for(uint8_t i=0; i<3 ; i++) {
    laserRanger.rangingTest(&measureLaserRange, false); // pass in 'true' to get debug data printout!
    delay(50);
    if(measureLaserRange.RangeStatus != 4) {   // 4 indicates out of range
       curMeasurement =  measureLaserRange.RangeMilliMeter;
       if(i == 0) {
         aveMeasurement = curMeasurement;
         continue;
       }
       if(abs(aveMeasurement - curMeasurement) > 50) {
         return(5000);
       }
       aveMeasurement = (aveMeasurement + curMeasurement) / 2;
    } else {
      return(5000);
    }
  }  
  return(aveMeasurement);
}
