/*This file handles the LED display.*/

#include "app.h"
#include "leds.h"
#include "i2c.h"
#include "system_config.h"
#include <math.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "master_control.h"

static uint16_t u16_blue_LED_duty_cycles[NUM_OF_BLUE_LEDS];
static uint16_t u16BlueLEDDutyCycleBuffer[NUM_OF_BLUE_LEDS];
static uint16_t u16_red_LED_duty_cycle;
static uint32_t u32MenuFuncLEDMask;
static uint8_t u8AlternateFuncFlag = FALSE;
static uint8_t u8AlternateFuncState = INDICATE_MENU;
static uint8_t u8BlinkFlag[NUM_OF_BLUE_LEDS];
static uint8_t u8MenuBlinkState;
static uint8_t u8MenuBlinkTimer;
static uint8_t u8IndicateOverdubModeFlag;
static uint8_t u8IndicateAxesState;
static uint8_t u8IndicateSeqeunceModeFlag;
static uint8_t u8SwitchLEDState = SWITCH_LED_OFF;
static uint8_t u8SwitchLEDBlinkState = FALSE;
static uint8_t u8SwitchBlinkTimer = 0;
static uint8_t u8PowerUpSequenceFlag = TRUE;
static uint8_t u8IndicateErrorFlag = FALSE;
static uint8_t u8IndicateMuteModeFlag = FALSE;

#define MENU_BLINK_TIMER_RESET   60
#define SWITCH_BLINK_TIMER_RESET    25

const uint8_t led_ordered_array[NUM_OF_BLUE_LEDS] = {LED1,
 LED2   ,
 LED3   ,
 LED4   ,
 LED5   ,
 LED6    ,
 LED7   ,
 LED8    ,
 LED9   ,
 LED10    ,
 LED11   ,
 LED12    ,
 LED13    ,
 LED14   ,
 LED15   ,
 LED16    ,
 LED17   ,
 LED18   ,
 LED19   ,
 LED20   };

/*This structure logs where the Blue LEDs are in space with an x,y coordinate*/
const led_parameters led_parameter_array[NUM_OF_BLUE_LEDS] = {
		//X LOCATION , Y LOCATION
		{MIN_LOCATION, THIRD_LOCATION},//0-LED1 - Center of left side
                {THIRD_LOCATION, MAX_LOCATION},//1-LED2 - Center Top
                {FIRST_LOCATION, MIN_LOCATION},//2-LED3 - Bottom Left
                {MAX_LOCATION,FIRST_LOCATION},//3-LED14 - Right Bottom
                {MIN_LOCATION, FOURTH_LOCATION},//4-LED6 - Fourth from Bottom Left
                {FOURTH_LOCATION, MAX_LOCATION},//5-LED12 - Fourth from Left Top
                {SECOND_LOCATION,MIN_LOCATION},//6-LED18 - Second From Left Bottom
                {MAX_LOCATION, SECOND_LOCATION},//7-LED5 - Second from Bottom on Right
                {FIRST_LOCATION, MAX_LOCATION},//8-LED11 - Top Left
                {FIFTH_LOCATION, MAX_LOCATION},//9-LED20 - Top Right
                {THIRD_LOCATION, MIN_LOCATION},//10-LED4 - Center of Bottom
                {MAX_LOCATION, THIRD_LOCATION},//11-LED10 - Center Right
                {FIRST_LOCATION, MAX_LOCATION},//12-LED16 - Left Top
                {MIN_LOCATION, SECOND_LOCATION},//13-LED3 - Second Left Side from Bottom
                {FOURTH_LOCATION,MIN_LOCATION},//14-LED9 - Fourth from Left Bottom
                {MAX_LOCATION, FOURTH_LOCATION},//15-LED15 - Second from top Right
		{SECOND_LOCATION, MAX_LOCATION},//16-LED2 - Second Top Row from Left
		{MIN_LOCATION,FIRST_LOCATION},//17-LED13 - Left Bottom
                {FIFTH_LOCATION, MIN_LOCATION},//18-LED19 - Bottom Right
		{MAX_LOCATION, FIFTH_LOCATION}//19-LED17 - Right Top
		
};

