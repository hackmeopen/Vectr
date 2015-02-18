/* 
 * File:   leds.h
 * Author: matthewheins
 *
 * Created on March 2, 2014, 7:59 PM
 */

#ifndef LEDS_H
#define	LEDS_H

#include <stdint.h>
#include "app.h"

typedef struct{
	uint16_t u16_X_position;
	uint16_t u16_Y_position;
}led_parameters;

/*LED numbering doesn't match the board because they're multiplexed.
 * I renumbered them to make easy sense to me. They start with 1 as the left LED on the
 * top and 2 is clockwise from there. The indexes then match the indexing in the
 * LED parameter array.
 * This definition makes them easily accessible by the menu routine
 */
#define LED18   0
#define LED3    1
#define LED16   17
#define LED10   3
#define LED19   4
#define LED4    5
#define LED14   6
#define LED9    7
#define LED20   8
#define LED6    19
#define LED13   10
#define LED8    11
#define LED1    12
#define LED17   13
#define LED12   14
#define LED7    15
#define LED2    16
#define LED15   2
#define LED11   18
#define LED5    9


#define MIN_LOCATION		0
#define FIRST_LOCATION          0x1999   
#define SECOND_LOCATION         0x4CCB
#define THIRD_LOCATION          0x7FFF
#define FOURTH_LOCATION         0x9996
#define FIFTH_LOCATION          0xE661
#define MAX_LOCATION		0xFFFF
#define ONE_THIRD_LOCATION	0x3333
#define TWO_THIRDS_LOCATION	0xCCCC

#define MID_LOCATION		0x7FFF
#define DISTANCE_LIMIT		0x9C40

#define NUM_OF_BLUE_LEDS	20
#define NUM_OF_BLUE_PWMS	4
#define NUM_LEDS_PER_SIDE           5


#define MAX_LED_SLEW_RATE	50


/*LED Shift Register States*/
enum{
	BLUE_GROUP1=0,
	BLUE_GROUP2,
	BLUE_GROUP3,
	BLUE_GROUP4,
        BLUE_GROUP5,
	UPDATE_LEDS
};

//Alternate LED Function State
enum{
    INDICATE_MENU = 0
};

enum{
    OFF = 0,
    ON,
    BLINK
};

enum{
    SWITCH_LED_OFF,
    SWITCH_LED_GREEN,
    SWITCH_LED_RED,
    SWITCH_LED_GREEN_BLINK_ONCE,
    SWITCH_LED_RED_BLINK_ONCE,
    SWITCH_LED_GREEN_BLINKING,
    SWITCH_LED_RED_BLINKING,
    SWITCH_LED_RED_GREEN_ALTERNATING
};

extern const led_parameters led_parameter_array[NUM_OF_BLUE_LEDS];

void convert_position_to_leds(pos_and_gesture_data * p_and_g_struct);
void setRedLEDs(uint16_t u16Brightness);
void reset_led_pwms(void);
void clear_led_shift_registers(void);
void simulate_movement(pos_and_gesture_data * p_and_g_struct);
void ledStateMachine(void);
void set_led_pwms(uint8_t u8Index);
void turnOffAllLEDs(void);
void turnOffRedLEDs(void);
void setLEDState(uint8_t u8LEDIndex, uint8_t u8NewState);
void resetBlink(void);
void setLeftLEDs(uint16_t u8LEDBrightness, uint8_t u8NewState);
void setTopLEDs(uint16_t u8LEDBrightness, uint8_t u8NewState);
void setRightLEDs(uint16_t u8LEDBrightness, uint8_t u8NewState);
void setBottomLEDs(uint16_t u16LEDBrightness, uint8_t u8NewState);
void setIndicateOverdubModeFlag(uint8_t u8NewSetting);
void setSwitchLEDState(uint8_t u8NewState);
void LEDIndicateError(void);
uint8_t runPowerUpSequence(void);


#endif	/* LEDS_H */

