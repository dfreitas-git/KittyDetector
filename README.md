# KittyDetector

* Cat "stay-off-counter" alarm.   Laser range finder detects cat.  If cat gets within the detection zone, then a warning beep sounds.  

* The detection zone is controlled with a potentiometer that will adjust from 0mm to 1023mm. 

* Code uses sleep to power down for one second, then do a quick range check, buzzing alarm if necessary, then back to sleep.
Circuit consumes about 2mA on average.   Runs on one 18650 battery which should be good for about a month.

* When a low battery voltage is detected, the buzzer will chrip once a minute to let you know to re-charge the battery.

## Parts List
- Arduino Pro Mini 3.3v
- VL53L0X laser range finder module ( https://www.amazon.com/gp/product/B08RRT1KJ6/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&th=1 )
- 5v continuous sounding Piezo-Buzzer
- 100K potentiometer for range-control
- Common Resistors/Caps/Diodes/Switches
- 18650 battery, holder, and 1S TP4056 BMS module

dlf  3/16/2024

## Schematics
![Alt text](./Schematic_KittyDetector.png "Schematic_KittyDetector")

## Build
## Single 18650 with 1S BMS soldered on the end and stuck in place with hot-melt glue.  The Rest just stuffed into the box (Fish Paper to keep things from shorting)!
![Alt text](./Inside.jpg "Inside of Box")
## Looks better on the outside.
![Alt text](./Outside.jpg "Outside of Box")