const uint8_t u8LEDsLeftSideMapping[NUM_LEDS_PER_SIDE] ={
LED20,
LED19,
LED18,
LED17,
LED16
};

const uint8_t u8LEDsTopSideMapping[NUM_LEDS_PER_SIDE] = {
LED1,
LED2,
LED3,
LED4,
LED5
};

const uint8_t u8LEDsRightSideMapping[NUM_LEDS_PER_SIDE] = {
LED6,
LED7,
LED8,
LED9,
LED10
};

const uint8_t u8LEDsBottomSideMapping[NUM_LEDS_PER_SIDE] = {
LED11,
LED12,
LED13,
LED14,
LED1
};


enum{
    RAMP_UP = 0,
    RAMP_DOWN
};

#define POWER_UP_TIMER_RESET    25


void updateBlueLEDs(uint8_t u8Index);
void setBlueLEDBrightness(uint8_t u8LEDIndex, uint16_t u16NewBrightness);
void BlinkSwitchLED(void);
void runIndicateOverdubMode(void);
void runIndicateSequencesMode(void);
void runIndicateMuteMode(void);

void ledStateMachine(void){
    static uint8_t u8LEDState;
    static pos_and_gesture_data pos_and_gesture_struct;


    /*Set the Blue LED PWMs*/
    switch(u8LEDState){

        case BLUE_GROUP1:
            SET_SR_CLK;//Clock in a 0
            RESET_LED_TIMER;
            set_led_pwms(BLUE_GROUP1);
            CLEAR_SR_CLK;//Toggle low
            u8LEDState++;
            break;
        case BLUE_GROUP2:
            /*Shift out 0*/
            SET_SR_CLK;//Clock in a 0
            RESET_LED_TIMER;
            set_led_pwms(BLUE_GROUP2);
            CLEAR_SR_CLK;//Toggle low
            u8LEDState++;
            break;
        case BLUE_GROUP3:
            /*Shift out 0*/
            SET_SR_CLK;//Clock in a 0
            RESET_LED_TIMER;
            set_led_pwms(BLUE_GROUP3);
            CLEAR_SR_CLK;//Toggle low
            u8LEDState++;
            break;
        case BLUE_GROUP4:
            SET_SR_CLK;//Clock in a 0
            RESET_LED_TIMER;
            set_led_pwms(BLUE_GROUP4);
            CLEAR_SR_CLK;//Toggle low
            SET_SR_DATA;//Set up to clock in a 1
            u8LEDState++;
                break;
        case BLUE_GROUP5:
            SET_SR_CLK;//Clock in a 1
            RESET_LED_TIMER;
            set_led_pwms(BLUE_GROUP5);
            CLEAR_SR_CLK;//Toggle low
            RESET_SR_DATA;
            u8LEDState++;// = BLUE_GROUP1;
                break;
        case UPDATE_LEDS:
            //Turn off the LEDs
            reset_led_pwms();
            /*If there's an update to the values, do it*/
            if(xQueueReceive(xLEDQueue, &pos_and_gesture_struct, 0) &&
            u8AlternateFuncFlag == FALSE && u8IndicateErrorFlag == FALSE){
                convert_position_to_leds(&pos_and_gesture_struct);
            }
            u8LEDState = BLUE_GROUP1;
            break;
    }

    //Run the blinking for the menu LEDs
    if(u8MenuBlinkTimer-- == 0){
        u8MenuBlinkTimer = MENU_BLINK_TIMER_RESET;
        u8MenuBlinkState ^= ON;
       
        if(u8IndicateErrorFlag == TRUE){
            u16_red_LED_duty_cycle = MAX_BRIGHTNESS;
            u8IndicateErrorFlag = FALSE;
        }
    }

    //Run the blinking for the switch LED
    if(u8SwitchBlinkTimer-- == 0){
        //Switch blinking indicates clocks either incoming or outgoing
        //If record is set to external triggering
        /*What I want
         * The switch LED will blink to indicate incoming clock.
         * It could toggle with clocks during recording or just blink for each clock.
         * It will also blink to indicate outgoing clock.
         * It will blink the opposite color on the first beat in a sequence.
         * During record, it will blink green on the first beat and then red thereafter.
         * During playback, it will blink red on the first beat and then green thereafter.
         *
         * When the clock edge either arrives or is generated, the LED will be turned on.
         * The timer will be started.
         * When it runs out the LED will be turned off.
         * Make sure we're in a blinky mode though.
         */
        if(u8SwitchLEDState > SWITCH_LED_RED){      
            if(u8SwitchLEDState > SWITCH_LED_RED_BLINK_ONCE){
                BlinkSwitchLED();
            }
            else{
                SET_SWITCH_LED_OFF;
            }
        }
    }

    if(u8IndicateOverdubModeFlag == TRUE){
        //Indicate active axes
        runIndicateOverdubMode();
    }

    if(u8IndicateSeqeunceModeFlag == TRUE){
        runIndicateSequencesMode();
    }

    if(u8IndicateMuteModeFlag == TRUE){
        runIndicateMuteMode();
    }


    vTaskDelay(39);
}

