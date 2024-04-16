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
const int LOW_BAT_LED_DURATION= 200;      // How long to flash LED (in ms) for battery low 
const int LOW_BAT_LED_INTERVAL= 1800;     // How long between flashes (in ms)
const int POWER_LED_DURATION= 200;        // How long to flash LED (in ms) to indicate power is on
const int COUNT_UNTIL_FLASH= 10;          // Flash the LED after this many measurements just to show power is on
int countUntilFlash;

// ADC reference adjust.  Set as a percent by measuring battery and comparing to calculated value
const float ADC_REF_ADJUST = 1.08;

// Low limit (in millivolts) before signaling low battery 
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


  if (!laserRanger.begin()) {
    Serial.println(F("Failed to boot laserRanger VL53L0X"));
    while(1);
  }

  // One flash to show all is well
  digitalWrite(LED,1);
  delay(1000);
  digitalWrite(LED,0);
}


//***************************************************
//  Main loop section
//***************************************************
void loop() {
  // Enter power down state for 1 second with ADC and BOD module disabled.
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  
  
  // Waking up.  Go check the sensor (but first see if we need to flash the LED)
  if(countUntilFlash-- == 0) {
    countUntilFlash=COUNT_UNTIL_FLASH;
    digitalWrite(LED,1);
    delay(POWER_LED_DURATION);
    digitalWrite(LED,0);
  }

  // First check the range Pot.  No mapping necessary as we are using 1023mm as our max distance.
  // So, just read the 0-1023 value and use as our alarm range.  
  alarmDist = analogRead(A0);
  distanceToObject = checkForObject();

  // Get the current battery voltage in millivolts. vbat = ((ADC/1023) * 1.1v)/(R1/(R1+R2))
  // We use a voltage divider R2 over R1 to drop 4.2v down to 1.1v for the ADC as we're using the internal 1.1v bandgap
  batteryVoltage = analogRead(A1) * 4.1 * ADC_REF_ADJUST;

  // If the battery is drained to the limit, go into flashing loop
  if(batteryVoltage <= BATTERY_LOW_LIMIT) {
    while(1) {
      digitalWrite(LED,1);
      delay(LOW_BAT_LED_DURATION);
      digitalWrite(LED,0);
      delay(LOW_BAT_LED_INTERVAL);
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
