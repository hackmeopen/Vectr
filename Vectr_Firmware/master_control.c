#include "system_config.h"
#include "master_control.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "app.h"
#include "mem.h"
#include "i2c.h"
#include "leds.h"
#include "dac.h"
#include "quantization_tables.h"

//TODO: Test - TRIGA menu setting and function.
//TODO: Test - Change the record input - Use the record input to keep synchronized
//TODO: Test - Add the above behaviors to Overdub as well.
//TODO: Test pass through clock.
//TODO: Test hold behaviors in all modes
//TODO: Test - Make the LED blinking indicate sequencing events - clock pulses.
//TODO: Test - the clock stopping control in overdubbing.
//TODO: Test - Make it so an encoder switch press when the record clock is stopped resets the loop.
//TODO: Test - Fix the behavior to go to hold or live playback, simple turns.
//TODO: Test - what happens when multiple switch presses are made during external sync'ed recording
//TODO: Test - Implement automatic speed tracking to playback.
//TODO: Test - Make sure playback is smooth. There may be some sort of jumps.
//TODO: Test - Second press during TRIGA external recording should be disregarded.
//TODO: Test - Implement time quantization - Make a way to turn it on and off with gestures.
//TODO: Test - Fix the switch blinking for when the record mode is internal
//TODO: Test - Make Z output decay to zero when the hand is taken away.
//TODO: Test - Implement the hand gate output "being recorded" - Fake the playback.
//TODO: Test - Add the new clock sync to muting, sequencing.
//TODO: Test - Make playback start back at the beginning when playback is restarted from live play mode.
//TODO: Test - Tap tempo recording starts one clock late.
//TODO: Test - Tap tempo needs to let me keep hitting the switch.
//TODO: Test - Different clock sequence lengths in all modes. especially with the clock sync stuff.
//TODO: Test - Turn off tap tempo when external recording is enabled.
//TODO: Test - Implement quantized speed changes when record is set to external.
//TODO: Test - Deal with all the playback modes with the new clocks.
//TODO: Test - Hand Gate not working in overdub

//TODO: Test - Deal with clock sync during flash playback.
//TODO: Test- Add tap tempo to playback. make it so the clock can keep running and update to new speeds. Maybe during tap tempo the speed averaging the handle clock
// routine doesn't work.
//TODO: Test - Quantized external speed changes not working right. Maybe double the clock
//TODO: Test - Get back to one clock number.
//TODO: Test - Implement clock divisions to do the quantized speed changes. Deal with the clock trigger in external mode?
//TODO: Test - Separate out the LED blinking in playback so it can be called from the timer routine.
//TODO: TEST- Change major to minor in the quantization.
//TODO: Test - Deal with gated modes and clock.
//TODO: Test external clocking with sequencing mode.
//TODO: Test - Handle record clock during air scratching.
//TODO: Test - Check clocks with effects.


//TODO: Negative range doesnt go all the way to -5V
//TODO: Figure out what it takes to update the MGC3130 library
//TODO: Specify a solution for microchip demos
//TODO: Make LEDs dim or add screen saver mode when inactive for 5 minutes.


#define MENU_MODE_GESTURE           MGC3130_DOUBLE_TAP_BOTTOM
#define OVERDUB_MODE_GESTURE        MGC3130_DOUBLE_TAP_CENTER
#define MUTE_MODE_GESTURE         MGC3130_DOUBLE_TAP_RIGHT
#define SEQUENCER_MODE_GESTURE      MGC3130_DOUBLE_TAP_LEFT
#define AIR_SCRATCH_MODE_GESTURE    MGC3130_DOUBLE_TAP_TOP

#define X_TOUCH         MGC3130_TOUCH_LEFT
#define Y_TOUCH         MGC3130_TOUCH_TOP
#define Z_TOUCH         MGC3130_TOUCH_RIGHT
#define BOTTOM_TOUCH    MGC3130_TOUCH_BOTTOM
#define CENTER_TOUCH    MGC3130_TOUCH_CENTER

#define GESTURE_DEBOUNCE_TIMER_RESET    50

static VectrDataStruct VectrData;
static VectrDataStruct * p_VectrData;

static uint8_t u8KeyPressFlag = FALSE;//Main switch pressed
static uint8_t u8EncKeyPressFlag = FALSE;//Encoder switch pressed
static uint8_t u8MenuModeFlag = FALSE;
static uint8_t u8TouchFlag = FALSE;
static uint8_t u8SequenceRecordedFlag = FALSE;
static uint8_t u8PlaybackRunFlag = STOP;//Is playback running or not?
static uint8_t u8RecordRunFlag = FALSE;
static uint8_t u8OverdubRunFlag = FALSE;
static uint8_t u8PerformanceModeFlag = NO_PERFORMANCE_ACTIVE;
static uint8_t u8GestureFlag = MGC3130_NO_TOUCH;
static uint8_t u8RecordingArmedFlag = FALSE;
static uint8_t u8PlaybackArmedFlag = FALSE;
static uint8_t u8OverdubArmedFlag = FALSE;
static uint8_t u8LivePlayActivationFlag = FALSE;
static uint8_t u8HandPresentFlag = FALSE;
static uint8_t u8GatePulseFlag = FALSE;
static uint8_t u8ClockPulseFlag = FALSE;
static uint8_t u8PulseExpiredFlag = FALSE;
static uint8_t u8QuantizationGateFlag = FALSE;
static uint8_t u8HandHoldFlag = FALSE;
static uint8_t u8HoldState = OFF;
static uint8_t u8MemCommand;
static uint8_t u8HoldActivationFlag = NO_HOLD_EVENT;
static uint8_t u8SyncTrigger = NO_TRIGGER;
static uint8_t u8SequenceModeChangeSequenceFlag = FALSE;
static uint8_t u16ModulationOnFlag = FALSE;
static uint8_t u8ResetFlag = FALSE;
static uint8_t u8AirScratchRunFlag = FALSE;
static uint8_t u8OperatingMode;
static uint8_t u8PreviousOperatingMode;

static pos_and_gesture_data pos_and_gesture_struct;
static pos_and_gesture_data * p_mem_pos_and_gesture_struct;
static pos_and_gesture_data * p_overdub_pos_and_gesture_struct;
static pos_and_gesture_data hold_position_struct;
static pos_and_gesture_data time_quantize_struct;

static uint8_t u8BufferDataCount = 0;

static memory_data_packet memBuffer;
static memory_data_packet overdubBuffer;

static uint8_t u8SequenceModeIndexes[MAX_NUM_OF_SEQUENCES];
uint8_t u8SequenceModeSelectedSequenceIndex;

static uint8_t u8NumOfClockPulses;
static uint32_t u32NextClockPulseIndex;

static uint16_t u16LastScratchPosition;
static uint16_t u16CurrentScratchSpeedIndex;
static uint32_t u32AirScratchPlaybackSpeed;

static uint8_t u8TapTempoModeActiveFlag;
static uint8_t u8TapTempoKeyPressFlag;
#define LENGTH_OF_TAP_TEMPO_ARRAY   3
static uint32_t u32TapTempoArray[LENGTH_OF_TAP_TEMPO_ARRAY];
static uint8_t u8TapTempoArrayIndex;
static uint8_t u8TapTempoSetFlag;
static uint8_t u8InitTapTempoFlag;
static uint8_t u8ClockTriggerFlag;//Set by the Timer clock routine in app.c TIM3 to inform that the clock expired.

static uint32_t u32NewPlaybackSpeedClocked;
static uint8_t u8NewPlaybackSpeedClockedFlag;
static uint32_t u32NewClockClicks;
static uint8_t u8ExternalAirWheelActiveFlag;
static uint8_t u8ExternalAirWheelActiveSyncFlag;
static uint8_t u8ExternalAirwheelClockDelayCount;
static uint8_t u8ExternalAirwheelClockDelay;

#define EXTERNAL_AIR_WHEEL_MIN_CHANGE_INCREMENT 32


static uint8_t u8MultiTapTriggerTimer;
#define MULTI_TAP_TIMER_RESET   50
uint8_t u8decodeMultiTapGestureState;
enum{
    WAITING_FOR_GESTURE = 0,
    FIRST_TAP_TEMPO_GESTURE_RECEIVED,
    WAITING_FOR_SECOND_TAP_TEMPO_GESTURE,
    FIRST_TIME_QUANT_GESTURE_RECEIVED
};

#define LENGTH_OF_INPUT_CLOCK_ARRAY     4
#define LOG_LENGTH_INPUT_CLOCK_ARRAY    2
#define MARGIN_BETWEEN_EXPECTED_CLOCKS_AND_AVERAGE  64

static uint8_t u8CurrentInputClockCount;
static uint8_t u8ClockLengthOfRecordedSequence;
static uint32_t u32TargetNumTicksBetweenClocks;//The length of time expected between clock pulses
static uint32_t u32NumTicksSinceLastClock;
static uint32_t u32NumTicksBetweenClocksArray[LENGTH_OF_INPUT_CLOCK_ARRAY];
static uint32_t u32AvgNumTicksBetweenClocks;
static uint32_t u32ExpectedNumTicksBetweenClocks;
static uint8_t u8CurrentClockArrayIndex;

typedef struct{
    uint8_t u8GestureFlag;
    uint8_t u8OverdubActiveFlag;
    uint8_t u8AirScratchFlag;
    uint8_t u8SequencingFlag;
}MasterFlags;

static MasterFlags Flags;

#define STANDARD_PLAYBACK_SPEED     SAMPLE_TIMER_PERIOD //199999
#define SIZE_OF_ADJUSTMENT_ARRAY 6
#define SPEED_ADJUSTMENT_STEP_SIZE  64 //NUMBER OF STEPS PER SHIFTING LEVEL
#define SPEED_ADJUSTMENT_STEP_SHIFT 6
#define SPEED_ADJUSTMENT_INCREMENT   1000
#define MINIMUM_SAMPLE_PERIOD   SAMPLE_TIMER_PERIOD>>3 //8x faster
#define MAXIMUM_SAMPLE_PERIOD   SAMPLE_TIMER_PERIOD<<4 //16x slower
static uint32_t u32PlaybackSpeed = STANDARD_PLAYBACK_SPEED;//Number of speed increments away from standard speed
static int16_t i16PlaybackData = 0;
static uint16_t u16PlaybackSpeedTableIndex = INITIAL_SPEED_INDEX;

#define Z_OUTPUT_DECAY_RATE 400 //How many counts per cycle will the Z decay at.

const uint16_t u16PrescaleValues[8] = {TMR_PRESCALE_VALUE_1,
TMR_PRESCALE_VALUE_2,
TMR_PRESCALE_VALUE_4,
TMR_PRESCALE_VALUE_8,
TMR_PRESCALE_VALUE_16,
TMR_PRESCALE_VALUE_32,
TMR_PRESCALE_VALUE_64,
TMR_PRESCALE_VALUE_256};

void slewPosition(pos_and_gesture_data * p_pos_and_gesture_struct);
void scaleRange(pos_and_gesture_data * p_pos_and_gesture_struct);
uint8_t decodeXYZTouchGesture(uint16_t u16Data);
uint8_t decodeDoubleTapGesture(uint16_t u16Data);
void indicateActiveAxes(uint8_t u8State);
void adjustSpeed(int16_t i16AirwheelData);
void adjustSpeedTable(int16_t i16AirwheelData);
void resetSpeed(void);
void switchStateMachine(void);
void armRecording(void);
void armPlayback(void);
void armOverdub(void);
void disarmOverdub(void);
void startNewRecording(void);
void finishRecording(void);
void setHoldPosition(pos_and_gesture_data * p_pos_and_gesture_struct);
void quantizePosition(pos_and_gesture_data * p_pos_and_gesture_struct);
void linearizePosition(pos_and_gesture_data * p_pos_and_gesture_struct);
void gateHandler(pos_and_gesture_data * p_pos_and_gesture_struct);
void holdHandler(pos_and_gesture_data * p_position_data_struct, pos_and_gesture_data * p_memory_data_struct,
pos_and_gesture_data * p_hold_data_struct);
uint16_t scaleBinarySearch(const uint16_t *p_scale, uint16_t u16Position, uint8_t u8Length);
void runPlaybackMode(uint8_t u8RecordTrigger /* = 0 */);
void mutePosition(pos_and_gesture_data * p_pos_and_gesture_struct);
void runModulation(pos_and_gesture_data * p_pos_and_gesture_struct);
void adjustSpeedModulation(uint16_t u16NewValue);
void setNumberOfClockPulses(void);
uint32_t calculateRampOutput(void);
void runAirScratchMode(pos_and_gesture_data * p_pos_and_gesture_struct);
void calculateClockTimer(uint32_t u32Speed);
void enterAirScratchMode(void);
uint16_t scaleSearch(const uint16_t *p_scale, uint16_t u16Position, uint8_t u8Length);
void resetInputClockHandling(void);
uint8_t handleClock(void);
uint32_t calculateAvgNumClicksBetweenClocks(void);
void resetTapTempoMode(void);
void runTapTempo(void);
uint32_t calculateTapTempo(void);
void resetMultiTapGestureDetection(void);
uint8_t decodeMultiTapGestures(uint16_t u16TouchData);
void handleTimeQuantization(pos_and_gesture_data * pos_and_gesture_struct, uint8_t u8ClockTrigger, uint8_t u8RecordTrigger);
void handleZDecay(pos_and_gesture_data * pos_and_gesture_struct);
void syncLoop(void);
void setAvgNumClicksBetweenClocks(uint32_t u32NewNumClicks);

/*Reset all the variables related to the input clock measurements.*/
void resetInputClockHandling(void){
    int i;

    u8CurrentInputClockCount = 0;
    u32TargetNumTicksBetweenClocks = 0;
    u32NumTicksSinceLastClock = 0;
    u32AvgNumTicksBetweenClocks = 0;
    u8CurrentClockArrayIndex = 0;

    for(i=0; i < LENGTH_OF_INPUT_CLOCK_ARRAY; i++ ){
        u32NumTicksBetweenClocksArray[i] = 0;
    }
}

#define LENGTH_OF_LOG  200
uint16_t  u16LogIndex = 0;
uint32_t u32LogValues[LENGTH_OF_LOG];
int16_t i16SecondLogValues[LENGTH_OF_LOG];

/*This function handles the record clock input.
 * It counts clock pulses.
 * For recording mode, it counts clock pulses and lets the main master control
 * function know when the number of clock pulses equals a multiple of the
 * desired loop length.
 * For overdubbing, it does the same as recording except that the loop length
 * doesn't change.
 * Playback is like overdubbing except that the LED colors are opposite.
 * And during playback, the LED only blinks for clock when playback is running.
 *
 * We're also concerned with staying locked to the input clock. To do this
 * we count using timer 3 in app.c. We count timer overruns between clock
 * pulses. From this, we can tell when the clock has stopped and when the clock
 * has changed speed.
*/
uint8_t handleClock(void){
    uint8_t u8modulus;

    /* Keep track of the length of time between clocks. To get the info, we request
     * it from the timer routine and then reset the timer clock to start counting
     * over again. The tracking loops back around gathering over a set number of
     * cycles to be able to average the data.
     * We use this information for a number of reasons.
     * 1. We want to know if the tempo changed. We can then adjust the playback
     * speed to suit the change in tempo.
     * 2. We also want to start and stop the playback based on whether or not the
     */
    if(p_VectrData->u8Source[RECORD] == EXTERNAL || u8TapTempoSetFlag == TRUE){
        u32NumTicksBetweenClocksArray[u8CurrentClockArrayIndex] = getLastRecClockCount();
       // resetRecClockCount();
        u8CurrentClockArrayIndex++;
        if(u8CurrentClockArrayIndex >= LENGTH_OF_INPUT_CLOCK_ARRAY){
            u8CurrentClockArrayIndex = 0;
        }
        
        u32LogValues[u16LogIndex]= u32NumTicksBetweenClocksArray[u8CurrentClockArrayIndex];

        if(u16LogIndex++ > LENGTH_OF_LOG){
            u16LogIndex = 0;
        }

        //With external recording, the clock output mirrors the clock input
        if(u8ExternalAirWheelActiveFlag == FALSE){
            SET_LOOP_SYNC_OUT;
            setClockPulseFlag();//Setting this flag lets the TIM5 routine know to turn the pulse off.
        }
    }

    /*Blink the switch LED to indicate incoming record clock signals.*/
    if(u8OperatingMode == RECORDING){

        handleSwitchLEDClockBlink();
        
        /*Keep track of the number of record input clocks. Know where we are in the loop
         and cycle the loop back around in playback and overdubbing.*/
        u8CurrentInputClockCount++;

        //Check to see if the current count is a multiple of the desired clock length.
        u8modulus = u8CurrentInputClockCount % (1<<VectrData.u8NumClocks);

    }
    else if(u8OperatingMode == PLAYBACK || u8OperatingMode == OVERDUBBING){
        /*This state is for playback or for overdubbing when playback isn't running.*/

        if(u8CurrentInputClockCount == 0 && u8ExternalAirWheelActiveFlag == FALSE){
            if( u8OperatingMode == PLAYBACK ||
                Flags.u8OverdubActiveFlag == FALSE || u8HandPresentFlag == FALSE){
                syncLoop();
            }
        }else if(u8ExternalAirWheelActiveSyncFlag == TRUE){
           
            if(u8ExternalAirwheelClockDelayCount-- == 0){
                syncLoop();
               u8ExternalAirWheelActiveSyncFlag = FALSE;
               u8CurrentInputClockCount = 0;
               if(u8ClockLengthOfRecordedSequence > 1){
                    setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
                }else{
                    setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
                }
            }
        }
        
        if(u8ExternalAirWheelActiveFlag == FALSE){
            handleSwitchLEDClockBlink();
            u8CurrentInputClockCount++;

            if(u8CurrentInputClockCount >= u8ClockLengthOfRecordedSequence){
                u8CurrentInputClockCount = 0;
            }
        }

        /* Calculate the running average of time between clocks.
         * If the result has changed, speed up or slow down the playback.
         */
        if(u8NewPlaybackSpeedClockedFlag != TRUE){

            //calculateAvgNumClicksBetweenClocks();
            setAvgNumClicksBetweenClocks(getLastRecClockCount());
        }else{
            //The speed changed. Need to update.
            u32PlaybackSpeed = u32NewPlaybackSpeedClocked;
            u8NewPlaybackSpeedClockedFlag = FALSE;
            setAvgNumClicksBetweenClocks(u32NewClockClicks);
            SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
            RESET_SAMPLE_TIMER;
            calculateClockTimer(u32PlaybackSpeed);
        }
        
    }
    else{
        //This mode is for Live Play Mode when tap tempo has been activated.
        handleSwitchLEDClockBlink();

        u8CurrentInputClockCount++;

        if(u8CurrentInputClockCount == 1 << p_VectrData->u8NumClocks){
            u8CurrentInputClockCount = 0;
        }
    }

    if(u8modulus == 0){
        return 1;
    }
    else{
        return 0;
    }
}



void handleSwitchLEDClockBlink(void){
    
    if(u8OperatingMode == RECORDING){
         setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
    }else if(u8OperatingMode == OVERDUBBING && Flags.u8OverdubActiveFlag == TRUE
            && u8HandPresentFlag == TRUE){
        if(u8CurrentInputClockCount != 0){
            setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
        }else{
            setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
        }

    }else if(u8OperatingMode == PLAYBACK || u8OperatingMode == SEQUENCING){
        if(u8ExternalAirWheelActiveSyncFlag ==  FALSE){
            if(u8ExternalAirWheelActiveFlag == TRUE){
               u8CurrentInputClockCount++;

               if(u8CurrentInputClockCount >= u8ClockLengthOfRecordedSequence - 1){
                   // u8CurrentInputClockCount = 0;
                    u8ExternalAirWheelActiveSyncFlag = TRUE;
                    u8ExternalAirwheelClockDelayCount = u8ExternalAirwheelClockDelay;
                }
            }


            if(u8CurrentInputClockCount != 0){
                setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
            }else{
                if(u8ClockLengthOfRecordedSequence > 1){
                    setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
                }else{
                    setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
                }
            }
        }

    }else if(u8OperatingMode == OVERDUBBING){
        if(u8ExternalAirWheelActiveFlag == TRUE){
           u8CurrentInputClockCount++;

           if(u8CurrentInputClockCount >= u8ClockLengthOfRecordedSequence){
                u8CurrentInputClockCount = 0;
                u8ExternalAirWheelActiveSyncFlag = TRUE;
            }
        }


        if(u8CurrentInputClockCount != 0){
            if(u8CurrentInputClockCount & 1 && (u8CurrentInputClockCount != u8ClockLengthOfRecordedSequence - 1)){
                setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
            }else{
                setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
            }
        }else{
            if(u8ClockLengthOfRecordedSequence > 1){
                setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
            }else{
                setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
            }
        }
    }else if(u8OperatingMode == LIVE_PLAY){
        //This mode is for Live Play Mode when tap tempo has been activated.
        if(u8CurrentInputClockCount != 0){
            setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
        }else{
            setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
        }
    }else if(u8OperatingMode == AIR_SCRATCHING){
        if(u8AirScratchRunFlag == FALSE){
            if(u8CurrentInputClockCount != 0){
                if(u8CurrentInputClockCount & 1){ //&& (u8CurrentInputClockCount != u8ClockLengthOfRecordedSequence - 1)){
                    setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
                }else{
                    setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
                }
            }else{
                if(u8ClockLengthOfRecordedSequence > 1){
                    setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
                }else{
                    setSwitchLEDState(SWITCH_LED_GREEN_BLINK_ONCE);
                }
            }
        }else{
            setSwitchLEDState(SWITCH_LED_RED_GREEN_ALTERNATING);
        }
    }
}

 //Reset to the loop to be certain that it's staying sync'ed