void runIndicateOverdubMode(void){
    uint8_t u8allOffFlag = TRUE;

    if(getOverdubStatus(X_OUTPUT_INDEX) == TRUE){
        setLeftLEDs(MAX_BRIGHTNESS, ON);
        u8allOffFlag = FALSE;
    }
    else{
        setLeftLEDs(MAX_BRIGHTNESS, OFF);
    }
    if(getOverdubStatus(Y_OUTPUT_INDEX) == TRUE){
        setTopLEDs(MAX_BRIGHTNESS, ON);
        u8allOffFlag = FALSE;
    }
    else{
        setTopLEDs(MAX_BRIGHTNESS, OFF);
    }
    if(getOverdubStatus(Z_OUTPUT_INDEX) == TRUE){
        setRightLEDs(MAX_BRIGHTNESS, ON);
        u8allOffFlag = FALSE;
    }
    else{
        setRightLEDs(MAX_BRIGHTNESS, OFF);
    }

    if(u8allOffFlag == TRUE){
        setRedLEDs(256);
    }
    else{
        turnOffRedLEDs();
    }
}

void setIndicateMuteModeFlag(uint8_t u8NewState){
    u8IndicateMuteModeFlag = u8NewState;
}

void runIndicateMuteMode(void){
    uint8_t u8allOffFlag = TRUE;

    if(getMuteStatus(X_OUTPUT_INDEX) == FALSE){
        setLeftLEDs(MAX_BRIGHTNESS, ON);
        u8allOffFlag = FALSE;
    }
    else{
        setLeftLEDs(MAX_BRIGHTNESS, OFF);
    }
    if(getMuteStatus(Y_OUTPUT_INDEX) == FALSE){
        setTopLEDs(MAX_BRIGHTNESS, ON);
        u8allOffFlag = FALSE;
    }
    else{
       setTopLEDs(MAX_BRIGHTNESS, OFF);
    }
    if(getMuteStatus(Z_OUTPUT_INDEX) == FALSE){
        setRightLEDs(MAX_BRIGHTNESS, ON);
        u8allOffFlag = FALSE;
    }
    else{
        setRightLEDs(MAX_BRIGHTNESS, OFF);
    }

    if(u8allOffFlag == TRUE){
        setRedLEDs(256);
    }
    else{
        turnOffRedLEDs();
    }
}

void setIndicateSequenceModeFlag(uint8_t u8NewState){
    u8IndicateSeqeunceModeFlag = u8NewState;
}

