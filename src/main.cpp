#include <Arduino.h>
#include "Adafruit_VL53L0X.h"
#include <LowPower.h>


// Range definitions
VL53L0X_RangingMeasurementData_t measureLaserRange;
Adafruit_VL53L0X laserRanger = Adafruit_VL53L0X();
int distanceToObject = 5000;   // Set to big default number in case we want to test for out of range
int alarmDist;
int batteryVoltage;

// **************   I/O Pin Definitions  *********
const uint8_t BUZZER = 3;           // Buzzer + pin

// Timer vars
const int WARNING_BUZZ_DURATION = 2000;   // How long (ms) to buzz the buzzer to warn of proximity violation
const int CHIRP_DURATION= 100;            // How long to chirp when the battery is low (in ms)
const long CHIRP_INTERVAL= 60000;         // How long between chirps (in ms)
const uint8_t DETECT_INTERVAL = 2;        // Number of seconds between checking detector

// ADC reference adjust.  Set as a percent by measuring battery and comparing to calculated value
const float ADC_REF_ADJUST = 1.08;

// Low limit (in millivolts) before signaling low battery (with chirps)
const int BATTERY_LOW_LIMIT = 3500;  

// Prototypes
int checkForObject();

void setup() {
  // Turn on serial monitor
  Serial.begin(115200);
  delay(1000);

  // Pot to set the alarm range.  Sets the alarmDist.  
  pinMode(A0, INPUT);  //Range potentiometer
  pinMode(BUZZER,OUTPUT);

  // Use 1.1v bandgap as reference for ADC's
  analogReference(INTERNAL);
  
  digitalWrite(BUZZER,0);

  if (!laserRanger.begin()) {
    Serial.println(F("Failed to boot laserRanger VL53L0X"));
    while(1);
  }
}


void loop() {
  // Enter power down state with ADC and BOD module disabled.
  LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);  
  
  // Waking up.  Go check the sensor

  // First check the range Pot.  No mapping necessary as we are using 1023mm as our max distance.
  // So, just read the 0-1023 value and use as our alarm range.  
  alarmDist = analogRead(A0);
  distanceToObject = checkForObject();
  

  // Print out the current battery voltage in millivolts. vbat = ((ADC/1023) * 1.1v)/(R1/(R1+R2))
  // We use a voltage divider R2 over R1 to drop 4.2v down to 1.1v for the ADC as we are using the internal 1.1v bandgap
  batteryVoltage = analogRead(A1) * 4.1 * ADC_REF_ADJUST;

  // If the battery is drained to the limit, go into chirping loop
  if(batteryVoltage <= BATTERY_LOW_LIMIT) {
    while(1) {
      digitalWrite(BUZZER,1);
      delay(CHIRP_DURATION);
      digitalWrite(BUZZER,0);
      delay(CHIRP_INTERVAL);
    }
  }

  //Serial.print("Battery: "); Serial.println(batteryVoltage);
  //delay(50); // Need to wait for serial buffer to clear else characters will get corrupted when we loop around to sleep...

  // If Kitty getting close, buzz the warning buzzer
  if(distanceToObject <= alarmDist) {
    //Serial.print("alarmDist: "); Serial.println(alarmDist);
    //Serial.print("distanceToObject "); Serial.println(distanceToObject);
    digitalWrite(BUZZER,1);
    //Serial.print("Buzzzzz! "); Serial.print(WARNING_BUZZ_DURATION); Serial.println(" milliseconds");
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
    if(measureLaserRange.RangeStatus != 4) {   // 4 indicates out of range
       curMeasurement =  measureLaserRange.RangeMilliMeter;
       if(i == 0) {
         aveMeasurement = curMeasurement;
         continue;
       }
       if(abs(aveMeasurement - curMeasurement) > 50) {
         return(5000);
       }
       aveMeasurement = (aveMeasurement + curMeasurement) / (i+1);
       delay(50);
    } else {
      return(5000);
    }
  }  
  return(aveMeasurement);
}