//Keep the loop sync'ed for all the different playback modes.
/*****SET the RESET OUTPUT for reset gate mode!!!!*/
void syncLoop(void){
    if(getCurrentGateMode() == RESET_GATE){
        SET_GATE_OUT_PORT;
        u8GatePulseFlag = TRUE;
    }
    setClockSyncFlag();
    //If clock is being multiplied, keep it synced
    if(u8ExternalAirWheelActiveFlag == TRUE){
        resetClockTimer();
        resetRecClockCount();
    }
}

void resetTapTempoMode(void){
    int i;

    u8TapTempoArrayIndex = 0;
    u8TapTempoSetFlag = FALSE;
    u8InitTapTempoFlag = TRUE;//Let's us know to start tap tempo with the first press.
    for(i=0; i < LENGTH_OF_TAP_TEMPO_ARRAY; i++ ){
        u32TapTempoArray[i] = 0;
    }
}

void runTapTempo(void){
    int i;
    static uint8_t u8TimeOutTimer = 100;

    /* If the switch has been pressed, log the time between steps.
     * If it's the first press, then start the record timer.
     */
    if(u8TapTempoKeyPressFlag == TRUE){
        if(u8TapTempoSetFlag == FALSE){
            
            if(u8InitTapTempoFlag == TRUE){
                setClockEnableFlag(TRUE);
                resetRecClockCount();
                resetClockTimer();
                u8InitTapTempoFlag = FALSE;
                setClockEnableFlag(TRUE);
                START_CLOCK_TIMER;
                u8CurrentInputClockCount = 0;
                setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
            }else{
                u32TapTempoArray[u8TapTempoArrayIndex] = getRecClockCount();
                resetRecClockCount();
                setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
                if(u8TapTempoArrayIndex == LENGTH_OF_TAP_TEMPO_ARRAY - 1){                   
                    u8TapTempoArrayIndex = 0;
                    setClockTimerTriggerCount(calculateTapTempo());
                    u8TimeOutTimer = 100;
                    u8TapTempoSetFlag = TRUE;
                    resetClockTimer();
                }
                u8CurrentInputClockCount++;
                u8TapTempoArrayIndex++;
            }

            SET_LOOP_SYNC_OUT;
            setClockPulseFlag();//Setting this flag lets the TIM5 routine know to turn the pulse off.
        }
        else{
            
            //We have a complete array.
            u32TapTempoArray[u8TapTempoArrayIndex] = getRecClockCount();
            resetRecClockCount();
            setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
            if(u8TapTempoArrayIndex == LENGTH_OF_TAP_TEMPO_ARRAY - 1){
                u8TapTempoArrayIndex = 0;
            }
            u8TimeOutTimer = 100;
            u8TapTempoArrayIndex++;
            u8CurrentInputClockCount++;
         //   resetClockTimer();
            setClockTimerTriggerCount(calculateTapTempo());
//            SET_LOOP_SYNC_OUT;
//            setClockPulseFlag();//Setting this flag lets the TIM5 routine know to turn the pulse off.
        }
        
        u8TapTempoKeyPressFlag = FALSE;
    }else if(u8ClockTriggerFlag == TRUE){
        //If tap tempo is still active and the clock edge comes before a button
        //press, then there are clock type things to do.
        setSwitchLEDState(SWITCH_LED_RED_BLINK_ONCE);
        SET_LOOP_SYNC_OUT;
        setClockPulseFlag();//Setting this flag lets the TIM5 routine know to turn the pulse off.
        u8CurrentInputClockCount++;
        u8ClockTriggerFlag = FALSE;
    }

    if(u8TapTempoSetFlag == TRUE){
        if(u8TimeOutTimer-- == 0){
            u8TapTempoModeActiveFlag = FALSE;
        }
    }
}

uint8_t getTapTempoSetFlag(void){
    return u8TapTempoSetFlag;
}

void setClockTriggerFlag(void){
    u8ClockTriggerFlag = TRUE;
}

uint32_t calculateTapTempo(void){
    int i;
    uint32_t u32Result = 0;
    for(i=0; i < LENGTH_OF_TAP_TEMPO_ARRAY; i++ ){
        u32Result += u32TapTempoArray[i];
    }

    u32Result /= LENGTH_OF_TAP_TEMPO_ARRAY;
    return u32Result;
}

void setAvgNumClicksBetweenClocks(uint32_t u32NewNumClicks){
    u32AvgNumTicksBetweenClocks = u32NewNumClicks;
 //   u32TargetNumTicksBetweenClocks = u32NewNumClicks;
    u32ExpectedNumTicksBetweenClocks = u32NewNumClicks + MARGIN_BETWEEN_EXPECTED_CLOCKS_AND_AVERAGE;
}

/* Calculate the average number of timer ticks between clock edges. If the recorded sequence
 * is shorter than the array we are using for averaging, then calculate a shorter average.
 * The function returns a 1 if the average number of clocks changes by an amount significant
 * enough to warrant a change in the playback speed.
 */
uint32_t calculateAvgNumClicksBetweenClocks(void){
    int i;
    uint32_t u32Sum = 0;
    uint32_t u32Result = 0;
    uint32_t u32LongestTicks = 0;

    if(u8ClockLengthOfRecordedSequence > LENGTH_OF_INPUT_CLOCK_ARRAY){
        for(i=0;i<LENGTH_OF_INPUT_CLOCK_ARRAY;i++){
            if(u32NumTicksBetweenClocksArray[i] > u32LongestTicks){
                u32LongestTicks = u32NumTicksBetweenClocksArray[i];
            }
            u32Sum += u32NumTicksBetweenClocksArray[i];
        }
        u32Result = u32Sum >> LOG_LENGTH_INPUT_CLOCK_ARRAY;
    }
    else{
        for(i=0;i<u8ClockLengthOfRecordedSequence;i++){
            u32Sum += u32NumTicksBetweenClocksArray[i];
            if(u32NumTicksBetweenClocksArray[i] > u32LongestTicks){
                u32LongestTicks = u32NumTicksBetweenClocksArray[i];
            }
        }
        u32Result = u32Sum / u8ClockLengthOfRecordedSequence;
    }

    u32AvgNumTicksBetweenClocks = u32Result;
    u32ExpectedNumTicksBetweenClocks = u32LongestTicks + MARGIN_BETWEEN_EXPECTED_CLOCKS_AND_AVERAGE;

    return u32AvgNumTicksBetweenClocks;
}



static uint8_t u8SkipClockEdge = FALSE;

/* When the record input is being used to synchronize playback,
 * this function will speed up and slow down the playback clock to keep
 * the number of ticks between clock edges constant.
 */
void regulateClockPlaybackSpeed(void){
    int16_t i16Temp;
    uint8_t u8Change = 0;
    int16_t i16Difference = u32AvgNumTicksBetweenClocks - u32TargetNumTicksBetweenClocks;
    uint32_t u32NewPlaybackSpeed;
    int32_t i32PlaybackSpeedChange;

    /*If the number of clock ticks between edges changes, then we need to speed up or slow
     * down the playback clock to keep the playback speed constant.
     */

    //Too few clock counts - clock needs to run faster. Smaller playback speed is faster.
    //Negative means slower, positive means faster
    if(u8SkipClockEdge == FALSE){
        i32PlaybackSpeedChange = u32PlaybackSpeed * i16Difference;
        i32PlaybackSpeedChange /= (int32_t) u32TargetNumTicksBetweenClocks;
        u32NewPlaybackSpeed = u32PlaybackSpeed + i32PlaybackSpeedChange;
        u32PlaybackSpeed = u32NewPlaybackSpeed;
        SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
        RESET_SAMPLE_TIMER;

        /*Adjust the clock timer accordingly.*/
        calculateClockTimer(u32PlaybackSpeed);
    }else{
        u8SkipClockEdge = FALSE;
    }
}

/*Time quantization
 * If an axis is time quantized, it only changes on a clock edge.
 */
void handleTimeQuantization(pos_and_gesture_data * pos_and_gesture_struct, uint8_t u8ClockTrigger, uint8_t u8RecordTrigger){
    int i;
    uint8_t u8ClockFlag = FALSE;
    uint16_t * p_u16CurrentPosition = &time_quantize_struct.u16XPosition;
    uint16_t * p_u16NewPosition = (uint16_t *) pos_and_gesture_struct;

    if(u8TapTempoSetFlag == TRUE && u8ClockTrigger == TRUE
       || (p_VectrData->u8Source[RECORD] == EXTERNAL && u8RecordTrigger == TRIGGER_WENT_HIGH)){
        u8ClockFlag = TRUE;
    }

    for(i=0;i<NUMBER_OF_OUTPUTS;i++){
        //If time quantization for that axis is on.
        if(p_VectrData->u8TimeQuantization[i] == TRUE){
            //If a clock pulse came in.
            if(!u8ClockFlag){
                //Hold the output til the next clock cycle.
                *(p_u16NewPosition+i) = *(p_u16CurrentPosition + i);
            }
            else{
                //Store the new position value.
                *(p_u16CurrentPosition + i) =  *(p_u16NewPosition+i);
            }
        }
    }

}

void handleZDecay(pos_and_gesture_data * pos_and_gesture_struct){
    if(pos_and_gesture_struct->u16ZPosition > Z_OUTPUT_DECAY_RATE){
       pos_and_gesture_struct->u16ZPosition -= Z_OUTPUT_DECAY_RATE;
    }else{
       pos_and_gesture_struct->u16ZPosition = 0;
    }

    p_VectrData->u16CurrentZPosition = pos_and_gesture_struct->u16ZPosition;
}

void MasterControlInit(void){

    defaultSettings();
    Flags.u8OverdubActiveFlag = FALSE;
    u8HoldState = OFF;
    u8OperatingMode = LIVE_PLAY;
    u8EncKeyPressFlag = FALSE;
    u8MenuModeFlag = FALSE;
    u16ModulationOnFlag = FALSE;
    CLEAR_GATE_OUT_PORT;
    CLEAR_LOOP_SYNC_OUT;
    resetMultiTapGestureDetection();

    if(MODULATION_JACK_PLUGGED_IN){
        u16ModulationOnFlag = TRUE;
    }

    p_VectrData = &VectrData;
    p_mem_pos_and_gesture_struct = &memBuffer.sample_1;
}

/*  Implementing the internally regulated clock + getting the LED blinking during internally sync'ed recording.
 *  Set up a default clock rate of 120BPM
 *  This will get the clock flashing during record. It will change speeds in playback.
 *
 * Set up tap tempo
 *
 */

void MasterControlStateMachine(void){

    static pos_and_gesture_data last_position_data;


    uint16_t u16NumRecordClocks;
    uint8_t u8MessageReceivedFlag = FALSE;
    uint16_t u16TouchData;
    uint16_t u16AirwheelData;
    
    static uint8_t u8GestureDebounceTimer;
    static uint16_t u16LastAirWheelData;
    int16_t i16AirWheelChange;
    uint8_t u8SampleTimerFlag;
    io_event_message event_message;
    uint8_t u8PlayTrigger = NO_TRIGGER;
    uint8_t u8RecordTrigger = NO_TRIGGER;
    uint8_t u8HoldTrigger = NO_TRIGGER;

    u8SyncTrigger = NO_TRIGGER;
    u8HoldActivationFlag = NO_HOLD_EVENT;
    
    //This sample flag is set by the Timer 4/5 32 bit timer. It's timing is controlled by the speed parameter
    u8SampleTimerFlag = xSemaphoreTake(xSemaphoreSampleTimer, 6*TICKS_PER_MS);

    //This queue receives position and gesture data from the I2C algorithm
    if(xQueueReceive(xPositionQueue, &pos_and_gesture_struct, 0)){
        u8MessageReceivedFlag = TRUE;
        last_position_data.u16XPosition = pos_and_gesture_struct.u16XPosition;
        last_position_data.u16YPosition = pos_and_gesture_struct.u16YPosition;
        last_position_data.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
        last_position_data.u16Gesture = pos_and_gesture_struct.u16Gesture;
        last_position_data.u16Touch = pos_and_gesture_struct.u16Touch;
    }
    else{
        u8MessageReceivedFlag = FALSE;
        pos_and_gesture_struct.u16XPosition = last_position_data.u16XPosition;
        pos_and_gesture_struct.u16YPosition = last_position_data.u16YPosition;
        pos_and_gesture_struct.u16ZPosition = last_position_data.u16ZPosition;
        pos_and_gesture_struct.u16Gesture = last_position_data.u16Gesture;
        pos_and_gesture_struct.u16Touch = last_position_data.u16Touch;
    }

    //Run menu mode if it's active. The encoder is either used for menu mode or for
    //live interactions. Can't be both at the same time.
    if(u8MenuModeFlag == TRUE){
        MenuStateMachine();
    }
    else{
        /*Encoder live interaction can cause hold events and reactivate live play.*/
        encoderLiveInteraction();
    }

    /*Gesture debouncing keeps accidental gestures from occurring after the last gesture.*/
    if(u8GestureDebounceTimer > 0){
       u8GestureDebounceTimer--;
    }

    if(u8MultiTapTriggerTimer > 0){
        u8MultiTapTriggerTimer--;
        if(u8MultiTapTriggerTimer == 0){
            resetMultiTapGestureDetection();
        }
    }

    switchStateMachine();//Handle switch presses to change state

    ENABLE_PORT_D_CHANGE_INTERRUPT;

    /*Events on the inputs, Record In, Playback In, Hold In, and Sync In get delivered
      on this queue. They should set flags so the appropriate action can be taken in each
     of the different states.*/
    if(xQueueReceive(xIOEventQueue, &event_message, 0)){
       switch(event_message.u16messageType){
           case RECORD_IN_EVENT:
               u8RecordTrigger = event_message.u16message;
               break;
           case PLAYBACK_IN_EVENT:
               u8PlayTrigger = event_message.u16message;
               break;
           case HOLD_IN_EVENT:
               u8HoldTrigger = event_message.u16message;
               if(u8HoldTrigger == TRIGGER_WENT_HIGH){
                   u8HoldActivationFlag = HOLD_ACTIVATE;
               }else{
                   u8HoldActivationFlag = HOLD_DEACTIVATE;
               }
               break;
           case SYNC_IN_EVENT:
               u8SyncTrigger = event_message.u16message;
               break;
           case JACK_DETECT_IN_EVENT:
               /*If the modulation mode is SCRUB, we disable the clock timer and
                 otherwise, it's enabled.*/
               if(event_message.u16message == 1){
                   u16ModulationOnFlag = FALSE;
                   if(p_VectrData->u8ModulationMode == SCRUB){
                       if(u8PlaybackRunFlag == TRUE){
                            START_CLOCK_TIMER;
                       }
                   }
                   else if(p_VectrData->u8ModulationMode == TRIM){
                       setNewSequenceEndAddress(getActiveSequenceLength());
                   }
               }
               else{
                   u16ModulationOnFlag = TRUE;
                   if(p_VectrData->u8ModulationMode == SCRUB){
                       STOP_CLOCK_TIMER;
                   }
               }
           default:
               break;
       }
    }

    //The following state machine handles the outputs related to each state.
    switch(u8OperatingMode){
        case LIVE_PLAY:

            /*If a hand is over the sensor, a message will be received and managed.*/
            if(u8MessageReceivedFlag == TRUE){
                /*Decode touch and gestures. If a double tap on the bottom occurs,
                 * enter menu mode*/
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){
                    u8GestureDebounceTimer = GESTURE_DEBOUNCE_TIMER_RESET;
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            if(u8MenuModeFlag == FALSE){
                                u8MenuModeFlag = TRUE;
                                menuHandlerInit();
                            }
                            break;
                        case OVERDUB_MODE:
                            //No overdub mode from live play. Too easy to accidentally trigger.
                            break;
                        case MUTE_MODE:
                            //Go to Mute Mode
                            enterMuteMode();
                            break;
                        case SEQUENCER_MODE:
                            //Go to sequencer mode if at least one sequence has been recorded
                            enterSequencerMode();
                            break;
                        case AIR_SCRATCH_MODE:
                            //Go to air scratch mode if a sequence has been recorded.
                            if(u8SequenceRecordedFlag == TRUE){
                                enterAirScratchMode();
                            }
                            else{
                                //indicate error.
                                LEDIndicateError();
                            }
                            break;
                        default:
                            break;
                    }
                }
                else{
                    /*Check for entering tap tempo mode
                     * 1. Tap on the right electrode.
                     * 2. Tap on the left electrode within 100ms.
                     */
                    decodeMultiTapGestures(u16TouchData);
                }
            }

            if(u8HandPresentFlag == FALSE && u8HoldState == OFF){
                u8HoldActivationFlag = HOLD_ACTIVATE;
                u8HandHoldFlag =  TRUE;
            }
            else if(u8HandPresentFlag == TRUE &&  u8HandHoldFlag == TRUE){
                u8HandHoldFlag = FALSE;
                u8HoldActivationFlag = HOLD_DEACTIVATE;
            }         

            /*If we've received a hold command, store the current value.*/
            if(u8HoldActivationFlag == HOLD_ACTIVATE){
                u8HoldState = ON;
                setHoldPosition(&pos_and_gesture_struct);
            }
            else if(u8HoldActivationFlag == HOLD_DEACTIVATE){
                u8HoldState = OFF;
            }
            else if(u8LivePlayActivationFlag == TRUE){
                /*We're already in live play. Just clear the flags.*/
                u8HoldState = OFF;
                u8LivePlayActivationFlag = FALSE;
            }

            /*If hold is active, execute the hold behavior*/
            if(u8HoldState == ON){
                holdHandler(&pos_and_gesture_struct, &pos_and_gesture_struct, &hold_position_struct);

                //If the hand hold is active, decay the Z output.
                if(u8HandHoldFlag == TRUE){
                    handleZDecay(&pos_and_gesture_struct);
                    last_position_data.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    hold_position_struct.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                }
                else{
                  //  u8HandHoldFlag = TRUE;
                    last_position_data.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    hold_position_struct.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                }
            }

            handleTimeQuantization(&pos_and_gesture_struct, u8ClockTriggerFlag, u8RecordTrigger);

            /*If menu mode is active, the menu controls the LEDs.*/
            if(u8MenuModeFlag == FALSE){
                xQueueSend(xLEDQueue, &pos_and_gesture_struct, 0);
            }

            /*Handle time quantization. If an output is time quantized, then
             * it only changes when we get a clock pulse.
             */
            
            slewPosition(&pos_and_gesture_struct);
            gateHandler(&pos_and_gesture_struct);
            mutePosition(&pos_and_gesture_struct);
            linearizePosition(&pos_and_gesture_struct);
            scaleRange(&pos_and_gesture_struct);
            quantizePosition(&pos_and_gesture_struct);

            /*Send the data out to the DAC.*/
            xQueueSend(xSPIDACQueue, &pos_and_gesture_struct, 0);

            /*If tap tempo is actively being set, run the mode*/
            if(u8TapTempoModeActiveFlag == TRUE){
                runTapTempo();
            }

            /*Check for armed playback or recording and the appropriate trigger.*/
            if(u8PlaybackArmedFlag == ARMED){
                if(p_VectrData->u8Control[PLAY] == GATE){
                    //If the playback input is high, start playback.
                    //Don't disarm the flag.
                    if(PLAY_IN_IS_HIGH){
                        u8PlayTrigger = NO_TRIGGER;
                        u8HoldState = OFF;
                        if(u8SequenceRecordedFlag == TRUE){
                            u8OperatingMode = PLAYBACK;
                            u8PlaybackRunFlag = RUN;
                            setRAMRetriggerFlag();
                        }
                    }
                }else{
                    if(u8PlayTrigger == TRIGGER_WENT_HIGH){
                        /*If we received the playback trigger.*/
                        u8PlayTrigger = NO_TRIGGER;
                        u8HoldState = OFF;
                        if(u8SequenceRecordedFlag == TRUE){
                            u8PlaybackArmedFlag = NOT_ARMED;
                            u8OperatingMode = PLAYBACK;
                            u8PlaybackRunFlag = RUN;
                            setRAMRetriggerFlag();
                        }
                    }
                }
            }
            /*Check if it's time to start recording.*/
            if(u8RecordingArmedFlag == ARMED){
                if(p_VectrData->u8Source[RECORD] == EXTERNAL){
                   if(u8RecordTrigger == TRIGGER_WENT_HIGH){/*If we received the record trigger.*/
                        u8HoldState = OFF;
                        u8RecordTrigger = NO_TRIGGER;
                        u8RecordingArmedFlag = NOT_ARMED;
                        startNewRecording();
                   }
                }
                else{
                    //Switch controlled recording with tap tempo starts
                    //recording on a clock edge
                    if(u8ClockTriggerFlag == TRUE){
                        u8RecordingArmedFlag = NOT_ARMED;
                        startNewRecording();
                        u8ClockTriggerFlag = FALSE;
                    }
                }
            }
            else{//Recording not armed.
                if((p_VectrData->u8Source[RECORD] == EXTERNAL && u8RecordTrigger == TRIGGER_WENT_HIGH)
                    ||
                   (u8TapTempoSetFlag == TRUE && u8TapTempoModeActiveFlag == FALSE && u8ClockTriggerFlag == TRUE)){
                    handleClock();
                    u8ClockTriggerFlag = FALSE;
                }
            }
            break;