void runIndicateSequencesMode(void){
    static uint8_t u8IndicateSequencesState;
    uint8_t u8SelectedSequenceIndex = getSelectedSequenceIndex();

    switch(u8IndicateSequencesState){
        case 0://Indicate position 1
            if(getSequenceModeState(u8IndicateSequencesState) != NO_SEQUENCE_MASK){
                setLeftLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setLeftLEDs(MAX_BRIGHTNESS, OFF);
            }
            setTopLEDs(MAX_BRIGHTNESS, OFF);
            setRightLEDs(MAX_BRIGHTNESS, OFF);
            if(u8MenuBlinkTimer == MENU_BLINK_TIMER_RESET){
                u8IndicateSequencesState++;
            }
            break;
        case 1:
            if(getSequenceModeState(u8IndicateSequencesState) != NO_SEQUENCE_MASK){
                setTopLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setTopLEDs(MAX_BRIGHTNESS, OFF);
            }
            setLeftLEDs(MAX_BRIGHTNESS, OFF);
            setRightLEDs(MAX_BRIGHTNESS, OFF);
            if(u8MenuBlinkTimer == MENU_BLINK_TIMER_RESET){
                u8IndicateSequencesState++;
            }
            break;
        case 2:
            if(getSequenceModeState(u8IndicateSequencesState) != NO_SEQUENCE_MASK){
                setRightLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setRightLEDs(MAX_BRIGHTNESS, OFF);
            }
            setTopLEDs(MAX_BRIGHTNESS, OFF);
            setLeftLEDs(MAX_BRIGHTNESS, OFF);
            if(u8MenuBlinkTimer == MENU_BLINK_TIMER_RESET){
                u8IndicateSequencesState++;
            }
            break;
        case 3:
            if(getSequenceModeState(u8IndicateSequencesState) != NO_SEQUENCE_MASK){
                setRightLEDs(MAX_BRIGHTNESS, ON);
                setTopLEDs(MAX_BRIGHTNESS, ON);
                setLeftLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setRightLEDs(MAX_BRIGHTNESS, OFF);
                setTopLEDs(MAX_BRIGHTNESS, OFF);
                setLeftLEDs(MAX_BRIGHTNESS, OFF);
            }

            if(u8MenuBlinkTimer == MENU_BLINK_TIMER_RESET){
                u8IndicateSequencesState = 0;
            }
            break;
        case 4:
            break;
    }

    //Turn on the LEDs for the selected sequence
    switch(u8SelectedSequenceIndex){
        case 0:
            setLeftLEDs(MAX_BRIGHTNESS, ON);
            break;
        case 1:
            setTopLEDs(MAX_BRIGHTNESS, ON);
            break;
        case 2:
            setRightLEDs(MAX_BRIGHTNESS, ON);
            break;
        case 3:
            setLeftLEDs(MAX_BRIGHTNESS, ON);
            setTopLEDs(MAX_BRIGHTNESS, ON);
            setRightLEDs(MAX_BRIGHTNESS, ON);
            break;
        default:
            break;

    }

}

uint8_t runPowerUpSequence(void){
    static uint8_t u8PowerUpState = RAMP_UP;
    static uint8_t u8BlueLEDCount = 0;
    static uint8_t u8PowerUpTimer = POWER_UP_TIMER_RESET;

    if(u8PowerUpTimer-- == 0){
        setBlueLEDBrightness(led_ordered_array[u8BlueLEDCount], HALF_BRIGHTNESS);
        u8BlueLEDCount++;
        if(u8BlueLEDCount == NUM_OF_BLUE_LEDS){
            setRedLEDs(HALF_BRIGHTNESS);
            return 0;
        }
        u8PowerUpTimer = POWER_UP_TIMER_RESET;
    }

    return 1;
}

/*Indicate that the system encountered an error condition.*/
void LEDIndicateError(void){
    u8IndicateErrorFlag = TRUE;
    turnOffAllLEDs();
    u8MenuBlinkTimer = MENU_BLINK_TIMER_RESET;
}

void setSwitchLEDState(uint8_t u8NewState){
    u8SwitchLEDState = u8NewState;
    switch(u8SwitchLEDState){
        case SWITCH_LED_OFF:
            SET_SWITCH_LED_OFF;
            break;
        case SWITCH_LED_GREEN:
            SET_SWITCH_LED_GREEN;
            break;
        case SWITCH_LED_RED:
            SET_SWITCH_LED_RED;
            break;
        case SWITCH_LED_GREEN_BLINK_ONCE:
            SET_SWITCH_LED_GREEN;
            u8SwitchLEDBlinkState = TRUE;
            u8SwitchBlinkTimer = SWITCH_BLINK_TIMER_RESET;
            break;
        case SWITCH_LED_RED_BLINK_ONCE:
            SET_SWITCH_LED_RED;
            u8SwitchLEDBlinkState = TRUE;
            u8SwitchBlinkTimer = SWITCH_BLINK_TIMER_RESET;
            break;
        case SWITCH_LED_GREEN_BLINKING:
            SET_SWITCH_LED_GREEN;
            u8SwitchLEDBlinkState = TRUE;
            u8SwitchBlinkTimer = SWITCH_BLINK_TIMER_RESET;
            break;
        case SWITCH_LED_RED_BLINKING:
            SET_SWITCH_LED_RED;
            u8SwitchLEDBlinkState = TRUE;
            u8SwitchBlinkTimer = SWITCH_BLINK_TIMER_RESET;
            break;
        case SWITCH_LED_RED_GREEN_ALTERNATING:
            break;
        default:
            break;
    }
}

