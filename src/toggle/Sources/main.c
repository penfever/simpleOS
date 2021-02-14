/*After your program completes its initialization, the Orange LED should be on and all the other LEDs should be off.  
Each time pushbutton SW1 is depressed, the lit LED will rotate one position to the Yellow LED (i.e., only the
Yellow LED will be lit after a single depression of SW1).  The next time pushbutton SW1 is depressed, only the Green LED will be
illuminated.  The next time pushbutton SW1 is depressed, only the Blue LED will be illuminated.  
Then, the process starts again the next time pushbutton SW1 is depressed (i.e., only the Orange LED will be illuminated). 

In order to make sure that the LEDs rotate only a
single position each time SW1 is pressed, two steps are necessary: (1)
you will need to wait for the pushbutton to be released before another
button press could cause the LEDs to rotate 

while (pushbutton depressed){
    delay
}

and (2) you will need to
delay for a short while after you notice that the pushbutton has been
pressed to eliminate false button presses due to contact bounce.

delay()

With 120 MHz clock, each loop is 4 cycles * (1/120,000,000) seconds cycle time.

 */

#include <stdio.h>
#include "led.h"
#include "pushbutton.h"
#include "delay.h"
#include "toggle.h"

int main(void) {
	
	printf("Toggle Project Starting\n");
	
	/* Initialize all of the LEDs */
	ledInitAll();
	/* Initialize both of the pushbuttons */
	pushbuttonInitAll();

    int counter = 0;
    int curr;
    
    ledOrangeOn();
    delay(HALF_SECOND/20);

	while(1) {
        if(sw1In()) {
            counter += 1;
            while (sw1In()){
                delay(HALF_SECOND/100);
            }
		}
        else if(sw2In()) {
            counter -= 1;
            while (sw2In()){
                delay(HALF_SECOND/100);
            }
		}
        if (counter < 0){
            curr = (-1 * counter) % 4;
        }
        else{
            curr = counter % 4;
        }
        switch (curr)
        {
        case orange:
            ledBlueOff();
            ledYellowOff();
            ledOrangeOn();
            break;
        case yellow:
            ledOrangeOff();
            ledGreenOff();
            ledYellowOn();
            break;
        case green:
            ledYellowOff();
            ledBlueOff();
            ledGreenOn();
            break;
        case blue:
            ledGreenOff();
            ledOrangeOff();
            ledBlueOn();
            break;
        default:
            break;
        }
        delay(HALF_SECOND/100);
    }
}