//RECORDING
        case RECORDING:  
            if(u8MessageReceivedFlag == TRUE){
                /*Decode touch and gestures. If a double tap on the bottom occurs,
                 * enter menu mode*/
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){
                    u8GestureDebounceTimer = GESTURE_DEBOUNCE_TIMER_RESET;
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            if(u8MenuModeFlag == FALSE){
                                u8MenuModeFlag = TRUE;
                                menuHandlerInit();
                            }
                            break;
                        default:
                            break;
                    }
                }
            }


            if(u8RecordRunFlag == TRUE && u8SampleTimerFlag == TRUE && u8HoldState == OFF){
                /*Data is written to and read from the RAM multiple samples at a
                 time, so a buffer must be stored. This stuff looks rough but it removes the memory
                 write from the critical timing path*/
                if(u8BufferDataCount == 0){
                    memBuffer.sample_1.u16XPosition = pos_and_gesture_struct.u16XPosition;
                    memBuffer.sample_1.u16YPosition = pos_and_gesture_struct.u16YPosition;
                    memBuffer.sample_1.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    u8BufferDataCount++;
                }else{
                    memBuffer.sample_2.u16XPosition = pos_and_gesture_struct.u16XPosition;
                    memBuffer.sample_2.u16YPosition = pos_and_gesture_struct.u16YPosition;
                    memBuffer.sample_2.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    u8BufferDataCount = 0;
                    u8MemCommand = WRITE_RAM;
                    xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);//Set up for write
                    xQueueSend(xRAMWriteQueue, &memBuffer, 0);
                }
            }
            else{
                if(u8BufferDataCount == 0){
                    memBuffer.sample_1.u16XPosition = hold_position_struct.u16XPosition;
                    memBuffer.sample_1.u16YPosition = hold_position_struct.u16YPosition;
                    memBuffer.sample_1.u16ZPosition = hold_position_struct.u16ZPosition;
                    u8BufferDataCount++;
                }else{
                    memBuffer.sample_2.u16XPosition = hold_position_struct.u16XPosition;
                    memBuffer.sample_2.u16YPosition = hold_position_struct.u16YPosition;
                    memBuffer.sample_2.u16ZPosition = hold_position_struct.u16ZPosition;
                    u8BufferDataCount = 0;
                    u8MemCommand = WRITE_RAM;
                    xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);//Set up for write
                    xQueueSend(xRAMWriteQueue, &memBuffer, 0);
                }
            }

            if(u8HandPresentFlag == FALSE && u8HoldState == OFF){
                u8HoldActivationFlag = HOLD_ACTIVATE;
                u8HandHoldFlag =  TRUE;
            }
            else if(u8HandPresentFlag == TRUE &&  u8HandHoldFlag == TRUE){
                u8HandHoldFlag = FALSE;
                u8HoldActivationFlag = HOLD_DEACTIVATE;
            }

           /*Check encoder activations. Could go to live play or implement a hold*/
            if(u8HoldActivationFlag == HOLD_ACTIVATE){
                u8HoldState = ON;
                setHoldPosition(&pos_and_gesture_struct);
                u8HoldActivationFlag = FALSE;
            }
            else if(u8HoldActivationFlag == HOLD_DEACTIVATE){
                u8HoldState = OFF;
            }
            else if(u8LivePlayActivationFlag == TRUE){
                finishRecording();
                setSwitchLEDState(OFF);
                u8OperatingMode = LIVE_PLAY;
                u8LivePlayActivationFlag = FALSE;
            }

            /*If hold is active, execute the hold behavior*/
            if(u8HoldState == ON){

                holdHandler(&pos_and_gesture_struct, &pos_and_gesture_struct, &hold_position_struct);

                //If the hand hold is active, decay the Z output.
                if(u8HandHoldFlag == TRUE){
                    handleZDecay(&pos_and_gesture_struct);
                    last_position_data.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    hold_position_struct.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                }
            }

            handleTimeQuantization(&pos_and_gesture_struct, u8ClockTriggerFlag, u8RecordTrigger);
            slewPosition(&pos_and_gesture_struct);
            gateHandler(&pos_and_gesture_struct);
            xQueueSend(xLEDQueue, &pos_and_gesture_struct, 0);
            scaleRange(&pos_and_gesture_struct);
            /*Quantize the position.*/
            quantizePosition(&pos_and_gesture_struct);
            xQueueSend(xSPIDACQueue, &pos_and_gesture_struct, 0);

            /*Check for triggers...
            * If the mode is trigger, look for a rising edge trigger.
             * If the mode is gate, look for a falling edge trigger.
             */

            /* There are a few different behaviors.
             * 
             * Internal and External Recording.
             * For internal recording
             * 
             * Triggered mode will start with the switch press and stop with the next switch press.
             * Gated mode will start with the switch press and end when it's released.
             * 
             * For External recording
             * 
             * Triggered mode will start with the switch press. When the switch is pressed the second time
             * the loop ends when the counted number of input clock edges is a multiple of the current clock 
             * setting.
             * 
             * Gated mode start after a switch press and the clock input goes high.
             * It will end when the switch has been pressed and the clock input is low.
             *
             *
             *When the gate goes low, go straight to playback. Automatically without needing a switch press.
             *For trigger mode, count the number of triggers and then go automatically to playback.
             */

            /*If we're in gate mode, then a low level stops recording. A high level starts recording.
             */
            if(p_VectrData->u8Control[RECORD] == GATE && p_VectrData->u8Source[RECORD] == EXTERNAL){
                if(u8RecordRunFlag == TRUE && u8RecordTrigger == TRIGGER_WENT_LOW){
                    u8RecordRunFlag = FALSE;
                    setSwitchLEDState(SWITCH_LED_RED);
                }
                else if(u8RecordRunFlag == FALSE && u8RecordTrigger == TRIGGER_WENT_HIGH){
                    u8RecordRunFlag = TRUE;
                    setSwitchLEDState(SWITCH_LED_RED_BLINKING);
                }
            }

            /*Check for armed playback or recording and the appropriate trigger.
             *These can only be armed or disarmed if the Source for each is set to external
             */
            if(u8PlaybackArmedFlag == ARMED){
                /*Look for a triggers to start playback*/
                if(p_VectrData->u8Control[PLAY] == GATE){
                    //If the playback input is high, start playback.
                    //Don't disarm the flag.
                    if(PLAY_IN_IS_HIGH){
                        finishRecording();
                        u8PlayTrigger = NO_TRIGGER;
                        if(u8SequenceRecordedFlag == TRUE){
                            u8OperatingMode = PLAYBACK;
                            u8PlaybackRunFlag = RUN;
                        }
                    }
                }else{
                    if(u8PlayTrigger == TRIGGER_WENT_HIGH){
                        /*If we received the playback trigger.*/
                        finishRecording();
                        u8PlayTrigger = NO_TRIGGER;
                        if(u8SequenceRecordedFlag == TRUE){
                            u8PlaybackArmedFlag = NOT_ARMED;
                            u8OperatingMode = PLAYBACK;
                            u8PlaybackRunFlag = RUN;
                        }
                    }
                }
            }

            if(u8RecordingArmedFlag == ARMED){
                if(p_VectrData->u8Source[RECORD] == EXTERNAL){
                   if(u8RecordTrigger == TRIGGER_WENT_HIGH){/*If we received the record trigger.*/
                        u8HoldState = OFF;
                        u8RecordTrigger = NO_TRIGGER;
                        u8RecordingArmedFlag = NOT_ARMED;
                        startNewRecording();
                   }
                }
                else{
                    //Switch controlled recording with tap tempo starts
                    //recording on a clock edge
                    if(u8ClockTriggerFlag == TRUE){
                        u8RecordingArmedFlag = NOT_ARMED;
                        startNewRecording();
                    }
                }
            }
            else if(u8RecordingArmedFlag == DISARMED){
                /*Stop recording
                 * In gate mode, a high->low will stop recording.
                 * In trigger mode, a low->high will stop recording when
                 *
                 */
                if(p_VectrData->u8Source[RECORD] == EXTERNAL){
                    if(p_VectrData->u8Control[RECORD] == TRIGGER){
                        if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                            if(handleClock()){
                                u8RecordingArmedFlag = NOT_ARMED;
                                finishRecording();
                            }
                        }
                    }else{
                        /*Gate control*/
                        if(!REC_IN_IS_HIGH){
                            u8RecordingArmedFlag = NOT_ARMED;
                            finishRecording();
                        }
                    }
                }else{//Switch controlled
                    if(u8ClockTriggerFlag == TRUE ){
                       if(handleClock()){
                            u8RecordingArmedFlag = NOT_ARMED;
                            finishRecording();
                       }
                       u8ClockTriggerFlag = FALSE;
                    }
                }
            }else{
                //Recording is "NOT_ARMED", meaning it is running. 
                //If we're in EXTERNAL recording mode, we need to count edges.
                // If we're in regular trigger mode, we just count clocks.
                //If we in trigger auto mode, then we count clocks and look for the end of the loop.
                if(p_VectrData->u8Source[RECORD] == EXTERNAL
                   && (u8RecordTrigger == TRIGGER_WENT_HIGH) 
                   ||
                   (p_VectrData->u8Source[PLAYBACK] == SWITCH
                    && u8ClockTriggerFlag == TRUE)){

                        if(p_VectrData->u8Control[RECORD] == TRIGGER){
                            handleClock();
                        }
                        else if(p_VectrData->u8Control[RECORD] == TRIGGER_AUTO){
                            if(handleClock()){
                                u8RecordingArmedFlag = NOT_ARMED;
                                u8RecordTrigger = NO_TRIGGER;
                                finishRecording();
                            }
                        }

                        u8ClockTriggerFlag = FALSE;
                }
            }
      
            /*Recording - Keep track of airwheel but don't do anything with it to
             keep jumps from occurring*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;
//PLAYBACK
        case PLAYBACK:
            /*If playback is running, read the RAM.*/
            if(u8SampleTimerFlag == TRUE){
                runPlaybackMode(u8RecordTrigger);
            }

            /*During playback, we are only looking for gestures to enter performance
             or menu mode.*/
            if(u8MessageReceivedFlag == TRUE){
            /*Playback - Decode touch and gestures. If a double tap on the bottom occurs,
                 * enter menu mode*/
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            if(u8MenuModeFlag == FALSE){
                                u8MenuModeFlag = TRUE;
                                menuHandlerInit();
                            }
                            break;
                        case OVERDUB_MODE:
                            enterOverdubMode();
                            break;
                        case MUTE_MODE:
                            //Go to Mute Mode
                            enterMuteMode();
                            break;
                        case SEQUENCER_MODE:
                            //Go to sequencer mode if at least one sequence has been recorded
                            enterSequencerMode();
                            break;
                        case AIR_SCRATCH_MODE:
                            //Go to air scratch mode if a sequence has been recorded.
                            if(u8SequenceRecordedFlag == TRUE){
                                enterAirScratchMode();
                            }
                            else{
                                //indicate error.
                                LEDIndicateError();
                            }
                            break;
                        default:
                            break;
                    }
                }else{
                    /*Check for entering tap tempo mode
                     * 1. Tap on the right electrode.
                     * 2. Tap on the left electrode within 100ms.
                     */
                    decodeMultiTapGestures(u16TouchData);
                }

                /*Playback - Airwheel adjust speed.*/
                u16AirwheelData = pos_and_gesture_struct.u16Airwheel;

                if(u8HoldState != ON){
                    if(u16AirwheelData != u16LastAirWheelData){

                        if(u16AirwheelData != 0 && u8PlaybackRunFlag == RUN && u8MenuModeFlag == FALSE){
                        // if(u8PlaybackRunFlag == RUN && u8MenuModeFlag == FALSE){
                            i16AirWheelChange = u16AirwheelData - u16LastAirWheelData;

                            //If the data decremented past zero, the new data minus the old will be large.
                            if(i16AirWheelChange > 127){
                                i16AirWheelChange = u16LastAirWheelData - (256 - u16AirwheelData);
                            }

                            //If the data incremented past zero, the data will be large and negative
                            if(i16AirWheelChange < -127){
                                i16AirWheelChange = u16AirwheelData + (256 - u16LastAirWheelData);
                            }

                            if(p_VectrData->u8Source[RECORD] == SWITCH){
                                adjustSpeedTable(i16AirWheelChange);
                                u16LastAirWheelData = u16AirwheelData;
                            }
                            else{
                                //Externally clocked. Work in binary increments.
                                if(i16AirWheelChange >= EXTERNAL_AIR_WHEEL_MIN_CHANGE_INCREMENT){
                                    //Multiply the current speed by 2.
                                    u32NewPlaybackSpeedClocked = u32PlaybackSpeed>>1;//Lower number makes the clock faster
                                    u16LastAirWheelData = u16AirwheelData;
                                    if(u32NewPlaybackSpeedClocked > FASTEST_SPEED){
                                        turnOffAllLEDs();
                                        u16LastAirWheelData = u16AirwheelData;
                                        u8NewPlaybackSpeedClockedFlag = TRUE;
                                        u8ExternalAirWheelActiveFlag = TRUE;
                                        u32NewClockClicks = u32TargetNumTicksBetweenClocks<<1;
                                        if(u8ExternalAirwheelClockDelay > 0){
                                            u8ExternalAirwheelClockDelay>>=1;
                                        }
                                    }
                                }else if(i16AirWheelChange <= -EXTERNAL_AIR_WHEEL_MIN_CHANGE_INCREMENT){
                                    //Divide the current speed by 2.
                                    u32NewPlaybackSpeedClocked = u32PlaybackSpeed<<1;
                                    u16LastAirWheelData = u16AirwheelData;
                                    if(u32NewPlaybackSpeedClocked < SLOWEST_SPEED){
                                        turnOffAllLEDs();
                                        u8NewPlaybackSpeedClockedFlag = TRUE;
                                        u8ExternalAirWheelActiveFlag = TRUE;
                                        u32NewClockClicks = u32TargetNumTicksBetweenClocks>>1;
                                        if(u8ExternalAirwheelClockDelay != 0){
                                            u8ExternalAirwheelClockDelay<<=1;
                                        }else{
                                            u8ExternalAirwheelClockDelay=1;
                                        }
                                    }
                                }
                            }
                        }
                        else{
                            u16LastAirWheelData = u16AirwheelData;
                        }
                    }
                }else{
                    //Don't adjust speed during a hold
                    u16LastAirWheelData = u16AirwheelData;
                }
            }

            /*If tap tempo is actively being set, run the mode*/
            if(u8TapTempoModeActiveFlag == TRUE){
                runTapTempo();
            }

            /*If we're in gate mode, then a low level stops playback. A high level starts recording.
             */
            //Change to if trigger IS LOW!!!!!!!
            if(p_VectrData->u8Control[PLAY] == GATE && p_VectrData->u8Source[PLAY] == EXTERNAL){
               if(u8PlaybackArmedFlag == ARMED ){
                    if(PLAY_IN_IS_HIGH){
                        setPlaybackRunStatus(RUN);
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                    }
                    else{
                        setPlaybackRunStatus(STOP);
                        setSwitchLEDState(SWITCH_LED_GREEN);
                    }
               }
               else{
                   if(!PLAY_IN_IS_HIGH){
                       setPlaybackRunStatus(STOP);
                       setSwitchLEDState(SWITCH_LED_OFF);
                   }
               }
            }

             /*Check for armed playback or recording and the appropriate trigger.
             *These can only be armed or disarmed if the Source for each is set to external
             */
            if(u8PlaybackArmedFlag == ARMED){
                /*Look for a rising edge to start playback.
                 * If playback mode is Flip then a trigger will cause a direction change
                 * and not stop playback or start playback.
                 */
                if(p_VectrData->u8PlaybackMode != FLIP &&
                   p_VectrData->u8PlaybackMode != RETRIGGER && p_VectrData->u8PlaybackMode != ONESHOT){
                    if(((p_VectrData->u8Control[PLAY] == TRIGGER) || (p_VectrData->u8Control[PLAY] == TRIGGER_AUTO))
                            && (u8PlayTrigger == TRIGGER_WENT_HIGH)){
                        u8PlaybackArmedFlag = NOT_ARMED;
                        /*Clear the trigger and start playback.*/
                        u8PlayTrigger = NO_TRIGGER;
                        setPlaybackRunStatus(RUN);
                    }
                }else if(p_VectrData->u8PlaybackMode == FLIP){
                    if(p_VectrData->u8PlaybackDirection == FORWARD_PLAYBACK){
                        setPlaybackDirection(REVERSE_PLAYBACK);
                    }
                    else{
                        setPlaybackDirection(FORWARD_PLAYBACK);
                    }
                }
                else{
                    if(u8PlayTrigger == TRIGGER_WENT_HIGH){
                        //This if for one-shot mode and retrigger mode.
                        if(p_VectrData->u8PlaybackMode == FLIP || u8PlaybackRunFlag == ENDED){
                            setRAMRetriggerFlag();
                            //Retrigger playback mode.
                            setPlaybackRunStatus(RUN);
                        }
                        else{//One-shot mode
                            if(u8PlaybackRunFlag == RUN){
                              setPlaybackRunStatus(STOP);
                            }
                            else{
                               setPlaybackRunStatus(RUN);
                            }
                        }
                    }
                }
            }
            else if(u8PlaybackArmedFlag == DISARMED){
                if(((p_VectrData->u8Control[PLAY] == TRIGGER) || (p_VectrData->u8Control[PLAY] == TRIGGER_AUTO))
                     && (u8PlayTrigger == TRIGGER_WENT_LOW)){
       
                    u8PlaybackArmedFlag = NOT_ARMED;
                    /*Clear the trigger and start playback.*/
                    u8PlayTrigger = NO_TRIGGER;
                    setPlaybackRunStatus(STOP);
                    setSwitchLEDState(SWITCH_LED_OFF);
                }
            }

            if(u8RecordingArmedFlag == ARMED){
                if(p_VectrData->u8Source[RECORD] == EXTERNAL){
                   if(u8RecordTrigger == TRIGGER_WENT_HIGH){/*If we received the record trigger.*/
                        u8HoldState = OFF;
                        u8RecordTrigger = NO_TRIGGER;
                        u8RecordingArmedFlag = NOT_ARMED;
                        startNewRecording();
                   }
                }
                else{
                    //Switch controlled recording with tap tempo starts
                    //recording on a clock edge
                    if(u8ClockTriggerFlag == TRUE){
                        u8RecordingArmedFlag = NOT_ARMED;
                        startNewRecording();
                    }
                }
            }else{

                /*If record mode is set to external, we use the record input clock
                 * to keep the loop synchronized, to adjust to changes in tempo,
                 * and to start and stop the loop based on the presence or absence of
                 * the clock.
                 */
                if( p_VectrData->u8Control[RECORD] != GATE
                    && p_VectrData->u8Source[RECORD] == EXTERNAL){
                    
                    /* If playback is running, then we are looking for the clock edge.
                     * If the timer clock has run past the point that we expect 
                     * the clock plus some margin, then we stop playback.
                     * 
                     */
                    if(u8PlaybackRunFlag == RUN){
                        if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                            handleClock();
                            u8RecordTrigger = NO_TRIGGER;
                            if(u16ModulationOnFlag == FALSE){
                                regulateClockPlaybackSpeed();//Keep playback speed running.
                            }
                        }
                        else{
                            /* Check to see if the trigger is late. There is margin built into
                             * the expected number of ticks between clocks.
                             */
                            u16NumRecordClocks = getRecClockCount();
                            if(u16NumRecordClocks > u32ExpectedNumTicksBetweenClocks){
                               setPlaybackRunStatus(PAUSED);
                            }
                        }
                    }
                    else{
                        /* If playback is paused because the clock stopped, then we look for a clock edge
                         * to start playback again.
                         */
                        if(u8PlaybackRunFlag == PAUSED){
                            if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                                handleClock();
                                setPlaybackRunStatus(RUN);
                            }
                        }
                    }
                }
                else{
                    if((u8TapTempoModeActiveFlag == FALSE && u8ClockTriggerFlag == TRUE)){
                        handleClock();
                        u8ClockTriggerFlag = FALSE;
                    }
                }
            }
            break;
        /*Overdubbing mode records over the existing recording.*/
        case OVERDUBBING:
            /*In overdub mode, the playback runs like playback mode.
             *Initially, the LEDs will be lit up for the left, top, and right sides
             * to indicate the x, y, and z axes. Touching on any side will toggle that
             * axis on and off for overdubbing.
             * Pressing the switch activates overdubbing. Pressing it again ends overdubbing.
             * Double tapping on the bottom leaves overdubbing and goes back to playback.
             */

            /* When overdub is entered, the axes can be turned on or off. Touches
             * Touches on the left turn on/off x
             * Touches on the top turn on/off y
             * Touches on the right turn on/off z
             */

            /*Should menu mode be accessible in overdub mode? Maybe when
             overdub isn't running?*/
            if(u8MessageReceivedFlag == TRUE){
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){//double taps
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    /*Double tap enters menu mode*/
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            turnOffAllLEDs();
                            Flags.u8OverdubActiveFlag = FALSE;
                            setLEDAlternateFuncFlag(FALSE);
                            setIndicateOverdubModeFlag(FALSE);
                            u8OperatingMode =  PLAYBACK;
                            setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                            break;
                        default:
                            break;
                    }
                }
                else if(Flags.u8OverdubActiveFlag != TRUE){
                    //If overdub is inactive, the outputs can be turned on/off
                    if(u16TouchData > MGC3130_NO_TOUCH && u8GestureDebounceTimer == 0){
                        u8GestureFlag = decodeXYZTouchGesture(u16TouchData);
                        u8GestureDebounceTimer = GESTURE_DEBOUNCE_TIMER_RESET;
                        switch(u8GestureFlag){
                            case X_OUTPUT_INDEX:
                                p_VectrData->u8OverdubStatus[X_OUTPUT_INDEX] ^= 1;
                                break;
                            case Y_OUTPUT_INDEX:
                                p_VectrData->u8OverdubStatus[Y_OUTPUT_INDEX] ^= 1;
                                break;
                            case Z_OUTPUT_INDEX:
                                p_VectrData->u8OverdubStatus[Z_OUTPUT_INDEX] ^= 1;
                                break;
                            default:
                                break;
                        }
                        indicateActiveAxes(OVERDUB);
                    }
                }
            }

            /*Overdub running is like playback running. It's whether or not
             playback is running or it's stopped.*/
            if(u8OverdubRunFlag == TRUE && u8SampleTimerFlag == TRUE){

                /*We read two sets of samples at a time. So, it takes two
                 cycles to get through them. In the first cycle, we initiate a read
                 and in the second, we just use the data.*/
                if(u8BufferDataCount == 0){

                    xQueueReceive(xRAMReadQueue, &memBuffer, 0);
                    p_mem_pos_and_gesture_struct = &memBuffer.sample_1;
                    p_overdub_pos_and_gesture_struct = &overdubBuffer.sample_1;
                    u8BufferDataCount++;
                }
                else{
                    p_mem_pos_and_gesture_struct = &memBuffer.sample_2;
                    p_overdub_pos_and_gesture_struct = &overdubBuffer.sample_2;
                    u8BufferDataCount = 0;
                }

                /*This overdubbing works like a punch in/punchout except the punch in event
                 is the hand being present and the punch out is when the hand leaves. */
                if(Flags.u8OverdubActiveFlag == TRUE 
                   && (u8HandPresentFlag == TRUE || pos_and_gesture_struct.u16ZPosition > 0)){
                    //Overdub the axes with overdub active and for the others write back the accessed data
                    if(p_VectrData->u8OverdubStatus[X_OUTPUT_INDEX] == 1 && u8HandPresentFlag == TRUE){
                        p_mem_pos_and_gesture_struct->u16XPosition = pos_and_gesture_struct.u16XPosition;
                        p_overdub_pos_and_gesture_struct->u16XPosition = pos_and_gesture_struct.u16XPosition;
                    }
                    else{
                       p_overdub_pos_and_gesture_struct->u16XPosition =  p_mem_pos_and_gesture_struct->u16XPosition;
                    }

                    if(p_VectrData->u8OverdubStatus[Y_OUTPUT_INDEX] == 1 && u8HandPresentFlag == TRUE){
                        p_mem_pos_and_gesture_struct->u16YPosition = pos_and_gesture_struct.u16YPosition;
                        p_overdub_pos_and_gesture_struct->u16YPosition = pos_and_gesture_struct.u16YPosition;
                    }
                    else{
                        p_overdub_pos_and_gesture_struct->u16YPosition =  p_mem_pos_and_gesture_struct->u16YPosition;
                    }

                    if(p_VectrData->u8OverdubStatus[Z_OUTPUT_INDEX] == 1){
                        if(u8HandPresentFlag == FALSE){
                            handleZDecay(&pos_and_gesture_struct);
                            last_position_data.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                        }

                        p_mem_pos_and_gesture_struct->u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                        p_overdub_pos_and_gesture_struct->u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    }
                    else{
                        p_overdub_pos_and_gesture_struct->u16ZPosition =  p_mem_pos_and_gesture_struct->u16ZPosition;
                    }
                   
                    if(u8BufferDataCount == 0){
                        if(u8MemCommand == READ_RAM){
                            synchronizeReadWriteRAMAddress(getRAMReadAddress());
                        }
                        u8MemCommand = WRITETHENREAD_RAM;
                        xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);//Set up for OVERDUB
                        xQueueSend(xRAMWriteQueue, &overdubBuffer, 0);
                    }

                    handleTimeQuantization(p_mem_pos_and_gesture_struct, u8ClockTriggerFlag, u8RecordTrigger);
                    slewPosition(p_mem_pos_and_gesture_struct);
                    xQueueSend(xLEDQueue, p_mem_pos_and_gesture_struct, 0);
                    gateHandler(p_mem_pos_and_gesture_struct);
                    scaleRange(p_mem_pos_and_gesture_struct);
                    /*Quantize the position.*/
                    quantizePosition(p_mem_pos_and_gesture_struct);
                    xQueueSend(xSPIDACQueue, p_mem_pos_and_gesture_struct, 0);
                }
                else{//Not overdubbing
                    if(u8BufferDataCount == 0){
                        u8MemCommand = READ_RAM;
                        xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);//Set up for READ
                    }

                    if(u8HoldState == OFF){

                        //Playback is running. Run the clock
                        setClockEnableFlag(TRUE);
                        handleTimeQuantization(p_mem_pos_and_gesture_struct, u8ClockTriggerFlag, u8RecordTrigger);
                        slewPosition(p_mem_pos_and_gesture_struct);
                        //Handle the gate output
                        gateHandler(p_mem_pos_and_gesture_struct);
                   }
                   else{
                    /*If hold is active, execute the hold behavior*/
                        if(u8HandHoldFlag == TRUE){
                            handleZDecay(p_mem_pos_and_gesture_struct);
                        }
                        holdHandler(&pos_and_gesture_struct, p_mem_pos_and_gesture_struct, &hold_position_struct);
                        //Handle the gate output
                        gateHandler(p_mem_pos_and_gesture_struct);
                   }

                   /*Check encoder activations. Could go to live play or implement a hold*/
                    if(u8HoldActivationFlag == HOLD_ACTIVATE){
                        u8HoldState = ON;
                        setHoldPosition(p_mem_pos_and_gesture_struct);
                    }
                    else if(u8HoldActivationFlag == HOLD_DEACTIVATE){
                        u8HoldState = OFF;
                    }

                    if(u16ModulationOnFlag == TRUE){
                        runModulation(p_mem_pos_and_gesture_struct);
                    }

                    xQueueSend(xLEDQueue, p_mem_pos_and_gesture_struct, 0);
                    mutePosition(p_mem_pos_and_gesture_struct);
                    scaleRange(p_mem_pos_and_gesture_struct);
                    /*Quantize the position.*/
                    quantizePosition(p_mem_pos_and_gesture_struct);
                    xQueueSend(xSPIDACQueue, p_mem_pos_and_gesture_struct, 0);

                    
                    }
            }

            //Clear the flag
            if(u8LivePlayActivationFlag == TRUE){
                u8LivePlayActivationFlag = FALSE;
            }

            if(u8SyncTrigger == TRUE){
               if(p_VectrData->u8PlaybackMode != FLIP){
                   setRAMRetriggerFlag();

               }else{
                   if(p_VectrData->u8PlaybackDirection == FORWARD_PLAYBACK){
                       setPlaybackDirection(REVERSE_PLAYBACK);
                   }
                   else{
                       setPlaybackDirection(FORWARD_PLAYBACK);
                   }
               }
               u8SyncTrigger = FALSE;
            }

            //External Gate triggered mode.
            if(p_VectrData->u8Control[OVERDUB] == GATE && p_VectrData->u8Source[OVERDUB] == EXTERNAL){
                if(u8OverdubArmedFlag == ARMED){
                    if(REC_IN_IS_HIGH){
                        if(Flags.u8OverdubActiveFlag == FALSE){
                            setLEDAlternateFuncFlag(FALSE);
                            setIndicateOverdubModeFlag(FALSE);
                            turnOffAllLEDs();
                            setRAMWriteAddress(getRAMReadAddress());//synchronize write with read
                            setSwitchLEDState(SWITCH_LED_RED_BLINKING);
                        }
                        Flags.u8OverdubActiveFlag = TRUE;
                    }
                    else{ 
                        Flags.u8OverdubActiveFlag = FALSE;
                    }
                }else if(u8OverdubArmedFlag == DISARMED){
                    if(!REC_IN_IS_HIGH){
                        setLEDAlternateFuncFlag(TRUE);
                        setIndicateOverdubModeFlag(TRUE);
                        turnOffAllLEDs();
                        indicateActiveAxes(OVERDUB);
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                        Flags.u8OverdubActiveFlag = FALSE;
                    }
                }
            }

             /*Check for armed playback or recording and the appropriate trigger.
             *These can only be armed or disarmed if the Source for each is set to external
             */
            if(u8OverdubArmedFlag == ARMED){
                /*Look for a rising edge to start playback*/
                if(p_VectrData->u8Control[OVERDUB] == TRIGGER && u8RecordTrigger == TRIGGER_WENT_HIGH){
                    u8OverdubArmedFlag = NOT_ARMED;
                    /*Clear the trigger and start playback.*/
                    u8RecordTrigger = NO_TRIGGER;
                    Flags.u8OverdubActiveFlag = TRUE;
                    setSwitchLEDState(SWITCH_LED_RED_BLINKING);
                    setLEDAlternateFuncFlag(FALSE);
                    turnOffAllLEDs();
                    setIndicateOverdubModeFlag(FALSE);
                }
            }
            else if(u8OverdubArmedFlag == DISARMED){
                if(p_VectrData->u8Control[OVERDUB] == TRIGGER && u8RecordTrigger == TRIGGER_WENT_LOW){
                    u8OverdubArmedFlag = NOT_ARMED;
                    /*Clear the trigger and start playback.*/
                    u8RecordTrigger = NO_TRIGGER;
                    Flags.u8OverdubActiveFlag = FALSE;
                    setLEDAlternateFuncFlag(TRUE);
                    turnOffAllLEDs();
                    setIndicateOverdubModeFlag(TRUE);
                    if(u8PlaybackRunFlag == TRUE){
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                    }
                    else{
                        setSwitchLEDState(SWITCH_LED_OFF);
                    }
                }
            }
            else{

                /*If record mode is set to external, we use the record input clock
                 * to keep the loop synchronized, to adjust to changes in tempo,
                 * and to start and stop the loop based on the presence or absence of
                 * the clock.
                 */
                if( p_VectrData->u8Control[RECORD] != GATE
                    && p_VectrData->u8Source[RECORD] == EXTERNAL){

                    /* If playback is running, then we are looking for the clock edge.
                     * If the timer clock has run past the point that we expect
                     * the clock plus some margin, then we stop playback.
                     *
                     */
                    if(u8OverdubRunFlag == RUN){
                        if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                            handleClock();
                            if(u16ModulationOnFlag == FALSE){
                                regulateClockPlaybackSpeed();//Keep playback speed running.
                            }
                        }
                        else{
                            /* Check to see if the trigger is late. There is margin built into
                             * the expected number of ticks between clocks.
                             */
                            if(getRecClockCount() > u32ExpectedNumTicksBetweenClocks){
                                setPlaybackRunStatus(PAUSED);
                            }
                        }
                    }
                    else{
                        /* If playback is paused because the clock stopped, then we look for a clock edge
                         * to start playback again.
                         */
                        if(u8OverdubRunFlag == PAUSED){
                            if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                                handleClock();
                                setPlaybackRunStatus(RUN);
                            }
                        }
                    }
                }
                else{
                    //Switch triggered
                    if(u8TapTempoSetFlag == TRUE){
                        if(u8ClockTriggerFlag == TRUE){
                            handleClock();
                        }
                        u8ClockTriggerFlag = FALSE;
                    }
                }
            }
            /*Overdub - Keep track of airwheel data but don't do anything about it*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;//Overdubbing end

        case SEQUENCING:

            /* For Sequencing mode, the LEDs indicate the sequences that are available to play.
             * Left = Recorded sequence or Flash Sequence 0
             * Top = Next Flash sequence
             * Right = Next Flash Sequence
             * All Sides = Next Flash Sequence - Tied to the center touch.
             *
             * The activation of sequences follows the playback settings.
             */

            if(u8SampleTimerFlag == TRUE && u8SequenceRecordedFlag == TRUE){
                runPlaybackMode(u8RecordTrigger);
            }

            //Activate the selected sequence.
            if(u8SequenceModeChangeSequenceFlag == TRUE){
                u8SequenceModeChangeSequenceFlag = FALSE;

                //Is the sequence in RAM or Flash?
                if(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex] == RAM_SEQUENCE_MASK){
                    setStoredSequenceLocationFlag(STORED_IN_RAM);
                    resetRAMReadAddress();
                    resetRAMEndofReadAddress();
                    //Move the Vectr Data pointer to the standard table.
                    p_VectrData = &VectrData;
                    //loadSettingsFromFileTable(0);
                }
                else{
                    setStoredSequenceLocationFlag(STORED_IN_FLASH);
                    setFlashReadNewSequence(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex]);
                    //Move teh Vectr Data pointer to a table in the local file table.
                    p_VectrData = setFileTableDataPointer(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex]);
                   // loadSettingsFromFileTable(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex]);
                }
                u8SequenceRecordedFlag = TRUE;
                u8PlaybackRunFlag = RUN;
            }

            if(u8MessageReceivedFlag == TRUE){
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){//double taps
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            setLEDAlternateFuncFlag(FALSE);
                            turnOffAllLEDs();
                            Flags.u8SequencingFlag = FALSE;
                            setLEDAlternateFuncFlag(FALSE);
                            setIndicateSequenceModeFlag(FALSE);
                            u8OperatingMode =  u8PreviousOperatingMode;
                            u8PlaybackRunFlag = RUN;
                            /*Copy the current data structure into the standard local
                             data structure.*/
                            if(getStoredSequenceLocationFlag() != STORED_IN_RAM){
                                p_VectrData = &VectrData;
                                loadSettingsFromFileTable(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex]);
                            }

                            break;
                        default:
                            break;
                    }
                }
                else if(Flags.u8SequencingFlag != TRUE){
                    /*A touch on the axis enables the sequence.
                     * if there is one recorded.
                     */
                    if(u16TouchData > MGC3130_NO_TOUCH && u8GestureDebounceTimer == 0){
                        u8GestureFlag = decodeXYZTouchGesture(u16TouchData);
                        u8GestureDebounceTimer = GESTURE_DEBOUNCE_TIMER_RESET;
                        switch(u8GestureFlag){
                            case X_OUTPUT_INDEX://Left
                                if(u8SequenceModeIndexes[X_OUTPUT_INDEX] != NO_SEQUENCE_MASK){
                                    u8SequenceModeSelectedSequenceIndex = X_OUTPUT_INDEX;
                                }
                                break;
                            case Y_OUTPUT_INDEX://Top
                                if(u8SequenceModeIndexes[Y_OUTPUT_INDEX] != NO_SEQUENCE_MASK){
                                    u8SequenceModeSelectedSequenceIndex = Y_OUTPUT_INDEX;
                                }
                                break;
                            case Z_OUTPUT_INDEX://Right
                                if(u8SequenceModeIndexes[Z_OUTPUT_INDEX] != NO_SEQUENCE_MASK){
                                    u8SequenceModeSelectedSequenceIndex = Z_OUTPUT_INDEX;
                                }
                                break;
                            case CENTER_INDEX://Center
                                if(u8SequenceModeIndexes[CENTER_INDEX] != NO_SEQUENCE_MASK){
                                    u8SequenceModeSelectedSequenceIndex = CENTER_INDEX;
                                }
                                break;
                            default:
                                break;
                        }
                    }
                }
            }

             /*If record mode is set to external, we use the record input clock
             * to keep the loop synchronized, to adjust to changes in tempo,
             * and to start and stop the loop based on the presence or absence of
             * the clock.
             */
            if( p_VectrData->u8Control[RECORD] != GATE
                && p_VectrData->u8Source[RECORD] == EXTERNAL){

                /* If playback is running, then we are looking for the clock edge.
                 * If the timer clock has run past the point that we expect
                 * the clock plus some margin, then we stop playback.
                 *
                 */
                if(u8PlaybackRunFlag == RUN){
                    if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                        handleClock();
                        if(u16ModulationOnFlag == FALSE){
                            regulateClockPlaybackSpeed();//Keep playback speed running.
                        }
                    }
                    else{
                        /* Check to see if the trigger is late. There is margin built into
                         * the expected number of ticks between clocks.
                         */
                        if(getRecClockCount() > u32ExpectedNumTicksBetweenClocks){
                            setPlaybackRunStatus(PAUSED);
                        }
                    }
                }
                else{
                    /* If playback is paused because the clock stopped, then we look for a clock edge
                     * to start playback again.
                     */
                    if(u8PlaybackRunFlag == PAUSED){
                        if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                            handleClock();
                            setPlaybackRunStatus(RUN);
                        }
                    }
                }
            }
            else{
                //Switch triggered
                if(u8TapTempoSetFlag == TRUE){
                    if(u8ClockTriggerFlag == TRUE){
                        handleClock();
                    }
                    u8ClockTriggerFlag = FALSE;
                }
            }
            /*Sequencing - Keep track of airwheel data but don't do anything about it*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;
        case MUTING:
            if(u8SampleTimerFlag == TRUE && u8SequenceRecordedFlag == TRUE){
                runPlaybackMode(u8RecordTrigger);
            }

            if(u8MessageReceivedFlag == TRUE){
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){//double taps
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            setLEDAlternateFuncFlag(FALSE);
                            turnOffAllLEDs();
                            Flags.u8SequencingFlag = FALSE;
                            setLEDAlternateFuncFlag(FALSE);
                            setIndicateMuteModeFlag(FALSE);
                            u8OperatingMode =  u8PreviousOperatingMode;//Fix this. Back to Live Play?
                            break;
                        default:
                            break;
                    }
                }
                else if(u16TouchData > MGC3130_NO_TOUCH && u8GestureDebounceTimer == 0){
                    //A touch toggles the mute for that channel on and off.
                    u8GestureFlag = decodeXYZTouchGesture(u16TouchData);
                        u8GestureDebounceTimer = GESTURE_DEBOUNCE_TIMER_RESET;
                        switch(u8GestureFlag){
                            case X_OUTPUT_INDEX://Left
                                p_VectrData->u8MuteState[X_OUTPUT_INDEX] ^= TRUE;
                                break;
                            case Y_OUTPUT_INDEX://Top
                                p_VectrData->u8MuteState[Y_OUTPUT_INDEX] ^= TRUE;
                                break;
                            case Z_OUTPUT_INDEX://Right
                                p_VectrData->u8MuteState[Z_OUTPUT_INDEX] ^= TRUE;
                                break;
                            default:
                                break;
                        }
                }
            }

             /*If record mode is set to external, we use the record input clock
             * to keep the loop synchronized, to adjust to changes in tempo,
             * and to start and stop the loop based on the presence or absence of
             * the clock.
             */
            if( p_VectrData->u8Control[RECORD] != GATE
                && p_VectrData->u8Source[RECORD] == EXTERNAL){

                /* If playback is running, then we are looking for the clock edge.
                 * If the timer clock has run past the point that we expect
                 * the clock plus some margin, then we stop playback.
                 *
                 */
                if(u8PlaybackRunFlag == RUN){
                    if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                        handleClock();
                        if(u16ModulationOnFlag == FALSE){
                            regulateClockPlaybackSpeed();//Keep playback speed running.
                        }
                    }
                    else{
                        /* Check to see if the trigger is late. There is margin built into
                         * the expected number of ticks between clocks.
                         */
                        if(getRecClockCount() > u32ExpectedNumTicksBetweenClocks){
                            setPlaybackRunStatus(PAUSED);
                        }
                    }
                }
                else{
                    /* If playback is paused because the clock stopped, then we look for a clock edge
                     * to start playback again.
                     */
                    if(u8PlaybackRunFlag == PAUSED){
                        if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                            handleClock();
                            setPlaybackRunStatus(RUN);
                        }
                    }
                }
            }
            else{
                //Switch triggered
                if(u8TapTempoSetFlag == TRUE){
                    if(u8ClockTriggerFlag == TRUE){
                        handleClock();
                    }
                    u8ClockTriggerFlag = FALSE;
                }
            }

            /*Keep track of airwheel data but don't do anything about it*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;
        case AIR_SCRATCHING:

            runAirScratchMode(&pos_and_gesture_struct);
            
            runPlaybackMode(u8RecordTrigger);

            if(u8MessageReceivedFlag == TRUE){
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){//double taps
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            setLEDAlternateFuncFlag(FALSE);
                            turnOffAllLEDs();
                            Flags.u8SequencingFlag = FALSE;
                            setLEDAlternateFuncFlag(FALSE);
                            setIndicateMuteModeFlag(FALSE);
                            u8OperatingMode =  u8PreviousOperatingMode;
                            if(u8OperatingMode == PLAYBACK && u8PlaybackRunFlag == TRUE){
                                setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                            }
                            else{
                                setSwitchLEDState(SWITCH_LED_OFF);
                            }
                            break;
                        default:
                            break;
                    }
                }
            }

            if(p_VectrData->u8Source[RECORD] == EXTERNAL
               && (u8RecordTrigger == TRIGGER_WENT_HIGH)
               ||
               (p_VectrData->u8Source[PLAYBACK] == SWITCH
                && u8ClockTriggerFlag == TRUE)){
                    handleClock();
                    u8ClockTriggerFlag = FALSE;
            }

            /*Keep track of airwheel data but don't do anything about it*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;
        default:
            break;
    }
}

void defaultSettings(void){
    VectrData.u8PlaybackMode = LOOPING;
    VectrData.u8PlaybackDirection = FORWARD_PLAYBACK;
    VectrData.u8PlaybackMode = LOOPING;
    VectrData.u8ModulationMode = SPEED;
    VectrData.u8GateMode = HAND_GATE;
    VectrData.u16SlewRate[X_OUTPUT_INDEX] = MID_SLEW_RATE;
    VectrData.u16SlewRate[Y_OUTPUT_INDEX] = MID_SLEW_RATE;
    VectrData.u16SlewRate[Z_OUTPUT_INDEX] = MID_SLEW_RATE;
    VectrData.u8Range[X_OUTPUT_INDEX] = RANGE_UNI_5V;
    VectrData.u8Range[Y_OUTPUT_INDEX] = RANGE_UNI_5V;
    VectrData.u8Range[Z_OUTPUT_INDEX] = RANGE_UNI_5V;
    VectrData.u8Source[RECORD] = SWITCH;
    VectrData.u8Source[PLAY] = SWITCH;
    VectrData.u8Source[OVERDUB] = SWITCH;
    VectrData.u8Control[RECORD] = TRIGGER_AUTO;
    VectrData.u8Control[PLAY] = TRIGGER_AUTO;
    VectrData.u8Control[OVERDUB] = TRIGGER;
    VectrData.u8OverdubStatus[X_OUTPUT_INDEX] = TRUE;
    VectrData.u8OverdubStatus[Y_OUTPUT_INDEX] = TRUE;
    VectrData.u8OverdubStatus[Z_OUTPUT_INDEX] = TRUE;
    VectrData.u8HoldBehavior[X_OUTPUT_INDEX] = HOLD;
    VectrData.u8HoldBehavior[Y_OUTPUT_INDEX] = HOLD;
    VectrData.u8HoldBehavior[Z_OUTPUT_INDEX] = HOLD;
    VectrData.u16Quantization[X_OUTPUT_INDEX] = NO_QUANTIZATION;
    VectrData.u16Quantization[Y_OUTPUT_INDEX] = NO_QUANTIZATION;
    VectrData.u16Quantization[Z_OUTPUT_INDEX] = NO_QUANTIZATION;
    VectrData.u8MuteState[X_OUTPUT_INDEX] = FALSE;
    VectrData.u8MuteState[Y_OUTPUT_INDEX] = FALSE;
    VectrData.u8MuteState[Z_OUTPUT_INDEX] = FALSE;
    VectrData.u16CurrentXPosition = 0;
    VectrData.u16CurrentYPosition = 0;
    VectrData.u16CurrentZPosition = 0;
    VectrData.u16Linearity[X_OUTPUT_INDEX] = LINEARITY_STRAIGHT;
    VectrData.u16Linearity[Y_OUTPUT_INDEX] = LINEARITY_STRAIGHT;
    VectrData.u16Linearity[Z_OUTPUT_INDEX] = LINEARITY_STRAIGHT;
    VectrData.u8NumClocks = CLOCK_PULSE_16;
    VectrData.u8TimeQuantization[X_OUTPUT_INDEX] = FALSE;
    VectrData.u8TimeQuantization[Y_OUTPUT_INDEX] = FALSE;
    VectrData.u8TimeQuantization[Z_OUTPUT_INDEX] = FALSE;
}

void runPlaybackMode(uint8_t u8RecordTrigger){

   if(u8PlaybackRunFlag == RUN){
       if(u8HoldState == OFF){
            if(u8BufferDataCount == 0){
                xQueueReceive(xRAMReadQueue, &memBuffer, 0);
                p_mem_pos_and_gesture_struct = &memBuffer.sample_1;
                u8BufferDataCount++;
            }
            else{
                p_mem_pos_and_gesture_struct = &memBuffer.sample_2;
                u8MemCommand = READ_RAM;
                xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);
                u8BufferDataCount = 0;
            }

            //Playback is running. Run the clock
            setClockEnableFlag(TRUE);
            handleTimeQuantization(p_mem_pos_and_gesture_struct, u8ClockTriggerFlag, u8RecordTrigger);
            slewPosition(p_mem_pos_and_gesture_struct);
            //Handle the gate output
            gateHandler(p_mem_pos_and_gesture_struct);
       }
       else{
        /*If hold is active, execute the hold behavior*/
            if(u8HandHoldFlag == TRUE){
                handleZDecay(p_mem_pos_and_gesture_struct);
            }
            holdHandler(&pos_and_gesture_struct, p_mem_pos_and_gesture_struct, &hold_position_struct);
            //Handle the gate output
            gateHandler(p_mem_pos_and_gesture_struct);
       }

        

       /*Check encoder activations. Could go to live play or implement a hold*/
        if(u8HoldActivationFlag == HOLD_ACTIVATE){
            u8HoldState = ON;
            setHoldPosition(p_mem_pos_and_gesture_struct);
        }
        else if(u8HoldActivationFlag == HOLD_DEACTIVATE){
            u8HoldState = OFF;
        } 

        if(u16ModulationOnFlag == TRUE){
            runModulation(p_mem_pos_and_gesture_struct);
        }

        xQueueSend(xLEDQueue, p_mem_pos_and_gesture_struct, 0);
        mutePosition(p_mem_pos_and_gesture_struct);

        scaleRange(p_mem_pos_and_gesture_struct);
        /*Quantize the position.*/
        quantizePosition(p_mem_pos_and_gesture_struct);
        xQueueSend(xSPIDACQueue, p_mem_pos_and_gesture_struct, 0);
   }

   if(u8LivePlayActivationFlag == TRUE){
        setSwitchLEDState(OFF);
        u8HoldState = OFF;
        if(u8OperatingMode != SEQUENCING){
            u8OperatingMode = LIVE_PLAY;
            setPlaybackRunStatus(STOP);
        }
        u8LivePlayActivationFlag = FALSE;
   }
       
   if(u8SyncTrigger == TRUE){
       if(p_VectrData->u8PlaybackMode != FLIP){
           setRAMRetriggerFlag();

       }else{
           if(p_VectrData->u8PlaybackDirection == FORWARD_PLAYBACK){
               setPlaybackDirection(REVERSE_PLAYBACK);
           }
           else{
               setPlaybackDirection(FORWARD_PLAYBACK);
           }
       }

       u8SyncTrigger = FALSE;
    }   
}