void BlinkSwitchLED(void){
    u8SwitchLEDBlinkState ^= 1;
    if(u8SwitchLEDState != SWITCH_LED_RED_GREEN_ALTERNATING){
        if(u8SwitchLEDBlinkState == FALSE){
            SET_SWITCH_LED_OFF;
        }
        else{
            if(u8SwitchLEDState == SWITCH_LED_GREEN_BLINKING){
                SET_SWITCH_LED_GREEN;
            }else{
                SET_SWITCH_LED_RED;
            }
        }
    }else{
        if(u8SwitchLEDBlinkState == FALSE){
            SET_SWITCH_LED_RED;
        }
        else{
            SET_SWITCH_LED_GREEN;
        }
    }
}


void setIndicateOverdubModeFlag(uint8_t u8NewSetting){
    u8IndicateOverdubModeFlag = u8NewSetting;
}

void resetBlink(void){
    u8MenuBlinkTimer = MENU_BLINK_TIMER_RESET;
    u8MenuBlinkState = ON;
}

void setLEDAlternateFuncFlag(uint8_t u8NewState){
    u8AlternateFuncFlag =  u8NewState;
}

void setBlueLEDBrightness(uint8_t u8LEDIndex, uint16_t u16NewBrightness){
    u16BlueLEDDutyCycleBuffer[u8LEDIndex] = u16NewBrightness;
}

void setLEDState(uint8_t u8LEDIndex, uint8_t u8NewState){
    
    switch(u8NewState){
        case OFF:
            u16BlueLEDDutyCycleBuffer[u8LEDIndex] = 0;
            u8BlinkFlag[u8LEDIndex] = OFF;
            break;
        case ON:
            u16BlueLEDDutyCycleBuffer[u8LEDIndex] = MAX_BRIGHTNESS;
            u8BlinkFlag[u8LEDIndex] = OFF;
            break;
        case BLINK:
            u16BlueLEDDutyCycleBuffer[u8LEDIndex] = MAX_BRIGHTNESS;
            u8BlinkFlag[u8LEDIndex] = BLINK;
            break;
    }
}

void turnOffAllLEDs(void){
    int i;

    for(i=0;i<NUM_OF_BLUE_LEDS;i++){
        u16BlueLEDDutyCycleBuffer[i] = 0;
        u8BlinkFlag[i] = OFF;
    }

    u16_red_LED_duty_cycle = 0;
}

void turnOffRedLEDs(void){
    u16_red_LED_duty_cycle = 0;
}

void updateBlueLEDs(uint8_t u8Index){
    int i;

    for(i=u8Index; i<(u8Index+NUM_OF_BLUE_PWMS);i++){
        u16_blue_LED_duty_cycles[i] = u16BlueLEDDutyCycleBuffer[i];
    }
}


/*Somehow OC2 AND OC5 HAVE BEEN SWITCHED. DON'T UNDERSTAND HOW OR WHY*/
void set_led_pwms(uint8_t u8Index){
    
    u8Index *= NUM_OF_BLUE_PWMS;
    updateBlueLEDs(u8Index);

 
    if(u8BlinkFlag[u8Index] != BLINK || u8MenuBlinkState == ON){
        PLIB_OC_PulseWidth16BitSet( LED_PWM1_OC_ID, u16_blue_LED_duty_cycles[u8Index]);
    }else{
        PLIB_OC_PulseWidth16BitSet( LED_PWM1_OC_ID, 0);
    }

    u8Index++;
    if(u8BlinkFlag[u8Index] != BLINK || u8MenuBlinkState == ON){
        PLIB_OC_PulseWidth16BitSet( LED_PWM3_OC_ID, u16_blue_LED_duty_cycles[u8Index]);
    }else{
        PLIB_OC_PulseWidth16BitSet( LED_PWM3_OC_ID, 0);
    }

    u8Index++;
    if(u8BlinkFlag[u8Index] != BLINK || u8MenuBlinkState == ON){
        PLIB_OC_PulseWidth16BitSet( LED_PWM4_OC_ID, u16_blue_LED_duty_cycles[u8Index]);
    }else{
        PLIB_OC_PulseWidth16BitSet( LED_PWM4_OC_ID, 0);
    }

    u8Index++;
    if(u8BlinkFlag[u8Index] != BLINK || u8MenuBlinkState == ON){
        PLIB_OC_PulseWidth16BitSet( LED_PWM5_OC_ID, u16_blue_LED_duty_cycles[u8Index]);
    }else{
        PLIB_OC_PulseWidth16BitSet( LED_PWM5_OC_ID, 0);
    }


    PLIB_OC_PulseWidth16BitSet( LED_PWM2_OC_ID, u16_red_LED_duty_cycle);

    
}


void reset_led_pwms(void){
    PLIB_OC_PulseWidth16BitSet( LED_PWM1_OC_ID, 0);// u16_blue_LED_duty_cycles[u8Index]);
    PLIB_OC_PulseWidth16BitSet( LED_PWM2_OC_ID, 0);//u16_blue_LED_duty_cycles[u8Index]);
    PLIB_OC_PulseWidth16BitSet( LED_PWM3_OC_ID, 0);//u16_blue_LED_duty_cycles[u8Index]);
    PLIB_OC_PulseWidth16BitSet( LED_PWM4_OC_ID, 0);//u16_blue_LED_duty_cycles[u8Index]);
    PLIB_OC_PulseWidth16BitSet( LED_PWM5_OC_ID, 0);// u16_red_LED_duty_cycle);
}

void clear_led_shift_registers(void){
    uint8_t i;

    RESET_SR_DATA;

    for(i=0;i<NUM_OF_BLUE_LEDS;i++){
        TOGGLE_SR_CLK;
       // vTaskDelay(1*TICKS_PER_MS);
        TOGGLE_SR_CLK;
      //  vTaskDelay(1*TICKS_PER_MS);
    }

}

void setRedLEDs(uint16_t u16Brightness){
    u16_red_LED_duty_cycle = u16Brightness;
}

void setLeftLEDs(uint16_t u16LEDBrightness, uint8_t u8NewState){
//    int i;
//    for(i = 0; i < NUM_LEDS_PER_SIDE; i++ ){
//        setBlueLEDBrightness(u8LEDsLeftSideMapping[i], u16LEDBrightness);
//        setLEDState(u8LEDsLeftSideMapping[i], u8NewState);
//    }
    setBlueLEDBrightness(u8LEDsLeftSideMapping[2], u16LEDBrightness);
    setLEDState(u8LEDsLeftSideMapping[2], u8NewState);
}
void setTopLEDs(uint16_t u16LEDBrightness, uint8_t u8NewState){
//    int i;
//    for(i = 0; i < NUM_LEDS_PER_SIDE; i++ ){
//        setBlueLEDBrightness(u8LEDsTopSideMapping[i], u16LEDBrightness);
//        setLEDState(u8LEDsTopSideMapping[i], u8NewState);
//    }
    setBlueLEDBrightness(u8LEDsTopSideMapping[2], u16LEDBrightness);
    setLEDState(u8LEDsTopSideMapping[2], u8NewState);
}
void setRightLEDs(uint16_t u16LEDBrightness, uint8_t u8NewState){
//    int i;
//    for(i = 0; i < NUM_LEDS_PER_SIDE; i++){
//        setBlueLEDBrightness(u8LEDsRightSideMapping[i], u16LEDBrightness);
//        setLEDState(u8LEDsRightSideMapping[i], u8NewState);
//    }
    setBlueLEDBrightness(u8LEDsRightSideMapping[2], u16LEDBrightness);
    setLEDState(u8LEDsRightSideMapping[2], u8NewState);
}