/* Air scratch mode behavior.
 * If a hand is present, then it's controlling the speed. If a hand is not present, then the
 * loop plays at normal speed.
 * Upward hand movements cause the loop to play forward at a speed proportionate to the
 * forward movement.
 * Downward hand movements cause the loop to play backwards at a speed proportionate to the
 * downward movement.
 * The speed has inertia back toward the current system speed. If the neutral speed is faster when
 * the hand leaves, then it slows down.
 * If the neutral speed is slower than the present speed, when the hand leaves, then it slows down.
 *
 */
void runAirScratchMode(pos_and_gesture_data * p_pos_and_gesture_struct){
    static uint16_t u16LastScratchPosition;
    static uint8_t u8LastHandPresentFlag = FALSE;
    int32_t i32Movement;
    uint16_t u16NewScratchData = p_pos_and_gesture_struct->u16YPosition;

    if(u8LastHandPresentFlag == FALSE && u8HandPresentFlag == TRUE){
        u16LastScratchPosition = u16NewScratchData;
    }

    if(u8AirScratchRunFlag == TRUE && u8HandPresentFlag == TRUE){
        //If air scratch is on, and the hand is present, hand movements control playback.

        i32Movement =  u16NewScratchData - u16LastScratchPosition;
        i32Movement >>= 5;//Scale down to the size of the table.

        if(i32Movement > 2){
            setPlaybackDirection(FORWARD_PLAYBACK);
            //Upward hand movement.
            u8PlaybackRunFlag = TRUE;
            //Play forward based on the movement.
            i32Movement = u16PlaybackSpeedTableIndex + i32Movement;
            if(i32Movement >= LENGTH_OF_LOG_TABLE){
                i32Movement = LENGTH_OF_LOG_TABLE - 1;
            }
            u32AirScratchPlaybackSpeed = u32LogSpeedTable[i32Movement];
            u16CurrentScratchSpeedIndex = i32Movement;
            SET_SAMPLE_TIMER_PERIOD(u32AirScratchPlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
        else if(i32Movement < -2){
            setPlaybackDirection(REVERSE_PLAYBACK);
            //Downward hand movement.
            u8PlaybackRunFlag = TRUE;
            i32Movement = u16PlaybackSpeedTableIndex - i32Movement;
            if(i32Movement < 0){
                i32Movement = 0;
            }
            u32AirScratchPlaybackSpeed = u32LogSpeedTable[i32Movement];
            u16CurrentScratchSpeedIndex = i32Movement;
            SET_SAMPLE_TIMER_PERIOD(u32AirScratchPlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
        else{
            u8PlaybackRunFlag = FALSE;
        }
    }
    else{
        u8PlaybackRunFlag = TRUE;
        setPlaybackDirection(FORWARD_PLAYBACK);
        /*Slow down or speed up until we reach the neutral speed.*/
        if(u16CurrentScratchSpeedIndex > u16PlaybackSpeedTableIndex){
            u16CurrentScratchSpeedIndex -= SCRATCH_SLOWDOWN_CONSTANT;
            if(u16CurrentScratchSpeedIndex < u16PlaybackSpeedTableIndex){
                u16CurrentScratchSpeedIndex = u16PlaybackSpeedTableIndex;
            }

            u32AirScratchPlaybackSpeed = u32LogSpeedTable[u16CurrentScratchSpeedIndex];
            SET_SAMPLE_TIMER_PERIOD(u32AirScratchPlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
        else if(u16CurrentScratchSpeedIndex < u16PlaybackSpeedTableIndex){
            u16CurrentScratchSpeedIndex += SCRATCH_SLOWDOWN_CONSTANT;

            if(u16CurrentScratchSpeedIndex > u16PlaybackSpeedTableIndex){
                u16CurrentScratchSpeedIndex = u16PlaybackSpeedTableIndex;
            }

            u32AirScratchPlaybackSpeed = u32LogSpeedTable[u16CurrentScratchSpeedIndex];
            SET_SAMPLE_TIMER_PERIOD(u32AirScratchPlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
    }

    if(u8HandPresentFlag == TRUE){
        u16LastScratchPosition = u16NewScratchData;
        u8LastHandPresentFlag = TRUE;
    }else{
        u8LastHandPresentFlag = FALSE;
    }
}


uint32_t calculateRampOutput(void){
    uint32_t u32SequencePosition = getSequencePlaybackPosition();
    uint32_t u32SequenceLength = getActiveSequenceLength();
    uint32_t u32OutputValue;

    u32OutputValue = MAXIMUM_OUTPUT_VALUE*u32SequencePosition;
    u32OutputValue /= u32SequenceLength;

    return u32OutputValue;
}

/*Clock pulses are generated in the sample timer routine. This routine calculates
 how long to count for clock pulses from the number of bytes used to store the
 sequence.*/
uint32_t calculateNextClockPulse(void){
    static uint8_t u8RoundFlag = 0;
    uint32_t u32LengthOfSequence = getActiveSequenceLength();
    uint8_t u8NumClocks = p_VectrData->u8NumClocks;
    uint8_t u8Modulus;

    u32NextClockPulseIndex = u32LengthOfSequence>>u8NumClocks;

    u8Modulus = u32NextClockPulseIndex%6;

    if(u8Modulus){
        if(u8RoundFlag == 0){
            u8RoundFlag = 1;
            u32NextClockPulseIndex -= u8Modulus;
        }
        else{
            u8RoundFlag = 0;
            u32NextClockPulseIndex += 6-u8Modulus;
        }
    }

    return u32NextClockPulseIndex;
}

/*The clock timer creates the clock pulses. The regular sample timer can't be
 used because the resolution is too poor. With this routine, we run the clock timer
 at a multiple faster than the regular timer and count up to a number to trigger
 the correct to get more accurate timing.*/
void calculateClockTimer(uint32_t u32Speed){

    uint32_t u32LengthOfSequence = getActiveSequenceLength()/6;

    uint8_t u8ClockMode = p_VectrData->u8NumClocks;//Clock pulses per cycle
    uint32_t u32ClockSpeed = u32Speed;//How fast does the clock have to run?
    uint16_t u16Multiple = 3;//Run at least 8x faster than the sample clock
    uint32_t u32ClockTimerTriggerCount;
    
    /*The clock needs to run at a multiple of the frequency of the
     regular sample clock, but not any faster than necessary. The longest the
     clock can be is 65535 because it is 16 bit. We start with 8x faster. If that's
     not fast enough, we increase it to, 16x, 32x, or 64x.
     For example: the sample timer is running at 399999 counts per sample.
     The clock timer will run at 8x this speed at 49999 counts per cycle.
     */
    u32ClockSpeed >>= u16Multiple;
    while(u32ClockSpeed > MAX_CLOCK_TIMER_SPEED){
        u16Multiple++;
        u32ClockSpeed >>= 1;
    }
    
    /*Since the clock timer will be running at a multiple faster than the sample timer. It must 
     count to a number higher by that multiple to get the correct number of cycles to trigger
     the clock pulse.
     This gives us the length of the cycle in terms of the speed of the clock timer.*/
    u32ClockTimerTriggerCount = u32LengthOfSequence << u16Multiple;

    /*Now, divide by the number of clock pulses per cycle.
     This is the number we must count up to in the interrupt routine.*/
    u32ClockTimerTriggerCount >>= u8ClockMode;

    setClockTimerTriggerCount(u32ClockTimerTriggerCount);

    SET_CLOCK_TIMER_PERIOD(u32ClockSpeed);
    RESET_CLOCK_TIMER;

}

void clearNextClockPulseIndex(void){
    u32NextClockPulseIndex = 0;
}

void enterOverdubMode(void){
    if(u8SequenceRecordedFlag == TRUE && getStoredSequenceLocationFlag() == STORED_IN_RAM){
        u8OperatingMode =  OVERDUBBING;
        setLEDAlternateFuncFlag(TRUE);
        turnOffAllLEDs();

        setIndicateOverdubModeFlag(TRUE);
        Flags.u8OverdubActiveFlag = FALSE;
        u8OverdubRunFlag = u8PlaybackRunFlag;
    }
    else{
        //indicate error.
        LEDIndicateError();
    }
}

void enterAirScratchMode(void){
    uint32_t u32TableSpeed = u32LogSpeedTable[LENGTH_OF_LOG_TABLE-1];
    u16CurrentScratchSpeedIndex = LENGTH_OF_LOG_TABLE-1;
    
    while(u32TableSpeed < u32PlaybackSpeed){
        u32TableSpeed = u32LogSpeedTable[u16CurrentScratchSpeedIndex];
        u16CurrentScratchSpeedIndex--;
    }
    u16PlaybackSpeedTableIndex = u16CurrentScratchSpeedIndex;
    u32AirScratchPlaybackSpeed = u32TableSpeed;
    u8PreviousOperatingMode = u8OperatingMode;
    u8OperatingMode = AIR_SCRATCHING;
    u8AirScratchRunFlag = FALSE;
    setSwitchLEDState(SWITCH_LED_RED_GREEN_ALTERNATING);
}

void enterMuteMode(void){
    u8PreviousOperatingMode = u8OperatingMode;
    u8OperatingMode = MUTING;
    setLEDAlternateFuncFlag(TRUE);
    turnOffAllLEDs();
    setIndicateMuteModeFlag(TRUE);
}

/*In order to start the sequencer, we have to determine how many saved sequences there are
 and indicate the locations of stored sequences.*/
void enterSequencerMode(void){
    int i;
    int sequenceIndex = 0;
    int atLeastOneSequenceRecordedFlag = FALSE;

    //If there is a recorded sequence, it goes in the first position
    if(u8SequenceRecordedFlag == TRUE){
        u8SequenceModeIndexes[sequenceIndex] = RAM_SEQUENCE_MASK;
        atLeastOneSequenceRecordedFlag = TRUE;
    }

    //Load the other positions.
    for(i=0;i<(MAX_NUM_OF_SEQUENCES-u8SequenceRecordedFlag);i++){
        if(getIsSequenceProgrammed(i)){
            u8SequenceModeIndexes[i+u8SequenceRecordedFlag] = i;
            atLeastOneSequenceRecordedFlag = TRUE;
        }
        else{
            u8SequenceModeIndexes[i+u8SequenceRecordedFlag] = NO_SEQUENCE_MASK;
        }
    }

    /*If at least one sequence is recorded, go to sequence mode, otherwise
     * indicate an error*/
    if(atLeastOneSequenceRecordedFlag == TRUE){
        u8PreviousOperatingMode = u8OperatingMode;
        u8OperatingMode = SEQUENCING;
        Flags.u8SequencingFlag = FALSE;
        setLEDAlternateFuncFlag(TRUE);
        turnOffAllLEDs();
        setIndicateSequenceModeFlag(TRUE);
    }
    else{
        LEDIndicateError();
    }

}

//Muting sets the output channel to 0V.
void mutePosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    if(p_VectrData->u8MuteState[X_OUTPUT_INDEX] == TRUE){
        if(p_VectrData->u8Range[X_OUTPUT_INDEX] <= RANGE_BI_2_5V){
            p_pos_and_gesture_struct->u16XPosition = ZERO_VOLT_OUTPUT_VALUE;
        }
        else{
            p_pos_and_gesture_struct->u16XPosition = 0;
        }
    }

    if(p_VectrData->u8MuteState[Y_OUTPUT_INDEX] == TRUE){
        if(p_VectrData->u8Range[Y_OUTPUT_INDEX] <= RANGE_BI_2_5V){
            p_pos_and_gesture_struct->u16YPosition = ZERO_VOLT_OUTPUT_VALUE;
        }
        else{
            p_pos_and_gesture_struct->u16YPosition = 0;
        }
    }

    if(p_VectrData->u8MuteState[Z_OUTPUT_INDEX] == TRUE){
        if(p_VectrData->u8Range[Z_OUTPUT_INDEX] <= RANGE_BI_2_5V){
            p_pos_and_gesture_struct->u16ZPosition = ZERO_VOLT_OUTPUT_VALUE;
        }
        else{
            p_pos_and_gesture_struct->u16ZPosition = 0;
        }
    }
}

uint8_t getSelectedSequenceIndex(void){
    return u8SequenceModeSelectedSequenceIndex;
}

uint8_t getSequenceModeState(uint8_t u8SequenceIndex){
    if(u8SequenceModeSelectedSequenceIndex == u8SequenceIndex){
        return SEQUENCE_SELECTED;
    }
    else if(u8SequenceModeIndexes[u8SequenceIndex] != NO_SEQUENCE_MASK){
        return SEQUENCE_STORED;
    }
    else{
        return NO_SEQUENCE_MASK;
    }
}

void setPulseTimerExpiredFlag(void){
    u8PulseExpiredFlag = TRUE;
}

void setHandPresentFlag(uint8_t u8NewState){
    u8HandPresentFlag = u8NewState;
}

/*This function controls the gate output depending on the gate mode.*/
void gateHandler(pos_and_gesture_data * p_pos_and_gesture_struct){
    /*  HAND_GATE - High when a hand is present, low when it is not.
        RESET_GATE - Pulse at beginning of playback
        PLAY_GATE - High during playback, low when not.
        RECORD_GATE - High during recording, low when not.
        QUANT_GATE - Pulse for every quantization step.
     */
    uint8_t u8GateMode = p_VectrData->u8GateMode;

    switch(u8GateMode){
        case HAND_GATE:
            /*If a hand is present, high. Otherwise, low.*/
            if(p_pos_and_gesture_struct->u16ZPosition > 0){
                SET_GATE_OUT_PORT;
            }else{
                CLEAR_GATE_OUT_PORT;
            }
            break;
        case RESET_GATE:
            /*Handled in the TIM5 routine to keep the timing tight.*/
            break;
        case PLAY_GATE:
            /*If playback is running, high. Otherwise, low.*/
            if(u8PlaybackRunFlag == TRUE){
                SET_GATE_OUT_PORT;
            }else{
                CLEAR_GATE_OUT_PORT;
            }
            break;
        case RECORD_GATE:
            /*If record is running, high. Otherwise, low.*/
            if(u8RecordRunFlag == TRUE){
                SET_GATE_OUT_PORT;
            }
            else{
                CLEAR_GATE_OUT_PORT;
            }
            break;
        case QUANT_GATE:
            /*If a quantization event occurred, start a pulse.*/
            if(u8QuantizationGateFlag == TRUE && u8GatePulseFlag == FALSE){
                u8GatePulseFlag = TRUE;
                SET_GATE_OUT_PORT;
                u8QuantizationGateFlag = FALSE;
            }
            else{
                u8GatePulseFlag = FALSE;
                CLEAR_GATE_OUT_PORT;
            }
           
            break;
        default:
            break;
    }
}

/*Quantize the position data.*/
void quantizePosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    int i;
    static uint16_t u16LastQuantization[NUM_OF_AXES];
    uint8_t u8QuantizationMode;
    uint16_t u16temp;
    uint32_t u32temp;
    uint8_t u8index;
    uint16_t u16CurrentPosition[NUM_OF_AXES] = {p_pos_and_gesture_struct->u16XPosition,
                                        p_pos_and_gesture_struct->u16YPosition,
                                        p_pos_and_gesture_struct->u16ZPosition};
    
    for(i=0;i<NUMBER_OF_OUTPUTS;i++){
        u8QuantizationMode = p_VectrData->u16Quantization[i];
        u16temp = u16CurrentPosition[i];
        u32temp = u16CurrentPosition[i];
        switch(u8QuantizationMode){
            case NO_QUANTIZATION://do nothing.
                break;
            case CHROMATIC:
                u32temp <<= 4;
                u32temp /= 4369;
                if(u32temp & 0x01){
                    u32temp >>= 1;
                    u32temp++;
                }else{
                    u32temp >>= 1;
                }

                u16CurrentPosition[i] = u16ChromaticScale[u32temp];
                if(u16CurrentPosition[i] != u16LastQuantization[i]){
                    u8QuantizationGateFlag = TRUE;
                }
            
                break;
            case MAJOR:
                u16CurrentPosition[i] = scaleSearch(u16MinorScale, u16temp, LENGTH_OF_MINOR_SCALE);
                if(u16CurrentPosition[i] != u16LastQuantization[i]){
                    u8QuantizationGateFlag = TRUE;
                }
                break;
            case PENTATONIC:
                u16CurrentPosition[i] = scaleSearch(u16PentatonicScale, u16temp, LENGTH_OF_PENTATONIC_SCALE);
                if(u16CurrentPosition[i] != u16LastQuantization[i]){
                    u8QuantizationGateFlag = TRUE;
                }
                break;
            case OCTAVE:
                u16CurrentPosition[i] = scaleSearch(u16OctaveScale, u16temp, LENGTH_OF_OCTAVE_SCALE);
                if(u16CurrentPosition[i] != u16LastQuantization[i]){
                    u8QuantizationGateFlag = TRUE;
                }
                break;
            default:
                break;
        }

        u16LastQuantization[i] = u16CurrentPosition[i];
   }

    p_pos_and_gesture_struct->u16XPosition = u16CurrentPosition[0];
    p_pos_and_gesture_struct->u16YPosition = u16CurrentPosition[1];
    p_pos_and_gesture_struct->u16ZPosition = u16CurrentPosition[2];

}

/*Execute the modulation routine. This routine only runs when a cable is plugged
  into the modulation jack. Incoming data is 12 bit.
 */
void runModulation(pos_and_gesture_data * p_pos_and_gesture_struct){

    static uint16_t u16ADCData = 0;
    static uint16_t u16LastADCData;
    uint16_t u16Temp = 0;
    uint32_t u32PlaybackSpeed;
    uint32_t u32NewSequenceEndAddress;
    uint32_t u32SequenceLength;
    uint32_t u32NewReadAddress;
    uint8_t u8InversionFlag;
    uint16_t u16Attenuation;
    uint16_t u16CurrentPosition[NUM_OF_AXES];
    uint32_t u32PositionTemp;
    uint16_t u16Divisor;
    uint16_t u16Modulus;
    int i;

    if(!MODULATION_JACK_PLUGGED_IN){
        u16ModulationOnFlag = FALSE;
    }

    /*Get the ADC data and make some change based on it.*/
    if(xQueueReceive(xADCQueue, &u16ADCData, 0)){
        
            switch(p_VectrData->u8ModulationMode){
                case SPEED:
                    if(u16ADCData != u16LastADCData){
                        adjustSpeedModulation(u16ADCData>>2);
                    }
                    break;
                case QUANTIZED_SPEED:
                    if(u16ADCData != u16LastADCData){
                        //There are 16 speeds. Reduce down to 4 bits.
                        u16Temp = u16ADCData>>8;
                        u32PlaybackSpeed = u32QuantizedSpeedTable[u16Temp];
                        SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
                        RESET_SAMPLE_TIMER;
                        /*Adjust the clock timer accordingly.*/
                        calculateClockTimer(u32PlaybackSpeed);
                    }
                    break;
                case SCRUB:
                    /*Play the sequence with CV. We must know the length
                     * of the sequence. Then we can scale to a memory location and set that
                     * as the next read location.
                     * With RAM this is straightforward.
                     * For Flash, the location has to be translated to a sector.
                     * If the ADC value went down, then playback is going backwards.
                     * If the ADC value went up, then the playback is going forwards.
                     */

                    STOP_CLOCK_TIMER;

                    u32SequenceLength = getActiveSequenceLength();


                    /*The new sequence position is the determined by the ADC reading.*/
                    u32NewReadAddress = u32SequenceLength*u16ADCData;
                    u32NewReadAddress >>= 12;//ADC has 4096 levels, 12 bits.

                    /*Check if it's time for a clock pulse.*/
                       
                    u16Divisor = u32NewReadAddress>>p_VectrData->u8NumClocks;

                    if(u16Divisor != 0){
                        u16Modulus = u32NewReadAddress%u16Divisor;
                    }else{
                        u16Modulus = 0;
                    }

                    if(u16Modulus <= 3){
                        SET_LOOP_SYNC_OUT;
                    }
                    else{
                        CLEAR_LOOP_SYNC_OUT;
                    }

                    /*Set the new read position.*/
                    setNewReadAddress(u32NewReadAddress);

                    break;
                case TRIM:
                    /*0-5V We need to know the total length of the sequence, not
                     * the modified length.
                     * 0V trims the end all the way to the beginning.
                     * 5V trims the end all the way to the end.
                     * Linear in between.
                     */
                    u32SequenceLength = getActiveSequenceLength();
                    u32NewSequenceEndAddress = u32SequenceLength*u16ADCData;
                    u32NewSequenceEndAddress >>= 12;//ADC has 4096 levels, 12 bits.

                    /*Set the new end.*/
                    setNewSequenceEndAddress(u32NewSequenceEndAddress);

                    /*Adjust the clock timer accordingly.*/
                    calculateClockTimer(u32PlaybackSpeed);

                    break;
                case MIRROR:
                    /*OV = No modification.
                     2.5V = Maximum attenuation.
                     5V = Total inversion.
                     So the amount of attenuation depends on how close to 2.5V
                     and the inversion happens at 2.5V.
                     For each axis.*/
                    u16CurrentPosition[0] = p_pos_and_gesture_struct->u16XPosition;
                    u16CurrentPosition[1] = p_pos_and_gesture_struct->u16YPosition;
                    u16CurrentPosition[2] = p_pos_and_gesture_struct->u16ZPosition;

                    if(u16ADCData > HALF_ADC_VALUE){
                        u8InversionFlag = TRUE;
                        u16Attenuation = u16ADCData - HALF_ADC_VALUE;
                    }
                    else{
                        u8InversionFlag = FALSE;
                        u16Attenuation = HALF_ADC_VALUE - u16ADCData;//Attenuate less toward the bottom.
                    }

                    for(i=0; i<NUM_OF_AXES;i++){
                        
                        if(u16CurrentPosition[i] > HALF_OUTPUT_VALUE){
                            u32PositionTemp = u16CurrentPosition[i] - HALF_OUTPUT_VALUE;
                        }
                        else{
                            u32PositionTemp = HALF_OUTPUT_VALUE - u16CurrentPosition[i];
                        }
                        u32PositionTemp *= u16Attenuation;
                        u32PositionTemp >>= 11;
                        if(u8InversionFlag == TRUE){
                            if(u16CurrentPosition[i] > HALF_OUTPUT_VALUE){
                                u32PositionTemp = HALF_OUTPUT_VALUE - u32PositionTemp;
                            }
                            else{
                                u32PositionTemp = HALF_OUTPUT_VALUE + u32PositionTemp;
                            }
                        }
                        else{
                            if(u16CurrentPosition[i] > HALF_OUTPUT_VALUE){
                                u32PositionTemp = HALF_OUTPUT_VALUE + u32PositionTemp;
                            }
                            else{
                                u32PositionTemp = HALF_OUTPUT_VALUE - u32PositionTemp;
                            }
                        }
                        u16CurrentPosition[i] = u32PositionTemp;

                    }

                    p_pos_and_gesture_struct->u16XPosition = u16CurrentPosition[0];
                    p_pos_and_gesture_struct->u16YPosition = u16CurrentPosition[1];
                    p_pos_and_gesture_struct->u16ZPosition = u16CurrentPosition[2];

                    break;
                default:
                    break;
            }

            u16LastADCData = u16ADCData;
        }

}

uint16_t scaleSearch(const uint16_t *p_scale, uint16_t u16Position, uint8_t u8Length){
   uint8_t u8CurrentIndex = 0;
   uint8_t u8LastIndex = 0;
   uint16_t u16temp = u16Position;
   uint16_t u16Spread;
   uint16_t u16Difference;

   while(u16temp > p_scale[u8CurrentIndex]){
       u8LastIndex = u8CurrentIndex;
       u8CurrentIndex++;
   }

   u16Spread = p_scale[u8CurrentIndex] - p_scale[u8LastIndex];
   u16Spread >>= 1;
   u16Difference = p_scale[u8CurrentIndex] - u16Position;

   if(u16Difference < u16Spread){
       return p_scale[u8CurrentIndex];
   }
   else{
       return p_scale[u8LastIndex];
   }
}

uint16_t scaleBinarySearch(const uint16_t *p_scale, uint16_t u16Position, uint8_t u8Length){
    uint8_t u8CurrentIndex = u8Length>>1;//Start with the middle of the array.
    uint8_t u8LastIndex = 0;
    uint8_t u8SearchSize = 8;
    
    while(u8SearchSize > 0){
        
        //Get the search size
        if(u8CurrentIndex > u8LastIndex){
           u8SearchSize = u8CurrentIndex - u8LastIndex;
        }
        else{
            u8SearchSize = u8LastIndex - u8CurrentIndex;
        }
        u8SearchSize >>=1;

        u8LastIndex = u8CurrentIndex;

        if(u16Position > p_scale[u8CurrentIndex]){
            if(u8SearchSize != 0){
                u8CurrentIndex += u8SearchSize;
            }
            else{
                u8LastIndex++;
            }

        }else if(u16Position < p_scale[u8CurrentIndex]){
            if(u8SearchSize != 0){
                u8CurrentIndex -= u8SearchSize;
            }
            else{
                u8LastIndex--;
            }
        }else{
            //Spot on!
            u8SearchSize = 0;
            u8LastIndex = u8CurrentIndex + 1;
        }
    }

    //Now figure out which one it's closest to.
    if(u8CurrentIndex > u8LastIndex){
        if((p_scale[u8CurrentIndex] - u16Position) > (u16Position - p_scale[u8LastIndex])){
            return p_scale[u8LastIndex];
        }
        else{
            return p_scale[u8CurrentIndex];
        }
    }
    else{
        if((p_scale[u8LastIndex] - u16Position) > (u16Position - p_scale[u8CurrentIndex])){
            return p_scale[u8CurrentIndex];
        }
        else{
            return p_scale[u8LastIndex];
        }
    }
}

uint8_t getPlaybackDirection(void){
    return p_VectrData->u8PlaybackDirection;
}

void setPlaybackDirection(uint8_t u8NewDirection){
    uint8_t u8OldDirection = p_VectrData->u8PlaybackDirection;

    p_VectrData->u8PlaybackDirection = u8NewDirection;
    /*If playback is coming out of flash, then the end of the sector needs
     to be adjusted for the new playback direction.*/
    if(getStoredSequenceLocationFlag() == STORED_IN_FLASH && (u8OldDirection != u8NewDirection)){
        changeFlashPlaybackDirection(u8NewDirection);
    }
}

uint8_t getTimeQuantizationStatus(uint8_t u8Index){
    return p_VectrData->u8TimeQuantization[u8Index];
}

uint8_t getPlaybackMode(void){
    return p_VectrData->u8PlaybackMode;
}

uint8_t getRecordSource(void){
    return p_VectrData->u8Source[RECORD];
}

/*This function is executed during playback when a sync pulse arrives.*/
void syncHandler(void){
    /*Different modes for the different playback modes.
     * 1. Looping - Sync pulse causes playback to start over at the beginning of the recording.
     * 2. Pendulum - Sync pulse causes playback to start over at the beginning of the recording.
     * 3. Flip - Sync pulse causes playback to change direction
     * 4. One-shot -  Sync pulse causes playback to start over at the beginning of the recording.
     * 5. Retrigger - Sync pulse causes playback to start over at the beginning of the recording. Causes 
     */
    switch(p_VectrData->u8PlaybackMode){
        case LOOPING:
        case PENDULUM:
            //Reset the RAM read address.
            setRAMRetriggerFlag();
            break;
        case ONESHOT:
            setRAMRetriggerFlag();
            break;
        case RETRIGGER:
            setRAMRetriggerFlag();
            //And start playback.
            u8PlaybackRunFlag = RUN;
            break;
        case FLIP:
            setPlaybackDirection(p_VectrData->u8PlaybackDirection ^= REVERSE_PLAYBACK);
            break;
    }
}

uint8_t getHoldState(void){
    return u8HoldState;
}

void holdHandler(pos_and_gesture_data * p_position_data_struct, pos_and_gesture_data * p_memory_data_struct,
pos_and_gesture_data * p_hold_data_struct){
    int i;
    uint8_t u8HoldMode;
    uint16_t * p_u16Position;
    uint16_t * p_u16HoldPosition;
    uint16_t * p_u16MemoryPosition;

    if(p_position_data_struct != NULL){
        p_u16Position = &p_position_data_struct->u16XPosition;
    }

    if(p_hold_data_struct != NULL){
       p_u16HoldPosition = &p_hold_data_struct->u16XPosition;
    }
    else{
       p_u16HoldPosition = p_u16Position;
    }

    if(p_memory_data_struct != NULL){
        p_u16MemoryPosition = &p_memory_data_struct->u16XPosition;
    }
    else{
        p_u16MemoryPosition = p_u16Position;
    }

    /*There are behavioral differences for the hold mode depending on which master state
     we are in and whether playback is running or not.
     In live play mode, a received hold command will hold the outputs.*/
    switch(u8OperatingMode){
        case LIVE_PLAY:
        case RECORDING:
            /*Go through each of the axes and perform the hold behavior for that axis */
            for(i=0; i<NUM_OF_AXES; i++){
                u8HoldMode = p_VectrData->u8HoldBehavior[i];

                switch(u8HoldMode){
                    case HOLD:
                    case TRACKLIVE:
                    case LIVE:
                        *(p_u16Position + i) = *(p_u16HoldPosition + i);
                        break;
                    case ZERO:
                    case ENVELOPE_ZERO:
                        *(p_u16Position + i) = 0;
                        break;
                    default:
                        break;
                }
            }
            break;
        case PLAYBACK:
        case SEQUENCING:
        case AIR_SCRATCHING:
        case MUTING:
            for(i=0; i<NUM_OF_AXES; i++){
                u8HoldMode = p_VectrData->u8HoldBehavior[i];
                if(u8PlaybackRunFlag == RUN){
                    /*If playback is running, then the behavior is the same for all except
                     the track/live mode where the output becomes live.*/
                    if(u8HoldMode == ZERO){
                        *p_u16MemoryPosition = 0;
                    }
                    else if(u8HoldMode != TRACKLIVE){
                        *p_u16MemoryPosition = *p_u16HoldPosition;
                    }
                    else{
                        *p_u16MemoryPosition = *p_u16Position;
                    }
                }else{
                    switch(u8HoldMode){
                        case HOLD:
                            /*Do nothing. Play is stopped, new values will not be retrieved from RAM*/
                            break;
                        case TRACKLIVE:
                        case LIVE:
                            //Overwrite the memory data with the current live data.
                            *p_u16MemoryPosition = *p_u16Position;
                            break;
                        case ZERO:
                            *p_u16MemoryPosition = 0;
                            break;
                        case ENVELOPE_ZERO:
                            /*If playback has ended, then 0V, otherwise, do nothing.*/
                            if(u8PlaybackRunFlag == ENDED){
                              *p_u16MemoryPosition = 0;
                            }
                            else{
                              *p_u16MemoryPosition = *p_u16HoldPosition;
                            }
                            break;
                        default:
                            break;
                    }
                }
                p_u16MemoryPosition++;
                p_u16HoldPosition++;
                p_u16Position++;
            }
            break;
        case OVERDUBBING:
            if(u8OverdubRunFlag == TRUE){
                if(u8HoldMode == ZERO){
                    *(p_u16MemoryPosition + i) = 0;
                }
                else if(u8HoldMode != TRACKLIVE){
                    *(p_u16MemoryPosition + i) = *(p_u16HoldPosition + i);
                }
                else{
                    *(p_u16MemoryPosition + i) = *(p_u16Position + i);
                }
            }
            else{
                switch(u8HoldMode){
                        case HOLD:
                            /*Do nothing. Play is stopped, new values will not be retrieved from RAM*/
                            break;
                        case TRACKLIVE:
                        case LIVE:
                            //Overwrite the memory data with the current live data.
                            *(p_u16MemoryPosition + i) = *(p_u16Position + i);
                            break;
                        case ZERO:
                            *(p_u16Position + i) = 0;
                            break;
                        case ENVELOPE_ZERO:
                            /*If playback has ended, then 0V, otherwise, do nothing.*/
                            if(u8PlaybackRunFlag == ENDED){
                                *(p_u16Position + i) = 0;
                            }
                            break;
                        default:
                            break;
                    }
            }
            break;
        default:
            break;
    }
}

void setLivePlayActivationFlag(void){
    u8LivePlayActivationFlag = TRUE;
}

/*Start recording from scratch.*/
void startNewRecording(void){
    
    u8OperatingMode = RECORDING;
    u8RecordRunFlag = TRUE;
    u8PlaybackRunFlag = FALSE;
    resetRAMWriteAddress();
    memBuffer.sample_1.u16XPosition = pos_and_gesture_struct.u16XPosition;
    memBuffer.sample_1.u16YPosition = pos_and_gesture_struct.u16YPosition;
    memBuffer.sample_1.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
    memBuffer.sample_2.u16XPosition = pos_and_gesture_struct.u16XPosition;
    memBuffer.sample_2.u16YPosition = pos_and_gesture_struct.u16YPosition;
    memBuffer.sample_2.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
    u8BufferDataCount = 1;
    u8ExternalAirWheelActiveFlag = FALSE;
    u8ExternalAirwheelClockDelay = 0;

    setSwitchLEDState(SWITCH_LED_RED_BLINKING); 

    /*In external record mode, the clock must be handled. We start running the clock
     * because we need to know how long it is between clock edges.*/
    if(p_VectrData->u8Control[RECORD] != GATE && p_VectrData->u8Source[RECORD] == EXTERNAL){
        u8RecordingArmedFlag = NOT_ARMED;
        resetInputClockHandling();
        setClockEnableFlag(TRUE);
        START_CLOCK_TIMER;
        resetSpeed();//Reset the playback speed to default. Also resets the clock timer, Timer3.
    }
    else if(u8TapTempoSetFlag == TRUE){
        /*Internal recording mode.
         * If a tempo has been set with tap tempo, then set up to count clock edges
         * and activate the clock output.
         * If tap tempo has not been set, then a basic tempo is set just to flash the
         * switch LED.
         */
        resetInputClockHandling();
    }
    else{
        //Internal recording, no tap tempo.
        resetSpeed();//Reset the playback speed to default. Also resets the clock timer, Timer3.
    }
}

/*End a recording. If automatic play triggering is enabled, go to playback.*/
void finishRecording(void){
    int i;

    u8RecordRunFlag = FALSE;
    u8HoldState = OFF;
    u8SequenceRecordedFlag = TRUE;
    setStoredSequenceLocationFlag(STORED_IN_RAM);
    setPlaybackDirection(FORWARD_PLAYBACK);
    
    if((u8TapTempoSetFlag == FALSE) ){
        calculateClockTimer(u32PlaybackSpeed);
    }

    if(p_VectrData->u8Source[RECORD] == EXTERNAL && p_VectrData->u8Control[RECORD] != GATE
       || u8TapTempoSetFlag == TRUE){
        u8ClockLengthOfRecordedSequence = u8CurrentInputClockCount;
        setAvgNumClicksBetweenClocks(calculateAvgNumClicksBetweenClocks());
        u32TargetNumTicksBetweenClocks = u32AvgNumTicksBetweenClocks;
        u8CurrentInputClockCount = 1;
    }else{
        //Free running switch recording. We have to calculate the clock data.
        u8ClockLengthOfRecordedSequence = 1<<p_VectrData->u8NumClocks;
        u32TargetNumTicksBetweenClocks = getActiveSequenceLength()/6;//total sequence length/clock length
        u32TargetNumTicksBetweenClocks >>= p_VectrData->u8NumClocks;//Divided by the number of clock pulses.
        for(i=0; i< LENGTH_OF_INPUT_CLOCK_ARRAY; i++){
            u32NumTicksBetweenClocksArray[i] = u32TargetNumTicksBetweenClocks;
        }
    }
                 
    if(p_VectrData->u8Control[PLAY] == TRIGGER_AUTO){
        //Initiate a read.
        resetRAMReadAddress();
        u8BufferDataCount = 0;
        u8OperatingMode = PLAYBACK;
        setPlaybackRunStatus(RUN);
    }
    else{//Regular trigger mode ends recording without starting playback and stays in record mode.
        u8PlaybackRunFlag = ENDED;
    }
}

void setHoldPosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    if(p_pos_and_gesture_struct != NULL){
        hold_position_struct.u16XPosition = p_pos_and_gesture_struct->u16XPosition;
        hold_position_struct.u16YPosition = p_pos_and_gesture_struct->u16YPosition;
        hold_position_struct.u16ZPosition = p_pos_and_gesture_struct->u16ZPosition;
    }
}

void armRecording(void){
    if(u8RecordingArmedFlag != ARMED){
        u8RecordingArmedFlag = ARMED;
        setSwitchLEDState(SWITCH_LED_RED);
    }else{
        u8RecordingArmedFlag = NOT_ARMED;
        if(u8PlaybackRunFlag == TRUE){
            setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
        }
        else{
            setSwitchLEDState(SWITCH_LED_OFF);
        }
    }

}

void armPlayback(void){
    if(u8PlaybackArmedFlag != ARMED){
        u8PlaybackArmedFlag = ARMED;
        setSwitchLEDState(SWITCH_LED_GREEN);
    }else{
        u8PlaybackArmedFlag = NOT_ARMED;
        if(u8RecordRunFlag == TRUE){
            setSwitchLEDState(SWITCH_LED_RED_BLINKING);
        }
        else{
            setSwitchLEDState(SWITCH_LED_OFF);
        }
    }
}

void armOverdub(void){
    if(u8OverdubArmedFlag != ARMED){
        u8OverdubArmedFlag = ARMED;
        setSwitchLEDState(SWITCH_LED_RED);
    }
    else{
        u8OverdubArmedFlag = NOT_ARMED;
        //Handle the switch LED to indicate whether playback is running.
        if(u8OverdubRunFlag == TRUE){
           setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
        }
        else{
           setSwitchLEDState(SWITCH_LED_OFF);
        }
    }
}

void disarmRecording(void){
    u8RecordingArmedFlag = DISARMED;
}

void disarmPlayback(void){
    u8PlaybackArmedFlag = DISARMED;
}

void disarmOverdub(void){
    u8OverdubArmedFlag = DISARMED;
}

void switchStateMachine(void){
    uint8_t u8NewSwitchState;

    //If a switch was pressed or released, handle it
    if(xQueueReceive(xSwitchQueue, &u8NewSwitchState, 0)){
        switch(u8OperatingMode){
        case LIVE_PLAY:
            switch(u8NewSwitchState){
            case ENC_SWITCH_PRESSED://LIVE PLAY
               if(u8MenuModeFlag == FALSE){
                    /*If a sequence has been recorded move to playback, regardless of the
                     * trigger configuration
                     */
                    if(u8SequenceRecordedFlag == TRUE){
                        if(p_VectrData->u8Source[PLAY] == SWITCH){
                            u8OperatingMode = PLAYBACK;
                            setPlaybackRunStatus(RUN);
                            u8HoldState = OFF;//Turn off hold to start playback.
                            u8HoldActivationFlag = HOLD_DEACTIVATE;
                            setRAMRetriggerFlag();
                        }
                        else{//external control = arm playback
                            if(u8PlaybackArmedFlag != ARMED){
                                armPlayback();
                            }
                            else{
                                disarmPlayback();
                            }
                        }
                    }
                    else{
                        //indicate error.
                        LEDIndicateError();
                    }
                }else{
                   //Menu mode encoder switch press navigates the menu.
                    setMenuKeyPressFlag();
                }
                break;
            case MAIN_SWITCH_PRESSED://LIVE PLAY
               //If we're in menu mode, main switch is exit.
               if(u8MenuModeFlag == FALSE){
                   if(u8TapTempoModeActiveFlag == FALSE){
                       if(p_VectrData->u8Source[RECORD] == SWITCH && u8TapTempoSetFlag == FALSE){
                           //If recording is internal and free flowing, start recording.
                           u8OperatingMode = RECORDING;
                           startNewRecording();
                       }else{
                           //Go to record mode and wait for the trigger?
                           armRecording();
                       }
                   }
                   else{
                       u8TapTempoKeyPressFlag = TRUE;
                   }
               }
               else{
                   setMenuExitFlag();
               }
                break;
            case ENC_SWITCH_RELEASED://LIVE PLAY
                break;
            case MAIN_SWITCH_RELEASED://LIVE PLAY
                break;
            default:
                break;
            }
        
            break;
         case RECORDING:
            switch(u8NewSwitchState){
            case ENC_SWITCH_PRESSED://RECORDING
                /* PLAY/SWITCH/TRIGGER - Start playback immediately, stop recording
                 * PLAY/SWITCH/GATE - Start playback immediately, stop recording
                 * PLAY/SWITCH/TRIGGERAUTO - Start playback immediately, stop recording
                 * PLAY/EXT/TRIGGER - Arm playback
                 * PLAY/EXT/GATE - Arm playback
                 * PLAY/EXT/TRIGGERAUTO - Start playback immediately, stop recording
                 */
                if(u8MenuModeFlag == FALSE){
                    if(p_VectrData->u8Source[PLAY] == SWITCH || p_VectrData->u8Control[PLAY] == TRIGGER_AUTO){
                        u8OperatingMode = PLAYBACK;
                        u8PlaybackRunFlag = RUN;
                        resetRAMReadAddress();//Recording ends go to the beginning of the sequence
                        u8HoldActivationFlag = HOLD_DEACTIVATE;
                    }
                    else{//external control = arm playback


                        if(u8PlaybackArmedFlag != ARMED){
                            armPlayback();
                        }
                        else{
                            disarmPlayback();
                        }
                    }
                }else{
                    //Menu mode encoder switch press navigates the menu.
                    setMenuKeyPressFlag();
                }
                break;
            case MAIN_SWITCH_PRESSED://RECORDING
                /* REC/SWITCH/TRIGGER - Stop recording on main switch press
                 * REC/SWITCH/GATE - Start recording on main switch press
                 * REC/EXT/TRIGGER - Arm/disarm recording
                 * REC/EXT/GATE - Arm/disarm recording
                 */
                if(u8MenuModeFlag == FALSE){
                    if(p_VectrData->u8Source[RECORD] == SWITCH){
                        //Problem here for ending recording with tap tempo.
                        if(u8TapTempoSetFlag == FALSE){

                            if(u8RecordRunFlag == TRUE){
                                finishRecording();
                            }
                            else{
                                startNewRecording();
                            }
                        }else{
                            if(u8RecordRunFlag == FALSE){
                                armRecording();
                            }
                            else{
                                if(p_VectrData->u8Control[RECORD] != TRIGGER_AUTO){
                                    disarmRecording();
                                }
                            }
                        }
                    }
                    else{
                        /*If recording is running, we disarm.
                         If recording is not running, we arm it.*/
                        //If the mode is set to gate, then recording will either be armed or disarmed.
                        if(p_VectrData->u8Control[RECORD] == GATE){
                            if(u8RecordingArmedFlag == DISARMED){
                                armRecording();
                            }else{
                                disarmRecording();
                            }  
                        }else{//Trigger Recording
                            if(u8RecordRunFlag == FALSE){
                                armRecording();
                            }
                            else{
                                if(p_VectrData->u8Control[RECORD] != TRIGGER_AUTO){
                                    disarmRecording();
                                }
                            }
                        }
                    }
                }else{
                    setMenuExitFlag();
                }
                break;
            case ENC_SWITCH_RELEASED://RECORDING
                //Do nothing
                break;
            case MAIN_SWITCH_RELEASED://RECORDING
                /*REC/SWITCH/GATE - Stop recording
                 */
                /*Switch release only matters for gate record mode.
                  In gate mode, the release of the switch end recording.*/
                if(p_VectrData->u8Control[RECORD] == GATE && p_VectrData->u8Source[RECORD] == SWITCH){
                    finishRecording();
                }
                break;
            default:
                break;
            }
            break;
        case PLAYBACK:
            switch(u8NewSwitchState){
            case ENC_SWITCH_PRESSED://PLAYBACK
                /* PLAY/SWITCH/TRIGGER - Toggle playback on/off
                 * PLAY/SWITCH/GATE - Start playback immediately
                 * PLAY/SWITCH/TRIGGERAUTO - Toggle playback on/off
                 * PLAY/EXT/TRIGGER - Arm/disarm playback
                 * PLAY/EXT/GATE - Arm/disarm playback
                 * PLAY/EXT/TRIGGERAUTO - Arm/disarm playback
                 */
                if(u8MenuModeFlag == FALSE){
                    /* If playback is paused because the clock disappeared in externally controlled recording
                     * then pressing the switch will cause the loop to start over from the beginning.
                     * This is a way to keep loops and other things synchronized.
                     */
                    if(u8PlaybackRunFlag == PAUSED){
                        //Restart the loop.
                        setRAMRetriggerFlag();
                    }else{
                        if(p_VectrData->u8Source[PLAY] == SWITCH){
                            if(p_VectrData->u8PlaybackMode != FLIP &&
                               p_VectrData->u8PlaybackMode != RETRIGGER){
                                if(u8PlaybackRunFlag != RUN){
                                    u8HoldActivationFlag = HOLD_DEACTIVATE;
                                    setPlaybackRunStatus(RUN);
                                    u8HoldState = OFF;//Turn off any hold.
                                }
                                else{
                                    setPlaybackRunStatus(STOP);
                                }
                            }
                            else if(p_VectrData->u8PlaybackMode == FLIP){//Flip playback mode
                                if(p_VectrData->u8PlaybackDirection == FORWARD_PLAYBACK){
                                    setPlaybackDirection(REVERSE_PLAYBACK);
                                }
                                else{
                                    setPlaybackDirection(FORWARD_PLAYBACK);
                                }
                            }
                            else{
                                //This if for one-shot mode and retrigger mode.
                                if(p_VectrData->u8PlaybackMode == FLIP || u8PlaybackRunFlag == ENDED){
                                    setRAMRetriggerFlag();
                                    //Retrigger playback mode.
                                    setPlaybackRunStatus(RUN);
                                }
                                else{//One-shot mode
                                    if(u8PlaybackRunFlag == RUN){
                                      setPlaybackRunStatus(STOP);
                                    }
                                    else{
                                       setPlaybackRunStatus(RUN);
                                    }
                                }

                            }

                        }else{//external control = arm playback
                            /*If recording is running, we disarm.
                             If recording is not running, we arm it.
                             If the playback mode is flip, then we only arm playback
                             to cause the direction to flip with the next trigger.*/
                            if(p_VectrData->u8PlaybackMode != FLIP){
                                if(p_VectrData->u8Control[PLAY] != GATE){
                                    if(u8PlaybackRunFlag != RUN){
                                        armPlayback();
                                    }
                                    else{
                                        disarmPlayback();
                                    }
                                }else{
                                    if(u8PlaybackArmedFlag == DISARMED){
                                       u8PlaybackArmedFlag = ARMED;
                                    }else{
                                       u8PlaybackArmedFlag = DISARMED;
                                    }
                                }
                            }else{
                                armPlayback();
                            }

                        }
                    }
                }else{
                    setMenuKeyPressFlag();
                }
                break;
            case MAIN_SWITCH_PRESSED://PLAYBACK
                /* REC/SWITCH/TRIGGER - Start recording
                 * REC/SWITCH/GATE - Start recording
                 * REC/EXT/TRIGGER - Arm recording
                 * REC/EXT/GATE - Arm recording
                 */
                if(u8MenuModeFlag == FALSE){
                   if(u8TapTempoModeActiveFlag == FALSE){
                       if(p_VectrData->u8Source[RECORD] == SWITCH && u8TapTempoSetFlag == FALSE){
                           //If recording is internal and free flowing, start recording.
                           u8OperatingMode = RECORDING;
                           startNewRecording();
                       }else{
                           //Go to record mode and wait for the trigger?
                           armRecording();
                       }
                   }
                   else{
                       u8TapTempoKeyPressFlag = TRUE;
                   }
               }
               else{
                   setMenuExitFlag();
               }
                break;
            case ENC_SWITCH_RELEASED://PLAYBACK
                /*Switch release only matters for gate record mode.
                  In gate mode, the release of the switch end recording.*/
                if(p_VectrData->u8Control[PLAY] == GATE && p_VectrData->u8Source[PLAY] == SWITCH){
                    u8PlaybackRunFlag = STOP;
                    setSwitchLEDState(SWITCH_LED_OFF);
                }
                break;
            case MAIN_SWITCH_RELEASED://PLAYBACK
                break;
            default:
                break;
            }
        break;
        case OVERDUBBING:
            switch(u8NewSwitchState){
            case ENC_SWITCH_PRESSED://OVERDUBBING
                //Encoder switch press during overdub can start and stop playback
                if(u8OverdubRunFlag != PAUSED){
                    u8OverdubRunFlag ^= RUN;

                    if(u8OverdubRunFlag == RUN){
                        u8HoldState = OFF;//Turn off any hold
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                    }
                    else{
                        setSwitchLEDState(OFF);
                    }
                }
                else{
                    /*An encoder press will restart the */
                    setRAMRetriggerFlag();
                }
                break;
            case MAIN_SWITCH_PRESSED://OVERDUBBING
                //Main switch press activates/deactivates overdub
                if(p_VectrData->u8Source[OVERDUB] == SWITCH){
                    Flags.u8OverdubActiveFlag ^= TRUE;
                    if(Flags.u8OverdubActiveFlag == TRUE){
                        setLEDAlternateFuncFlag(FALSE);
                        setIndicateOverdubModeFlag(FALSE);
                        turnOffAllLEDs();
                        setRAMWriteAddress(getRAMReadAddress());//synchronize write with read
                        setSwitchLEDState(SWITCH_LED_RED_BLINKING);
                    }
                    else{
                        setLEDAlternateFuncFlag(TRUE);
                        turnOffAllLEDs();
                        setIndicateOverdubModeFlag(TRUE);
                        indicateActiveAxes(OVERDUB);
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                    }
                }else{//External Control
                    if(Flags.u8OverdubActiveFlag == TRUE){
                        disarmOverdub();
                    }
                    else{
                        armOverdub();
                    }
                }  
                break;
            case ENC_SWITCH_RELEASED://OVERDUBBING
                break;
            case MAIN_SWITCH_RELEASED://OVERDUBBING
                /*Switch release only matters for gate record mode.
                  In gate mode, the release of the switch end recording.*/
                if(p_VectrData->u8Control[OVERDUB] == GATE && p_VectrData->u8Source[OVERDUB] == SWITCH){
                    Flags.u8OverdubActiveFlag = FALSE;
                    setSwitchLEDState(SWITCH_LED_OFF);

                    if(u8OverdubRunFlag == TRUE){
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                    }
                    else{
                        setSwitchLEDState(OFF);
                    }
                }
                break;
            }
       break;
       case SEQUENCING:
            switch(u8NewSwitchState){
            case ENC_SWITCH_PRESSED://SEQUENCING
                if(u8MenuModeFlag == FALSE){
                    if(u8PlaybackRunFlag == PAUSED){
                        //Restart the loop.
                        setRAMRetriggerFlag();
                    }else{
                        if(p_VectrData->u8Source[PLAY] == SWITCH){
                            if(u8PlaybackRunFlag != RUN){
                                u8HoldState = OFF;
                                u8PlaybackRunFlag = RUN;
                            }
                            else{
                                u8PlaybackRunFlag = STOP;
                            }

                            if(u8PlaybackRunFlag == RUN){
                                setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                            }
                            else{
                                setSwitchLEDState(OFF);
                            }
                        }else{//external control = arm playback
                            /*If recording is running, we disarm.
                             If recording is not running, we arm it.*/
                            if(u8PlaybackRunFlag == RUN){
                                disarmPlayback();
                            }
                            else{
                                armPlayback();
                            }
                        }
                    }
                }else{
                    setMenuKeyPressFlag();
                }
                break;
            case MAIN_SWITCH_PRESSED://SEQUENCING
                //Main switch press changes the active sequence.
                u8SequenceModeChangeSequenceFlag = TRUE;
                break;
            case ENC_SWITCH_RELEASED://SEQUENCING
            break;
            case MAIN_SWITCH_RELEASED://SEQUENCING
            break;

            }
        break;
        case AIR_SCRATCHING:
            switch(u8NewSwitchState){
            case ENC_SWITCH_PRESSED://SEQUENCING
                if(u8MenuModeFlag == FALSE){
                    if(u8PlaybackRunFlag == PAUSED){
                        //Restart the loop.
                        setRAMRetriggerFlag();
                    }else{
                        if(p_VectrData->u8Source[PLAY] == SWITCH){
                            if(u8PlaybackRunFlag != RUN){
                                u8HoldState = OFF;
                                u8PlaybackRunFlag = RUN;
                            }
                            else{
                                u8PlaybackRunFlag = STOP;
                            }

                            if(u8PlaybackRunFlag == RUN){
                                setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                            }
                            else{
                                setSwitchLEDState(OFF);
                            }
                        }else{//external control = arm playback
                            /*If recording is running, we disarm.
                             If recording is not running, we arm it.*/
                            if(u8PlaybackRunFlag == RUN){
                                disarmPlayback();
                            }
                            else{
                                armPlayback();
                            }
                        }
                    }
                }else{
                    setMenuKeyPressFlag();
                }
                break;
            case MAIN_SWITCH_PRESSED://SEQUENCING
                //Main switch press activates or deactivates air scratching
                u8AirScratchRunFlag  ^= TRUE;
                break;
            case ENC_SWITCH_RELEASED://SEQUENCING
            break;
            case MAIN_SWITCH_RELEASED://SEQUENCING
            break;

            }
            break;

        default:
            break;
        }
    }
}

void resetSpeed(void){
    i16PlaybackData = 0;
    u16PlaybackSpeedTableIndex = ZERO_SPEED_INDEX;
    u32PlaybackSpeed = u32LogSpeedTable[u16PlaybackSpeedTableIndex];
    SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
    RESET_SAMPLE_TIMER;
    calculateClockTimer(u32PlaybackSpeed);
}

/*Adjust the playback speed. One revolution will */
void adjustSpeed(int16_t i16AirwheelData){
    int16_t i16OldAirwheelData = i16PlaybackData;
    int32_t i32Increment = 0;
    int16_t i16Step;

    i16PlaybackData += i16AirwheelData;


    i32Increment = SPEED_ADJUSTMENT_INCREMENT*i16AirwheelData;
    u32PlaybackSpeed -=  i32Increment;

    if(u32PlaybackSpeed < MINIMUM_SAMPLE_PERIOD){
        u32PlaybackSpeed = MINIMUM_SAMPLE_PERIOD;
        i16PlaybackData = i16OldAirwheelData;
    }
    else if(u32PlaybackSpeed > MAXIMUM_SAMPLE_PERIOD){
        u32PlaybackSpeed = MAXIMUM_SAMPLE_PERIOD;
        i16PlaybackData = i16OldAirwheelData;
    }


    SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
    RESET_SAMPLE_TIMER;
}

void adjustSpeedTable(int16_t i16AirwheelData){
    int16_t i16Temp;

    i16Temp = u16PlaybackSpeedTableIndex + i16AirwheelData;

    if(i16Temp > MINIMUM_SPEED_INDEX && i16Temp < MAXIMUM_SPEED_INDEX){
        u16PlaybackSpeedTableIndex = i16Temp;
        u32PlaybackSpeed = u32LogSpeedTable[u16PlaybackSpeedTableIndex];
        SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
        RESET_SAMPLE_TIMER;

        /*Adjust the clock timer accordingly.*/
        calculateClockTimer(u32PlaybackSpeed);
    }
}

void adjustSpeedModulation(uint16_t u16NewValue){
    u16PlaybackSpeedTableIndex = u16NewValue;
    u32PlaybackSpeed = u32LogSpeedTable[u16NewValue];
    SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
    RESET_SAMPLE_TIMER;

    /*Adjust the clock timer accordingly.*/
    calculateClockTimer(u32PlaybackSpeed);
}

uint8_t decodeXYZTouchGesture(uint16_t u16Data){
    if((u16Data & BOTTOM_TOUCH) == BOTTOM_TOUCH){
        return 255;
    }
    if((u16Data & X_TOUCH) == X_TOUCH){
        return X_OUTPUT_INDEX;
    }
    if((u16Data & Y_TOUCH) == Y_TOUCH){
        return Y_OUTPUT_INDEX;
    }
    if((u16Data & Z_TOUCH) == Z_TOUCH){
        return Z_OUTPUT_INDEX;
    }
    if((u16Data & CENTER_TOUCH) == CENTER_TOUCH){
        return CENTER_INDEX;
    }

    return 255;
}

void resetMultiTapGestureDetection(void){
    u8decodeMultiTapGestureState = WAITING_FOR_GESTURE;
}

uint8_t decodeMultiTapGestures(uint16_t u16TouchData){



    switch(u8decodeMultiTapGestureState){
        case WAITING_FOR_GESTURE:

            if(u16TouchData & MGC3130_TAP_RIGHT){
                //Start the timer to look for the left tap.
                u8MultiTapTriggerTimer = MULTI_TAP_TIMER_RESET;
                u8decodeMultiTapGestureState = FIRST_TAP_TEMPO_GESTURE_RECEIVED;
            }else if(u16TouchData & MGC3130_TAP_BOTTOM){
                //Start the timer to look for the left tap.
                u8MultiTapTriggerTimer = MULTI_TAP_TIMER_RESET;
                u8decodeMultiTapGestureState = FIRST_TIME_QUANT_GESTURE_RECEIVED;
            }

            break;
        case FIRST_TAP_TEMPO_GESTURE_RECEIVED:
            if(u16TouchData & MGC3130_TAP_LEFT){
                if(p_VectrData->u8Source[RECORD] == SWITCH){
                    if(u8TapTempoModeActiveFlag == FALSE){
                        u8TapTempoModeActiveFlag = TRUE;
                        //Turn on the Red switch LED
                        setSwitchLEDState(SWITCH_LED_RED);
                        resetTapTempoMode();
                    }
                    else{
                        u8TapTempoModeActiveFlag = FALSE;
                        setSwitchLEDState(SWITCH_LED_OFF);
                    }
                }
            }
            break;
        case FIRST_TIME_QUANT_GESTURE_RECEIVED:
            if(u16TouchData & MGC3130_TAP_LEFT){
                //Toggle X time quantization.
                VectrData.u8TimeQuantization[X_OUTPUT_INDEX] ^= 0x01;
                setLeftLEDs(MAX_BRIGHTNESS, ON);
                u8MultiTapTriggerTimer = 0;
                resetMultiTapGestureDetection();
                LEDIndicateError();//Temporary indication
            }
            else if(u16TouchData & MGC3130_TAP_TOP){
                //Toggle Y time quantization.
                VectrData.u8TimeQuantization[Y_OUTPUT_INDEX] ^= 0x01;
                setTopLEDs(MAX_BRIGHTNESS, ON);
                u8MultiTapTriggerTimer = 0;
                resetMultiTapGestureDetection();
                LEDIndicateError();//Temporary indication
            }
            else if(u16TouchData & MGC3130_TAP_RIGHT){
                //Toggle Z time quantization.
                VectrData.u8TimeQuantization[Z_OUTPUT_INDEX] ^= 0x01;
                setRightLEDs(MAX_BRIGHTNESS, ON);
                u8MultiTapTriggerTimer = 0;
                resetMultiTapGestureDetection();
                LEDIndicateError();//Temporary indication
            }
            break;
        default:
            break;
    }
}

uint8_t decodeDoubleTapGesture(uint16_t u16Data){

    if((u16Data & MENU_MODE_GESTURE) ==  MENU_MODE_GESTURE){
        return MENU_MODE;
    }
    if((u16Data & OVERDUB_MODE_GESTURE) == OVERDUB_MODE_GESTURE){
        return OVERDUB_MODE;
    }
    if((u16Data & SEQUENCER_MODE_GESTURE) == SEQUENCER_MODE_GESTURE){
        return SEQUENCER_MODE;
    }
    if((u16Data & MUTE_MODE_GESTURE) == MUTE_MODE_GESTURE){
        return MUTE_MODE;
    }
    if((u16Data & AIR_SCRATCH_MODE_GESTURE) == AIR_SCRATCH_MODE_GESTURE){
        return AIR_SCRATCH_MODE;
    }


    //No gesture
    return NO_PERFORMANCE_ACTIVE;
}

void indicateActiveAxes(uint8_t u8State){
    switch(u8State){
        case OVERDUB:
            if(p_VectrData->u8OverdubStatus[X_OUTPUT_INDEX]){
                setLeftLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setLeftLEDs(0, OFF);
            }
            if(p_VectrData->u8OverdubStatus[Y_OUTPUT_INDEX]){
                setTopLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setTopLEDs(0, OFF);
            }
            if(p_VectrData->u8OverdubStatus[Z_OUTPUT_INDEX]){
                setRightLEDs(MAX_BRIGHTNESS, ON);
            }
            else{
                setRightLEDs(0, OFF);
            }

            if(!p_VectrData->u8OverdubStatus[X_OUTPUT_INDEX] &&
               !p_VectrData->u8OverdubStatus[Y_OUTPUT_INDEX] &&
               !p_VectrData->u8OverdubStatus[Z_OUTPUT_INDEX]){
                setRedLEDs(256);
            }
            else{
                turnOffRedLEDs();
            }

            break;
        default:
            break;
    }
}

uint8_t getOverdubStatus(uint8_t u8Axis){
    return p_VectrData->u8OverdubStatus[u8Axis];
}

uint8_t getOverdubActiveFlag(void){
    return Flags.u8OverdubActiveFlag;
}

void setKeyPressFlag(void){
    u8KeyPressFlag = TRUE;
}

void setEncKeyPressFlag(void){
    u8EncKeyPressFlag = TRUE;
}

void setMenuModeFlag(uint8_t u8NewMode){
    u8MenuModeFlag = u8NewMode;
}

uint8_t getCurrentQuantization(uint8_t u8Index){
    return p_VectrData->u16Quantization[u8Index];
}

void setCurrentQuantization(uint8_t u8Index, uint16_t u16CurrentParameter){
    p_VectrData->u16Quantization[u8Index] = u16CurrentParameter;
}



/*This function uses the slew parameter to slow the rate of change of the position*/
void slewPosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    uint16_t u16XSlewRate  = p_VectrData->u16SlewRate[X_OUTPUT_INDEX];
    uint16_t u16YSlewRate  = p_VectrData->u16SlewRate[Y_OUTPUT_INDEX];
    uint16_t u16ZSlewRate  = p_VectrData->u16SlewRate[Z_OUTPUT_INDEX];
    uint16_t u16CurrentXPosition = p_VectrData->u16CurrentXPosition;
    uint16_t u16CurrentYPosition = p_VectrData->u16CurrentYPosition;
    uint16_t u16CurrentZPosition = p_VectrData->u16CurrentZPosition;
    uint16_t u16NewXPosition,
             u16NewYPosition,
             u16NewZPosition;
    int32_t i32XDifference,
            i32YDifference,
            i32ZDifference;

    u16NewXPosition = p_pos_and_gesture_struct->u16XPosition;
    u16NewYPosition = p_pos_and_gesture_struct->u16YPosition;
    u16NewZPosition = p_pos_and_gesture_struct->u16ZPosition;

    i32XDifference = u16NewXPosition - u16CurrentXPosition;
    i32YDifference = u16NewYPosition - u16CurrentYPosition;
    i32ZDifference = u16NewZPosition - u16CurrentZPosition;

    if(i32XDifference > u16XSlewRate){
        u16NewXPosition = u16CurrentXPosition + u16XSlewRate;
    }
    else if(i32XDifference < -u16XSlewRate){
        u16NewXPosition = u16CurrentXPosition - u16XSlewRate;
    }

    if(i32YDifference > u16YSlewRate){
        u16NewYPosition = u16CurrentYPosition + u16YSlewRate;
    }
    else if(i32YDifference < -u16YSlewRate){
        u16NewYPosition = u16CurrentYPosition - u16YSlewRate;
    }

    if(i32ZDifference > u16ZSlewRate){
        u16NewZPosition = u16CurrentZPosition + u16ZSlewRate;
    }
    else if(i32ZDifference < -u16ZSlewRate){
        u16NewZPosition = u16CurrentZPosition - u16ZSlewRate;
    }

    p_VectrData->u16CurrentXPosition = u16NewXPosition;
    p_VectrData->u16CurrentYPosition = u16NewYPosition;
    p_VectrData->u16CurrentZPosition = u16NewZPosition;

    p_pos_and_gesture_struct->u16XPosition = u16NewXPosition;
    p_pos_and_gesture_struct->u16YPosition = u16NewYPosition;
    p_pos_and_gesture_struct->u16ZPosition = u16NewZPosition;

}


/*In this function we make a logarithmic or antilogarithmic curve to the output by blending
 together a log table with the linear value to make more or less logarithmic outputs. We
 basically take a fraction of each
 * (32*Linear value + 32*log table value / 64)  = half logarithmic*/
void linearizePosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    uint16_t u16Linearity[NUM_OF_AXES]  = {  p_VectrData->u16Linearity[X_OUTPUT_INDEX],
                                                p_VectrData->u16Linearity[Y_OUTPUT_INDEX],
                                                p_VectrData->u16Linearity[Z_OUTPUT_INDEX]};
    uint16_t u16CurrentPosition[NUM_OF_AXES] = {p_pos_and_gesture_struct->u16XPosition,
                                        p_pos_and_gesture_struct->u16YPosition,
                                        p_pos_and_gesture_struct->u16ZPosition};
    uint16_t u16NewPosition[NUM_OF_AXES];
    uint16_t u16LookupIndex;
    uint32_t u32Result;
    uint16_t u16AntiLogValue, u16LogValue;
    uint32_t u32LinearPortion, u32LogPortion;
    int i;

    for(i=0; i< NUMBER_OF_OUTPUTS;i++){
        //Logarithmic
        if(u16Linearity[i] < LINEARITY_STRAIGHT){
            u32LinearPortion = u16CurrentPosition[i]*(u16Linearity[i]);

            u16LookupIndex = u16CurrentPosition[i]>>BIT_SHIFT_TO_LOG_TABLE;
            u16LogValue = u16LogTable[u16LookupIndex];
            u32LogPortion = u16LogValue*(LINEARITY_STRAIGHT-u16Linearity[i]);

            u32Result = u32LinearPortion + u32LogPortion;
            u32Result >>= BIT_LENGTH_OF_LINEARITY_STRAIGHT;
            u16NewPosition[i] = u32Result;
        }
        else if(u16Linearity[i] > LINEARITY_STRAIGHT){
            //Antilogarithmic
            //Invert the table and then subtract it from the max value.
            u16LookupIndex = u16CurrentPosition[i]>>BIT_SHIFT_TO_LOG_TABLE;
            u16LookupIndex = LENGTH_OF_LOG_TABLE - u16LookupIndex;
            u16AntiLogValue = u16LogTable[u16LookupIndex];
            u16AntiLogValue = MAX_LOG_TABLE_VALUE - u16AntiLogValue;


            u32LinearPortion = (LINEARITY_MAX - u16Linearity[i]);
            u32LinearPortion *= u16CurrentPosition[i];
            u32LogPortion = (u16Linearity[i] - LINEARITY_STRAIGHT);
            u32LogPortion *= u16AntiLogValue;
            u32Result = u32LinearPortion + u32LogPortion;
            u32Result >>= BIT_LENGTH_OF_LINEARITY_STRAIGHT;
            u16NewPosition[i] = u32Result;
        }
        else{
            u16NewPosition[i] = u16CurrentPosition[i];
        }


    }

    p_pos_and_gesture_struct->u16XPosition = u16NewPosition[X_OUTPUT_INDEX];
    p_pos_and_gesture_struct->u16YPosition = u16NewPosition[Y_OUTPUT_INDEX];
    p_pos_and_gesture_struct->u16ZPosition = u16NewPosition[Z_OUTPUT_INDEX];
}

void recordData(pos_and_gesture_data * p_pos_and_gesture_struct){
    /*Write the data to RAM*/

}

void scaleRange(pos_and_gesture_data * p_pos_and_gesture_struct){
    int i;
    uint32_t u32temp;
    static uint16_t u16Ramp;

    uint16_t u16CurrentPosition[NUM_OF_AXES] = {p_pos_and_gesture_struct->u16XPosition,
                                        p_pos_and_gesture_struct->u16YPosition,
                                        p_pos_and_gesture_struct->u16ZPosition};

    for(i=0;i<NUMBER_OF_OUTPUTS;i++){

        u32temp = u16CurrentPosition[i];

        switch(p_VectrData->u8Range[i]){
            case RANGE_UNI_5V:
                u32temp >>= 1;
                u32temp += ZERO_VOLT_OUTPUT_VALUE;
                break;
            case RANGE_BI_5V://do nothing
                break;
            case RANGE_BI_2_5V:
                u32temp >>= 1;
                u32temp += QUARTER_OUTPUT_VALUE;
                break;
            case RANGE_UNI_1V:
                u32temp /= 10;
                u32temp += ZERO_VOLT_OUTPUT_VALUE;
                break;
            case RAMP:
                /*ramp from beginning of loop to end of loop
                 * 
                */
                if(u8SequenceRecordedFlag == TRUE){
                    u32temp = calculateRampOutput();
                }
                else{
                    u32temp = ZERO_VOLT_OUTPUT_VALUE;
                }
                
                break;
        }

        u16CurrentPosition[i] = u32temp;

    }

    p_pos_and_gesture_struct->u16XPosition = u16CurrentPosition[0];
    p_pos_and_gesture_struct->u16YPosition = u16CurrentPosition[1];
    p_pos_and_gesture_struct->u16ZPosition = u16CurrentPosition[2];

}

uint8_t * getDataStructAddress(void){
    return &p_VectrData->u8Range[0];
}

uint16_t getCurrentRange(uint8_t u8Index){
    return p_VectrData->u8Range[u8Index];
}

void setCurrentRange(uint8_t u8Index, uint8_t u8NewValue){
    p_VectrData->u8Range[u8Index] = u8NewValue;
}

uint16_t getCurrentLinearity(uint8_t u8Index){
    return p_VectrData->u16Linearity[u8Index];
}

void setCurrentLinearity(uint8_t u8Index, uint8_t u8NewValue){
    p_VectrData->u16Linearity[u8Index] = u8NewValue;
}

uint16_t getCurrentSlewRate(uint8_t u8Index){
    return p_VectrData->u16SlewRate[u8Index];
}

void setCurrentSlewRate(uint8_t u8Index, uint16_t u16NewValue){
    p_VectrData->u16SlewRate[u8Index] = u16NewValue;
}

uint8_t getCurrentTrackBehavior(uint8_t u8Index){
    return p_VectrData->u8HoldBehavior[u8Index];
}

void setCurrentTrackBehavior(uint8_t u8Index, uint8_t u8NewValue){
    p_VectrData->u8HoldBehavior[u8Index] = u8NewValue;
}

/*Returns the current source for Record, Playback, or Overdub*/
uint8_t getCurrentSource(uint8_t u8Parameter){
    return p_VectrData->u8Source[u8Parameter];
}

uint8_t setCurrentSource(uint8_t u8Parameter, uint8_t u8NewSetting){
    if(u8NewSetting >= NUM_OF_SOURCE_SETTINGS){
        u8NewSetting = 0;
    }

    p_VectrData->u8Source[u8Parameter] = u8NewSetting;

    if(u8Parameter == RECORD){
        u8TapTempoSetFlag = FALSE;
    }

    return u8NewSetting;
}

uint8_t getPlaybackRunStatus(void){
    return u8PlaybackRunFlag;
}

void setPlaybackRunStatus(uint8_t u8NewState){
    u8PlaybackRunFlag = u8NewState;

    //If playback is set to run, start the clock timer
    if(u8PlaybackRunFlag == RUN){
        START_CLOCK_TIMER;
    }
    else{
        STOP_CLOCK_TIMER;
    }

    u8SkipClockEdge = TRUE;//Keep the clock from getting goofy

    u8OverdubRunFlag = u8PlaybackRunFlag;
}

uint8_t getCurrentControl(uint8_t u8Index){
    return p_VectrData->u8Control[u8Index];
}

uint8_t setCurrentControl(uint8_t u8Index, uint8_t u8NewSetting){

    p_VectrData->u8Control[u8Index] = u8NewSetting;

    return u8NewSetting;
}

uint8_t getCurrentLoopMode(void){
    return p_VectrData->u8PlaybackMode;
}

void setCurrentLoopMode(uint8_t u8NewSetting){
    p_VectrData->u8PlaybackMode = u8NewSetting;
    if(u8NewSetting == LOOPING){
        setPlaybackDirection(FORWARD_PLAYBACK);
    }
}

uint8_t getCurrentRecordClocks(void){
    return p_VectrData->u8NumClocks;
}

void setCurrentRecordClocks(uint8_t u8NewSetting){
    p_VectrData->u8NumClocks = u8NewSetting;
}

uint8_t getCurrentGateMode(void){
    return p_VectrData->u8GateMode;
}

void setCurrentGateMode(uint8_t u8NewState){
    p_VectrData->u8GateMode = u8NewState;
}

uint8_t getCurrentClockMode(void){
    return p_VectrData->u8NumClocks;
}

void setCurrentClockMode(uint8_t u8NewSetting){
    p_VectrData->u8NumClocks = u8NewSetting;
    calculateClockTimer(u32PlaybackSpeed);
}

uint8_t getCurrentModulationMode(void){
    return p_VectrData->u8ModulationMode;
}

void setCurrentModulationMode(uint8_t u8NewSetting){
    p_VectrData->u8ModulationMode = u8NewSetting;
}

uint8_t getSequenceRecordedFlag(void){
    return u8SequenceRecordedFlag;
}

void setSequenceRecordFlag(uint8_t u8NewState){
    u8SequenceRecordedFlag = u8NewState;
}

uint8_t getMuteStatus(uint8_t u8Index){
    return p_VectrData->u8MuteState[u8Index];
}

uint8_t getCurrentSequenceIndex(void){
    return u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex];
}

void setNumberOfClockPulses(void){
    u8NumOfClockPulses = 1 << p_VectrData->u8NumClocks;
}

uint32_t getNextClockPulseIndex(void){
    return u32NextClockPulseIndex;
}

uint8_t getGatePulseFlag(void){
    return u8GatePulseFlag;
}

void setGatePulseFlag(uint8_t u8NewState){
    u8GatePulseFlag = u8NewState;
}

void setResetFlag(void){
    u8ResetFlag = TRUE;
}

void clearResetFlag(void){
    u8ResetFlag = FALSE;
}

uint8_t getResetFlag(void){
    return u8ResetFlag;
}

uint8_t getu8ExternalAirWheelActiveFlag(void){
    return u8ExternalAirWheelActiveFlag;
}

VectrDataStruct * getVectrDataStart(void){
    return p_VectrData;
}