void setBottomLEDs(uint16_t u16LEDBrightness, uint8_t u8NewState){
    setBlueLEDBrightness(u8LEDsBottomSideMapping[2], u16LEDBrightness);
    setLEDState(u8LEDsRightSideMapping[2], u8NewState);
}


/*This function is used to test the LED algorithm*/
void simulate_movement(pos_and_gesture_data * p_and_g_struct){
    static uint16_t 	u16_XPosition,
                        u16_YPosition,
                        u16_ZPosition;
    static uint8_t      u8State;
    
    /*Make a square*/
    switch(u8State){
        case 0:
            u16_YPosition = 1;
            u16_ZPosition = 1;
            u16_XPosition += 256;
            if(u16_XPosition >= MAX_LOCATION-256){
                u8State++;
            }
            break;
        case 1:
            u16_YPosition += 256;
            if(u16_YPosition >= MAX_LOCATION-256){
                u8State++;
            }
            break;
        case 2:
            u16_XPosition -= 256;
            if(u16_XPosition <= 256){
                u8State++;
            }
            break;
        case 3:
            u16_YPosition -= 256;
            if(u16_YPosition <= 256){
                u8State = 0;
            }
            break;
        case 4:
            break;
        case 5:
            break;
        case 6:
            break;
            
        
    }
    p_and_g_struct->u16XPosition = u16_XPosition;
    p_and_g_struct->u16YPosition = u16_YPosition;
    p_and_g_struct->u16ZPosition = u16_ZPosition;
    
}


void convert_position_to_leds(pos_and_gesture_data * p_and_g_struct){
    uint8_t u8_index;
    int i32_XDistance,
                    i32_YDistance,
                    i32_BrightnessDifference;
    uint32_t u32_result;
    uint16_t 	u16_XPosition,
                u16_YPosition,
                u16_ZPosition,
                u16_XLED_Location,
                u16_YLED_Location;

    //Make local copies of the position variables
    u16_XPosition = p_and_g_struct->u16XPosition;
    u16_YPosition = p_and_g_struct->u16YPosition;
    u16_ZPosition = p_and_g_struct->u16ZPosition;

    if(u16_XPosition != 0 && u16_YPosition != 0){
        //Step through the blue LEDs determining brightness by the relative location of the LED to the hand position
        for(u8_index = 0; u8_index < NUM_OF_BLUE_LEDS; u8_index++){

            u16_XLED_Location = led_parameter_array[u8_index].u16_X_position;
            u16_YLED_Location = led_parameter_array[u8_index].u16_Y_position;

            //If the hand is too far away, the brightness is 0.
            //Otherwise, it's calculated
            i32_XDistance = abs(u16_XPosition - u16_XLED_Location);
            i32_YDistance = abs(u16_YPosition - u16_YLED_Location);
            if((i32_XDistance < DISTANCE_LIMIT) &&(i32_YDistance < DISTANCE_LIMIT)		){
                //Calculate the length of the vector = calculate the hypotenuse.
                //u64_result = 0;
                u32_result = i32_XDistance * i32_XDistance;
                u32_result += i32_YDistance * i32_YDistance;
                u32_result = sqrt(u32_result);
                //We want the inverse of the hypotenuse to get how close instead of how far
                if(u32_result < DISTANCE_LIMIT){
                    u32_result = DISTANCE_LIMIT - u32_result;
                    //Scale it according to the maximum brightness
                    u32_result *= MAX_BRIGHTNESS;
                    u32_result /= DISTANCE_LIMIT;

                    u16BlueLEDDutyCycleBuffer[u8_index] = (uint16_t) u32_result;
                }
                else{
                        u16BlueLEDDutyCycleBuffer[u8_index] = 0;
                }
            }else{
            u16BlueLEDDutyCycleBuffer[u8_index] = 0;
        }
    }
    }
    else{
            for(u8_index = 0; u8_index < NUM_OF_BLUE_LEDS; u8_index++){
                    u16_blue_LED_duty_cycles[u8_index] = 0;
            }
    }

   // if(u16_ZPosition != 0){
            u16_red_LED_duty_cycle = (u16_ZPosition*MAX_BRIGHTNESS/MAX_LOCATION);
   // }
}