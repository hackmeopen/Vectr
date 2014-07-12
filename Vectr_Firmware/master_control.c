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

//TODO Test hold modes
//TODO Test clock and gate output
//TODO Test Mute Mode
//TODO Test playback algorithms - final check.
//TODO Test Effects
//TODO Test slew rate menu adjustment
//TODO Test the ranges
//TODO Make sure the output can go to 0. Something with the I2C algorithm- maybe slew rate problem.
//TODO Test Ramp Output
//TODO Test Air Scratch - change speed and direction
//TODO Dismiss a hold or live play activation during sequence mode - Test this - maybe more null checking?
//TODO Test start-up behavior
//TODO Make sure nothing bad happens when gestures are made at startup.
//TODO Test memory at 20MHz SPI Bus
//TODO  Test store settings after making a menu change
//TODO Test jack detection with final sample
//TODO Test with 512k RAM with final sample

//TODO Include Bootloader

//TODO Fix the length of files being saved to flash -4096 is not evenly divisible by 12
//TODO After storing a patch, playback doesn't start again.


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

#define GESTURE_DEBOUNCE_TIMER_RESET    25


static VectrDataStruct VectrData;

static uint8_t u8KeyPressFlag = FALSE;
static uint8_t u8EncKeyPressFlag = FALSE;
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

static pos_and_gesture_data pos_and_gesture_struct;
static pos_and_gesture_data * p_mem_pos_and_gesture_struct;
static pos_and_gesture_data * p_overdub_pos_and_gesture_struct;
static pos_and_gesture_data hold_position_struct;


static uint8_t u8BufferDataCount = 0;

static memory_data_packet memBuffer;
static memory_data_packet overdubBuffer;

static uint8_t u8SequenceModeIndexes[MAX_NUM_OF_SEQUENCES];
uint8_t u8SequenceModeSelectedSequenceIndex;

static uint8_t u8NumOfClockPulses;
static uint8_t u32NextClockPulseIndex;

static uint16_t u16LastScratchPosition;
static uint32_t u32ScratchNeutralSpeed;


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
#define SPEED_ADJUSTMENT_INCREMENT   500
#define MINIMUM_SAMPLE_PERIOD   SAMPLE_TIMER_PERIOD>>3 //8x faster
#define MAXIMUM_SAMPLE_PERIOD   SAMPLE_TIMER_PERIOD<<4 //16x slower
static uint32_t u32PlaybackSpeed = STANDARD_PLAYBACK_SPEED;//Number of speed increments away from standard speed
static int16_t i16PlaybackData = 0;
static uint16_t u16PlaybackSpeedTableIndex = INITIAL_SPEED_INDEX;

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
void gateHandler(void);
void holdHandler(pos_and_gesture_data * p_position_data_struct, pos_and_gesture_data * p_memory_data_struct,
pos_and_gesture_data * p_hold_data_struct);
void clockHandler(uint32_t u32CurrentRAMAddress);
uint16_t scaleBinarySearch(const uint16_t *p_scale, uint16_t u16Position, uint8_t u8Length);
void runPlaybackMode(void);
void mutePosition(pos_and_gesture_data * p_pos_and_gesture_struct);
void runModulation(pos_and_gesture_data * p_pos_and_gesture_struct);
void adjustSpeedModulation(uint16_t u16NewValue);
void setNumberOfClockPulses(void);
void calculateNextClockPulse(void);
void calculateRampOutput(pos_and_gesture_data * p_pos_and_gesture_struct);
void runAirScratchMode(pos_and_gesture_data * p_pos_and_gesture_struct);

void MasterControlInit(void){

    VectrData.u8OperatingMode = LIVE_PLAY;
    VectrData.u8PlaybackMode = LOOPING;
    VectrData.u8PlaybackDirection = FORWARD_PLAYBACK;
    VectrData.u8PlaybackMode = LOOPING;
    VectrData.u16SlewRate[X_OUTPUT_INDEX] = MID_SLEW_RATE;
    VectrData.u16SlewRate[Y_OUTPUT_INDEX] = MID_SLEW_RATE;
    VectrData.u16SlewRate[Z_OUTPUT_INDEX] = MID_SLEW_RATE;

    VectrData.u8Range[X_OUTPUT_INDEX] = RANGE_UNI_5V;
    VectrData.u8Range[Y_OUTPUT_INDEX] = RANGE_UNI_5V;
    VectrData.u8Range[Z_OUTPUT_INDEX] = RANGE_UNI_5V;
    VectrData.u8Source[RECORD] = SWITCH;
    VectrData.u8Source[PLAY] = SWITCH;
    VectrData.u8Source[OVERDUB] = SWITCH;
    VectrData.u8Control[RECORD] = TRIGGER;
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

    Flags.u8OverdubActiveFlag = FALSE;

    u16ModulationOnFlag = FALSE;

    u8HoldState = OFF;
    u8MenuModeFlag = FALSE;
    u8EncKeyPressFlag = FALSE;

    p_mem_pos_and_gesture_struct = &memBuffer.sample_1;
}

void MasterControlStateMachine(void){
    
    uint8_t u8MessageReceivedFlag = FALSE;
    uint16_t u16TouchData;
    uint16_t u16AirwheelData;
    
    static uint8_t u8GestureDebounceTimer;
    static uint16_t u16LastAirWheelData;
    int16_t i16AirWheelChange;
    uint8_t u8SampleTimerFlag;//used when the clock is running very slowly to make sure gestures respond
    uint8_t u8OperatingMode = VectrData.u8OperatingMode;
    io_event_message event_message;
    uint8_t u8PlayTrigger = NO_TRIGGER;
    uint8_t u8RecordTrigger = NO_TRIGGER;
    uint8_t u8HoldTrigger = NO_TRIGGER;
    
    u8SyncTrigger = NO_TRIGGER;
    u8HoldActivationFlag = NO_HOLD_EVENT;
    
    //This sample flag is set by the Timer 4/5 32 bit timer. It's timing is controlled by the speed parameter
    u8SampleTimerFlag = xSemaphoreTake(xSemaphoreSampleTimer, 7*TICKS_PER_MS);

    //This queue receives position and gesture data from the I2C algorithm
    u8MessageReceivedFlag = xQueueReceive(xPositionQueue, &pos_and_gesture_struct, 0);

    //Run menu mode if it's active
    if(u8MenuModeFlag == TRUE){
        MenuStateMachine();
    }
    else{
        /*Encoder live interaction can cause hold events and reactivate live play.*/
        encoderLiveInteraction();
    }

    if(u8GestureDebounceTimer > 0){
       u8GestureDebounceTimer--;
    }

    switchStateMachine();//Handle switch presses to change state

    /*Events on the inputs, Record In, Playback In, Hold In, and Sync In get delivered
      on this queue. They should set flags so the appropriate action can be taken in each
     of the different states.*/
    if(xQueueReceive(xIOEventQueue, &event_message, 0)){
       switch(event_message.u8messageType){
           case RECORD_IN_EVENT:
               u8RecordTrigger = event_message.u8message;
               break;
           case PLAYBACK_IN_EVENT:
               u8PlayTrigger = event_message.u8message;
               break;
           case HOLD_IN_EVENT:
               u8HoldTrigger = event_message.u8message;
               if(u8HoldTrigger == TRIGGER_WENT_HIGH){
                   u8HoldActivationFlag = HOLD_ACTIVATE;
               }else{
                   u8HoldActivationFlag = HOLD_DEACTIVATE;
               }
               break;
           case SYNC_IN_EVENT:
               u8SyncTrigger = event_message.u8message;
               break;
           case JACK_DETECT_IN_EVENT:
               if(event_message.u8message == 1){
                   u16ModulationOnFlag = FALSE;
               }
               else{
                   u16ModulationOnFlag = TRUE;
               }
           default:
               break;
       }
    }

    //Handle the gate output and the clock output.
    gateHandler();

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
                            //Go to overdub mode if a sequence has been recorded.
                            if(u8SequenceRecordedFlag == TRUE){
                                enterOverdubMode();
                            }
                            else{
                                //indicate error.
                                LEDIndicateError();
                            }
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
                                u32ScratchNeutralSpeed = u32PlaybackSpeed;
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
            }
            else{
                pos_and_gesture_struct.u16XPosition = VectrData.u16CurrentXPosition;
                pos_and_gesture_struct.u16YPosition = VectrData.u16CurrentYPosition;
                pos_and_gesture_struct.u16ZPosition = VectrData.u16CurrentZPosition;
            }

            slewPosition(&pos_and_gesture_struct);

            mutePosition(&pos_and_gesture_struct);

            linearizePosition(&pos_and_gesture_struct);


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
                /*Back to Live Play*/
                u8HoldState = OFF;
                u8LivePlayActivationFlag = FALSE;
            }


            /*If hold is active, execute the hold behavior*/
            if(u8HoldState == ON){
                holdHandler(&pos_and_gesture_struct, NULL, &hold_position_struct);
            }

            /*If menu mode is active, the menu controls the LEDs.*/
            if(u8MenuModeFlag == FALSE){
                xQueueSend(xLEDQueue, &pos_and_gesture_struct, 0);
            }

            /*Implement scaling.*/
            scaleRange(&pos_and_gesture_struct);

            /*Quantize the position.*/
            quantizePosition(&pos_and_gesture_struct);

            /*Send the data out to the DAC.*/
            xQueueSend(xSPIDACQueue, &pos_and_gesture_struct, 0);

            /*Check for armed playback or recording and the appropriate trigger.*/
            if(u8PlaybackArmedFlag == ARMED){
                if(u8PlayTrigger == TRIGGER_WENT_HIGH){
                    /*If we received the playback trigger.*/
                    u8PlayTrigger = NO_TRIGGER;
                    if(u8SequenceRecordedFlag == TRUE){
                        u8PlaybackArmedFlag = NOT_ARMED;
                        VectrData.u8OperatingMode = PLAYBACK;
                        u8PlaybackRunFlag = RUN;
                    }
                }
            }
            if(u8RecordingArmedFlag == ARMED){
                if(u8RecordTrigger == TRIGGER_WENT_HIGH){/*If we received the record trigger.*/
                    u8RecordTrigger = NO_TRIGGER;
                    u8RecordingArmedFlag = NOT_ARMED;
                    startNewRecording();
                }
            }
            break;
        case RECORDING:

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
                VectrData.u8OperatingMode = LIVE_PLAY;
                u8LivePlayActivationFlag = FALSE;
            }

            /*If hold is active, execute the hold behavior*/
            if(u8HoldState == ON){
                holdHandler(&pos_and_gesture_struct, NULL, &hold_position_struct);
            }

            slewPosition(&pos_and_gesture_struct);
            xQueueSend(xLEDQueue, &pos_and_gesture_struct, 0); 
            scaleRange(&pos_and_gesture_struct);
            /*Quantize the position.*/
            quantizePosition(&pos_and_gesture_struct);
            xQueueSend(xSPIDACQueue, &pos_and_gesture_struct, 0);


            /*Check for triggers...
            * If the mode is trigger, look for a rising edge trigger.
             * If the mode is gate, look for a falling edge trigger.
             */

            /*If we're in gate mode, then a low level stops recording. A high level starts recording.
             */
            if(VectrData.u8Control[RECORD] == GATE && VectrData.u8Source[RECORD] == EXTERNAL){
                if(u8RecordRunFlag == TRUE && u8RecordTrigger == TRIGGER_WENT_LOW){
                    u8RecordRunFlag = FALSE;
                    finishRecording();
                }
                else if(u8RecordRunFlag == FALSE && u8RecordTrigger == TRIGGER_WENT_HIGH){
                    u8RecordRunFlag = TRUE;
                }
            }

            /*Check for armed playback or recording and the appropriate trigger.
             *These can only be armed or disarmed if the Source for each is set to external
             */
            if(u8PlaybackArmedFlag == ARMED){
                /*Look for a rising edge to start playback*/
                if(u8PlayTrigger == TRIGGER_WENT_HIGH){
                    finishRecording();
                    /*Clear the trigger and go to playback.*/
                    u8PlayTrigger = NO_TRIGGER;
                    u8PlaybackArmedFlag = NOT_ARMED;
                    VectrData.u8OperatingMode = PLAYBACK;
                    u8PlaybackRunFlag = RUN;
                }
            }
            if(u8RecordingArmedFlag == ARMED){
                if(u8RecordTrigger == TRIGGER_WENT_HIGH){/*If we received the record trigger.*/
                    u8RecordingArmedFlag = NOT_ARMED;
                    u8RecordTrigger = NO_TRIGGER;
                    startNewRecording();
                }
            }
            else if(u8RecordingArmedFlag == DISARMED){
                /*Stop recording
                 * In trigger mode, a low->high will stop recording.
                 * In gate mode, a high->low will stop recording.
                 */
                if(VectrData.u8Control[RECORD] == TRIGGER){
                    if(u8RecordTrigger == TRIGGER_WENT_HIGH){
                        u8RecordingArmedFlag = NOT_ARMED;
                        finishRecording();
                    }
                }else{
                    /*Gate control*/
                    if(u8RecordTrigger == TRIGGER_WENT_LOW){
                        u8RecordingArmedFlag = NOT_ARMED;
                        finishRecording();
                    }
                }
            }
            
            /*Recording - Keep track of airwheel but don't do anything with it to
             keep jumps from occurring*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;

        case PLAYBACK:
            /*If playback is running, read the RAM.*/
            if(u8SampleTimerFlag == TRUE){
                runPlaybackMode();
            }

            /*During playback, we are only looking for gestures to enter performance
             or menu mode.*/
            if(u8MessageReceivedFlag == TRUE){
            /*Playback - Decode touch and gestures. If a double tap on the bottom occurs,
                 * enter menu mode*/
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData != MGC3130_NO_TOUCH){
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            if(u8MenuModeFlag == FALSE){
                                u8MenuModeFlag = TRUE;
                                menuHandlerInit();
                            }
                            break;
                        case OVERDUB_MODE:
                            VectrData.u8OperatingMode =  OVERDUBBING;
                            setLEDAlternateFuncFlag(TRUE);
                            turnOffAllLEDs();
                            setIndicateOverdubModeFlag(TRUE);
                            Flags.u8OverdubActiveFlag = FALSE;
                            u8OverdubRunFlag = u8PlaybackRunFlag;
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
                /*Playback - Airwheel adjust speed.*/
                u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
                if(u16AirwheelData != u16LastAirWheelData){
                    if(u16AirwheelData != 0 && u8PlaybackRunFlag == RUN && u8MenuModeFlag == FALSE){
                        i16AirWheelChange = u16AirwheelData - u16LastAirWheelData;

                        //If the data decremented past zero, the new data minus the old will be large.
                        if(i16AirWheelChange > 127){
                            i16AirWheelChange = u16LastAirWheelData - (256 - u16AirwheelData);
                        }

                        //If the data incremented past zero, the data will be large and negative
                        if(i16AirWheelChange < -127){
                            i16AirWheelChange = u16AirwheelData + (256 - u16LastAirWheelData);
                        }
                   
                        adjustSpeedTable(i16AirWheelChange);
                    }
                    u16LastAirWheelData = u16AirwheelData;
                }

            }
            
            /*If we're in gate mode, then a low level stops recording. A high level starts recording.
             */
            if(VectrData.u8Control[PLAY] == GATE && VectrData.u8Source[PLAY] == EXTERNAL){
                if(u8PlaybackRunFlag == RUN && u8PlayTrigger == TRIGGER_WENT_LOW){
                    u8PlaybackRunFlag = STOP;
                }
                else if(u8PlaybackRunFlag == STOP && u8PlayTrigger == TRIGGER_WENT_HIGH){
                    u8PlaybackRunFlag = RUN;
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
                if(VectrData.u8PlaybackMode != FLIP &&
                   VectrData.u8PlaybackMode != RETRIGGER){
                    if(((VectrData.u8Control[PLAY] == TRIGGER) || (VectrData.u8Control[PLAY] == TRIGGER_AUTO))
                            && (u8PlayTrigger == TRIGGER_WENT_HIGH)){
                        u8PlaybackArmedFlag = NOT_ARMED;
                        /*Clear the trigger and start playback.*/
                        u8PlayTrigger = NO_TRIGGER;
                        u8PlaybackRunFlag = RUN;
                    }
                }else if(VectrData.u8PlaybackMode == FLIP){
                    if(VectrData.u8PlaybackDirection == FORWARD_PLAYBACK){
                        setPlaybackDirection(REVERSE_PLAYBACK);
                    }
                    else{
                        setPlaybackDirection(FORWARD_PLAYBACK);
                    }
                }
                else{
                    //Retrigger mode
                    setRAMRetriggerFlag();
                    u8PlaybackRunFlag = TRUE;
                }
            }
            else if(u8PlaybackArmedFlag == DISARMED){
                if(((VectrData.u8Control[PLAY] == TRIGGER) || (VectrData.u8Control[PLAY] == TRIGGER_AUTO))
                        && (u8PlayTrigger == TRIGGER_WENT_LOW)){
                    u8PlaybackArmedFlag = NOT_ARMED;
                    /*Clear the trigger and start playback.*/
                    u8PlayTrigger = NO_TRIGGER;
                    u8PlaybackRunFlag = STOP;
                }
            }
            if(u8RecordingArmedFlag == ARMED){
                if(u8RecordTrigger == TRIGGER_WENT_HIGH){/*If we received the record trigger.*/
                    u8RecordingArmedFlag = NOT_ARMED;
                    u8RecordTrigger = NO_TRIGGER;
                    startNewRecording();
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

            /*If ovedub isn't active, indicate the active axes by flashing the axes that
             are active*/
            //Handle the outputs
            if(u8OverdubRunFlag == TRUE && u8SampleTimerFlag == TRUE){
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

                //Overdub the retrieved sample
                if(Flags.u8OverdubActiveFlag == TRUE){
                    //Overdub the axes with overdub active and for the others write back the accessed data
                    if(VectrData.u8OverdubStatus[X_OUTPUT_INDEX] == 1){
                        p_mem_pos_and_gesture_struct->u16XPosition = pos_and_gesture_struct.u16XPosition;
                        p_overdub_pos_and_gesture_struct->u16XPosition = pos_and_gesture_struct.u16XPosition;
                    }
                    else{
                       p_overdub_pos_and_gesture_struct->u16XPosition =  p_mem_pos_and_gesture_struct->u16XPosition;
                    }

                    if(VectrData.u8OverdubStatus[Y_OUTPUT_INDEX] == 1){
                        p_mem_pos_and_gesture_struct->u16YPosition = pos_and_gesture_struct.u16YPosition;
                        p_overdub_pos_and_gesture_struct->u16YPosition = pos_and_gesture_struct.u16YPosition;
                    }
                    else{
                        p_overdub_pos_and_gesture_struct->u16YPosition =  p_mem_pos_and_gesture_struct->u16YPosition;
                    }

                    if(VectrData.u8OverdubStatus[Z_OUTPUT_INDEX] == 1){
                        p_mem_pos_and_gesture_struct->u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                        p_overdub_pos_and_gesture_struct->u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                    }
                    else{
                        p_overdub_pos_and_gesture_struct->u16ZPosition =  p_mem_pos_and_gesture_struct->u16ZPosition;
                    }
                    

                    if(u8BufferDataCount == 0){
                        u8MemCommand = WRITETHENREAD_RAM;
                        xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);//Set up for OVERDUB
                        xQueueSend(xRAMWriteQueue, &overdubBuffer, 0);
                    }
                    
                    slewPosition(p_mem_pos_and_gesture_struct);
                    xQueueSend(xLEDQueue, p_mem_pos_and_gesture_struct, 0);
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

                    slewPosition(p_mem_pos_and_gesture_struct);
                    scaleRange(p_mem_pos_and_gesture_struct);
                    /*Quantize the position.*/
                    quantizePosition(p_mem_pos_and_gesture_struct);
                    xQueueSend(xSPIDACQueue, p_mem_pos_and_gesture_struct, 0);
                }
            }

            /* When overdub is entered, the axes can be turned on or off. Touches
             * Touches on the left turn on/off x
             * Touches on the top turn on/off y
             * Touches on the right turn on/off z
             */
            if(u8MessageReceivedFlag == TRUE){
                u16TouchData = pos_and_gesture_struct.u16Touch;
                if(u16TouchData >= MGC3130_DOUBLE_TAP_BOTTOM){//double taps
                    u8GestureFlag = decodeDoubleTapGesture(u16TouchData);
                    switch(u8GestureFlag){
                        case MENU_MODE:
                            setLEDAlternateFuncFlag(FALSE);
                            turnOffAllLEDs();
                            Flags.u8OverdubActiveFlag = FALSE;
                            setLEDAlternateFuncFlag(FALSE);
                            setIndicateOverdubModeFlag(FALSE);
                            VectrData.u8OperatingMode =  PLAYBACK;
                            break;
                        case OVERDUB_MODE:

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
                                VectrData.u8OverdubStatus[X_OUTPUT_INDEX] ^= 1;
                                break;
                            case Y_OUTPUT_INDEX:
                                VectrData.u8OverdubStatus[Y_OUTPUT_INDEX] ^= 1;
                                break;
                            case Z_OUTPUT_INDEX:
                                VectrData.u8OverdubStatus[Z_OUTPUT_INDEX] ^= 1;
                                break;
                            default:
                                break;
                        }
                        indicateActiveAxes(OVERDUB);
                    }
                }
            }

            if(VectrData.u8Control[OVERDUB] == GATE && VectrData.u8Source[OVERDUB] == EXTERNAL){
                if(u8OverdubRunFlag == TRUE && u8RecordTrigger == TRIGGER_WENT_LOW){
                    Flags.u8OverdubActiveFlag = FALSE;
                }
                else if(u8OverdubRunFlag == FALSE && u8RecordTrigger == TRIGGER_WENT_HIGH){
                    Flags.u8OverdubActiveFlag = TRUE;
                }
            }

             /*Check for armed playback or recording and the appropriate trigger.
             *These can only be armed or disarmed if the Source for each is set to external
             */
            if(u8OverdubArmedFlag == ARMED){
                /*Look for a rising edge to start playback*/
                if(VectrData.u8Control[OVERDUB] == TRIGGER && u8RecordTrigger == TRIGGER_WENT_HIGH){
                    u8OverdubArmedFlag = NOT_ARMED;
                    /*Clear the trigger and start playback.*/
                    u8RecordTrigger = NO_TRIGGER;
                    Flags.u8OverdubActiveFlag = TRUE;
                }
            }
            else if(u8OverdubArmedFlag == DISARMED){
                if(VectrData.u8Control[OVERDUB] == TRIGGER && u8RecordTrigger == TRIGGER_WENT_LOW){
                    u8OverdubArmedFlag = NOT_ARMED;
                    /*Clear the trigger and start playback.*/
                    u8RecordTrigger = NO_TRIGGER;
                    Flags.u8OverdubActiveFlag = FALSE;
                }
            }  
            /*Overdub - Keep track of airwheel data but don't do anything about it*/
            u16AirwheelData = pos_and_gesture_struct.u16Airwheel;
            u16LastAirWheelData = u16AirwheelData;
            break;
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
                runPlaybackMode();
            }

            //Activate the selected sequence.
            if(u8SequenceModeChangeSequenceFlag == TRUE){
                u8SequenceModeChangeSequenceFlag = FALSE;

                //Is the sequence in RAM or Flash?
                if(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex] == RAM_SEQUENCE_MASK){
                    setStoredSequenceLocationFlag(STORED_IN_RAM);
                    resetRAMReadAddress();
                    resetRAMEndofReadAddress();
                }
                else{
                    setStoredSequenceLocationFlag(STORED_IN_FLASH);
                    setFlashReadNewSequence(u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex]);
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
                            VectrData.u8OperatingMode =  PLAYBACK;//Fix this. Back to Live Play?
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
            break;
        case MUTING:
            if(u8SampleTimerFlag == TRUE && u8SequenceRecordedFlag == TRUE){
                runPlaybackMode();
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
                            VectrData.u8OperatingMode =  PLAYBACK;//Fix this. Back to Live Play?
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
                                VectrData.u8MuteState[X_OUTPUT_INDEX] ^= TRUE;
                                break;
                            case Y_OUTPUT_INDEX://Top
                                VectrData.u8MuteState[Y_OUTPUT_INDEX] ^= TRUE;
                                break;
                            case Z_OUTPUT_INDEX://Right
                                VectrData.u8MuteState[Z_OUTPUT_INDEX] ^= TRUE;
                                break;
                            default:
                                break;
                        }
                }
            }
            break;
        case AIR_SCRATCHING:




            runAirScratchMode(&pos_and_gesture_struct);

            runPlaybackMode();
            break;
        default:
            break;
    }
}

void runPlaybackMode(void){
    uint32_t u32LengthOfSequence;
    

   if(u8PlaybackRunFlag == RUN && u8HoldState == OFF){

        if(u8BufferDataCount == 0){
            xQueueReceive(xRAMReadQueue, &memBuffer, 0);
            p_mem_pos_and_gesture_struct = &memBuffer.sample_1;
            u8BufferDataCount++;
        }
        else{
            p_mem_pos_and_gesture_struct = &memBuffer.sample_2;
            u8MemCommand = READ_RAM;
            xQueueSend(xMemInstructionQueue, &u8MemCommand, 0);//Set up for write
            u8BufferDataCount = 0;
        }
   }

   if(p_mem_pos_and_gesture_struct != NULL){
       slewPosition(p_mem_pos_and_gesture_struct);

       if(u16ModulationOnFlag == TRUE){
           runModulation(p_mem_pos_and_gesture_struct);
       }

       mutePosition(p_mem_pos_and_gesture_struct);

       /*Check encoder activations. Could go to live play or implement a hold*/
        if(u8HoldActivationFlag == HOLD_ACTIVATE){
            u8HoldState = ON;
            setHoldPosition(p_mem_pos_and_gesture_struct);
            u8HoldActivationFlag = FALSE;
        }
        else if(u8HoldActivationFlag == HOLD_DEACTIVATE){
            u8HoldState = OFF;
        }
        else if(u8LivePlayActivationFlag == TRUE){
            setSwitchLEDState(OFF);
            if(VectrData.u8OperatingMode != SEQUENCING){
                VectrData.u8OperatingMode = LIVE_PLAY;
            }
            u8LivePlayActivationFlag = FALSE;
        }

        if(u8SyncTrigger == TRUE){
            setRAMRetriggerFlag();
            u8SyncTrigger = FALSE;
        }

        /*If hold is active, execute the hold behavior*/
        if(u8HoldState == ON){
            holdHandler(&pos_and_gesture_struct, p_mem_pos_and_gesture_struct, &hold_position_struct);
        }

       //If the clock output is being driven, set the next index to trigger the clock pulse.
       if(u8ClockPulseFlag == TRUE){
          /*Calculate the number of memory addresses until the next clock pulse. It's the length of the sequence
           shifted down by the number of clocks.*/
           calculateNextClockPulse();
           u8ClockPulseFlag = FALSE;
       }



       xQueueSend(xLEDQueue, p_mem_pos_and_gesture_struct, 0);
       scaleRange(p_mem_pos_and_gesture_struct);
       /*Quantize the position.*/
       quantizePosition(p_mem_pos_and_gesture_struct);
       calculateRampOutput(p_mem_pos_and_gesture_struct);//Make a ramp on the outputs if necessary
       xQueueSend(xSPIDACQueue, p_mem_pos_and_gesture_struct, 0);
   }
   else{
       p_mem_pos_and_gesture_struct = &memBuffer.sample_1;
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
    static uint16_t u16CurrentScratchSpeedIndex;
    uint32_t i32Movement;
    uint16_t u16NewScratchData = p_pos_and_gesture_struct->u16YPosition;

    if(u8HandPresentFlag == TRUE){
        u8AirScratchRunFlag = TRUE;
        i32Movement =  u16NewScratchData - u16LastScratchPosition;
        if(i32Movement > 0){
            //Upward hand movement.
            u8PlaybackRunFlag = TRUE;
            //Play forward based on the movement.
            i32Movement >>= 6;//Scale down to the size of the table.
            i32Movement = u16PlaybackSpeedTableIndex - i32Movement;
            u32PlaybackSpeed = u32LogSpeedTable[i32Movement];
            u16CurrentScratchSpeedIndex = i32Movement;
            SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
        else if(i32Movement < 0){
            //Downward hand movement.
            u8PlaybackRunFlag = TRUE;
            i32Movement *= -1;
            i32Movement >>= 6;//Scale down to the size of the table.
            i32Movement = u16PlaybackSpeedTableIndex + i32Movement;
            u32PlaybackSpeed = u32LogSpeedTable[i32Movement];
            u16CurrentScratchSpeedIndex = i32Movement;
            SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
        else{
            u8PlaybackRunFlag = FALSE;
        }
    }
    else{
        u8PlaybackRunFlag = TRUE;
        if(u8AirScratchRunFlag == TRUE){
            /*Slow down or speed up until we reach the neutral speed.*/
            if(u16CurrentScratchSpeedIndex > u16PlaybackSpeedTableIndex){
                u16CurrentScratchSpeedIndex -= SCRATCH_SLOWDOWN_CONSTANT;
                if(u16CurrentScratchSpeedIndex < u16PlaybackSpeedTableIndex){
                    u16CurrentScratchSpeedIndex = u16PlaybackSpeedTableIndex;
                    u8AirScratchRunFlag = FALSE;
                }
            }
            else if(u16CurrentScratchSpeedIndex < u16PlaybackSpeedTableIndex){
                u16CurrentScratchSpeedIndex += SCRATCH_SLOWDOWN_CONSTANT;

                if(u16CurrentScratchSpeedIndex > u16PlaybackSpeedTableIndex){
                    u16CurrentScratchSpeedIndex = u16PlaybackSpeedTableIndex;
                    u8AirScratchRunFlag = FALSE;
                }
            }
            u32PlaybackSpeed = u32LogSpeedTable[u16CurrentScratchSpeedIndex];
            SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
            RESET_SAMPLE_TIMER;
        }
    }

    u16LastScratchPosition = u16NewScratchData;

}

void setAirScratchNeutralSpeed(uint32_t u32NewNeutralSpeed){
    u32ScratchNeutralSpeed = u32NewNeutralSpeed;
}

void calculateRampOutput(pos_and_gesture_data * p_pos_and_gesture_struct){
    uint32_t u32SequenceLength;
    uint32_t u32SequencePosition;
    uint32_t u32OutputValue;

    if(VectrData.u8Range[X_OUTPUT_INDEX] == RAMP ||
       VectrData.u8Range[Y_OUTPUT_INDEX] == RAMP ||
       VectrData.u8Range[Z_OUTPUT_INDEX] == RAMP){
        u32SequenceLength = getActiveSequenceLength();
        u32SequencePosition = getSequencePlaybackPosition();
        u32OutputValue = MAXIMUM_OUTPUT_VALUE*u32SequencePosition;
        u32OutputValue /= u32SequenceLength;
        
        if(VectrData.u8Range[X_OUTPUT_INDEX] == RAMP){
            p_pos_and_gesture_struct->u16XPosition = u32OutputValue;
        }
        if(VectrData.u8Range[Y_OUTPUT_INDEX] == RAMP){
            p_pos_and_gesture_struct->u16YPosition = u32OutputValue;
        }
        if(VectrData.u8Range[Z_OUTPUT_INDEX] == RAMP){
            p_pos_and_gesture_struct->u16ZPosition = u32OutputValue;
        }
    }
}

void calculateNextClockPulse(void){
    uint32_t u32LengthOfSequence = getActiveSequenceLength();
    
    u32NextClockPulseIndex = u32LengthOfSequence>>(1<<VectrData.u8ClockMode);
}

void enterOverdubMode(void){
    VectrData.u8OperatingMode =  OVERDUBBING;
    setLEDAlternateFuncFlag(TRUE);
    turnOffAllLEDs();
    setIndicateOverdubModeFlag(TRUE);
    Flags.u8OverdubActiveFlag = FALSE;
    u8OverdubRunFlag = u8PlaybackRunFlag;
}

void enterAirScratchMode(void){
    VectrData.u8OperatingMode = AIR_SCRATCHING;
    Flags.u8AirScratchFlag = FALSE;
}

void enterMuteMode(void){
    VectrData.u8OperatingMode = MUTING;
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
        VectrData.u8OperatingMode = SEQUENCING;
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
    if(VectrData.u8MuteState[X_OUTPUT_INDEX] == TRUE){
        if(VectrData.u8Range[X_OUTPUT_INDEX] <= RANGE_BI_2_5V){
            p_pos_and_gesture_struct->u16XPosition = ZERO_VOLT_OUTPUT_VALUE;
        }
        else{
            p_pos_and_gesture_struct->u16XPosition = 0;
        }
    }

    if(VectrData.u8MuteState[Y_OUTPUT_INDEX] == TRUE){
        if(VectrData.u8Range[Y_OUTPUT_INDEX] <= RANGE_BI_2_5V){
            p_pos_and_gesture_struct->u16YPosition = ZERO_VOLT_OUTPUT_VALUE;
        }
        else{
            p_pos_and_gesture_struct->u16YPosition = 0;
        }
    }

    if(VectrData.u8MuteState[Z_OUTPUT_INDEX] == TRUE){
        if(VectrData.u8Range[Z_OUTPUT_INDEX] <= RANGE_BI_2_5V){
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
void gateHandler(void){
    /*  HAND_GATE - High when a hand is present, low when it is not.
        RESET_GATE - Pulse at beginning of playback
        PLAY_GATE - High during playback, low when not.
        RECORD_GATE - High during recording, low when not.
        QUANT_GATE - Pulse for every quantization step.
     */
    uint8_t u8GateMode = VectrData.u8GateMode;

    switch(u8GateMode){
        case HAND_GATE:
            /*If a hand is present, high. Otherwise, low.*/
            if(u8HandPresentFlag == TRUE){
                SET_GATE_OUT_PORT;
            }else{
                CLEAR_GATE_OUT_PORT;
            }
            break;
        case RESET_GATE:
            /*Handled in the clock routine.*/
            //Get the current read address for clocking.

            if(u8ResetFlag == TRUE && u8GatePulseFlag == FALSE
              && u8PlaybackRunFlag == RUN){
                u8GatePulseFlag = TRUE;
                u8ResetFlag = FALSE;
            }
            else{
                CLEAR_GATE_OUT_PORT;
            }
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
                RESET_PULSE_TIMER;
            }
            else if(u8PulseExpiredFlag == TRUE && u8GatePulseFlag == TRUE){
                //Pulse expired. End it.
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
    uint8_t u8QuantizationMode;
    uint16_t u16temp;
    uint8_t u8index;
    uint16_t u16CurrentPosition[NUM_OF_AXES] = {p_pos_and_gesture_struct->u16XPosition,
                                        p_pos_and_gesture_struct->u16YPosition,
                                        p_pos_and_gesture_struct->u16ZPosition};
    
    for(i=0;i<NUMBER_OF_OUTPUTS;i++){
        u8QuantizationMode = VectrData.u16Quantization[i];
        u16temp = u16CurrentPosition[i];
        
        switch(u8QuantizationMode){
            case NO_QUANTIZATION://do nothing.
                break;
            case CHROMATIC:
                u16CurrentPosition[i] = scaleBinarySearch(u16ChromaticScale, u16temp, LENGTH_OF_CHROMATIC_SCALE);
                break;
            case MAJOR:
                u16CurrentPosition[i] = scaleBinarySearch(u16MajorScale, u16temp, LENGTH_OF_MAJOR_SCALE);
                break;
            case PENTATONIC:
                u16CurrentPosition[i] = scaleBinarySearch(u16PentatonicScale, u16temp, LENGTH_OF_PENTATONIC_SCALE);
                break;
            case OCTAVE:
                u16CurrentPosition[i] = scaleBinarySearch(u16OctaveScale, u16temp, LENGTH_OF_OCTAVE_SCALE);
                break;
            default:
                break;
    }
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
    int i;

    /*Get the ADC data and make some change based on it.*/
    if(xQueueReceive(xADCQueue, &u16ADCData, 0)){
        switch(VectrData.u8ModulationMode){
            case SPEED:
                adjustSpeedModulation(u16ADCData>>2);
                break;
            case QUANTIZED_SPEED:
                //There are 16 speeds. Reduce down to 4 bits.
                u16Temp = u16ADCData>>8;
                u32PlaybackSpeed = u32QuantizedSpeedTable[u16Temp];
                SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
                RESET_SAMPLE_TIMER;
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

                if(u16LastADCData < u16ADCData){
                    setPlaybackDirection(REVERSE_PLAYBACK);
                }
                else{
                    setPlaybackDirection(FORWARD_PLAYBACK);
                }

                u32SequenceLength = getActiveSequenceLength();

                /*The new sequence position is the determined by the ADC reading.*/
                u32NewReadAddress = u32SequenceLength*u16ADCData;
                u32NewReadAddress >>= 12;//ADC has 4096 levels, 12 bits.

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
                    u32PositionTemp = u16CurrentPosition[i];
                    u32PositionTemp *= u16Attenuation;
                    u32PositionTemp >>= 12;
                    if(u8InversionFlag == TRUE){
                        u32PositionTemp = MAXIMUM_OUTPUT_VALUE - u32PositionTemp;
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
    return VectrData.u8PlaybackDirection;
}

void setPlaybackDirection(uint8_t u8NewDirection){
    uint8_t u8OldDirection = VectrData.u8PlaybackDirection;

    VectrData.u8PlaybackDirection = u8NewDirection;
    /*If playback is coming out of flash, then the end of the sector needs
     to be adjusted for the new playback direction.*/
    if(getStoredSequenceLocationFlag() == STORED_IN_FLASH && (u8OldDirection != u8NewDirection)){
        changeFlashPlaybackDirection(u8NewDirection);
    }
}

uint8_t getPlaybackMode(void){
    return VectrData.u8PlaybackMode;
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
    switch(VectrData.u8PlaybackMode){
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
            setPlaybackDirection(VectrData.u8PlaybackDirection ^= REVERSE_PLAYBACK);
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
    uint16_t * p_u16Position = &(p_position_data_struct->u16XPosition);//Point to the start of the struct  X Position
    uint16_t * p_u16HoldPosition = &(p_hold_data_struct->u16XPosition);//Point to the start of the struct
    uint16_t * p_u16MemoryPosition = &(p_memory_data_struct->u16XPosition);

    /*There are behavioral differences for the hold mode depending on which master state
     we are in and whether playback is running or not.
     In live play mode, a received hold command will hold the outputs.*/
    switch(VectrData.u8OperatingMode){
        case LIVE_PLAY:
        case RECORDING:
            /*Go through each of the axes and perform the hold behavior for that axis */
            for(i=0; i<NUM_OF_AXES; i++){
                u8HoldMode = VectrData.u8HoldBehavior[i];

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
                u8HoldMode = VectrData.u8HoldBehavior[i];
                if(u8PlaybackRunFlag == RUN){
                    /*If playback is running, then the behavior is the same for all except
                     the track/live mode where the output becomes live.*/
                    if(u8HoldMode != TRACKLIVE){
                        *(p_u16MemoryPosition + i) = *(p_u16HoldPosition + i);
                    }
                    else{
                        *(p_u16MemoryPosition + i) = *(p_u16Position + i);
                    }
                }else{
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
            }
            break;
        case OVERDUBBING:
            if(u8OverdubRunFlag == TRUE){
                if(u8HoldMode != TRACKLIVE){
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

    resetSpeed();
    VectrData.u8OperatingMode = RECORDING;
    u8RecordRunFlag = TRUE;
    resetRAMWriteAddress();
    memBuffer.sample_1.u16XPosition = pos_and_gesture_struct.u16XPosition;
    memBuffer.sample_1.u16YPosition = pos_and_gesture_struct.u16YPosition;
    memBuffer.sample_1.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
    u8BufferDataCount = 1;
    setSwitchLEDState(SWITCH_LED_RED_BLINKING);
}

/*End a recording. If automatic play triggering is enabled, go to playback.*/
void finishRecording(void){
    u8RecordRunFlag = FALSE;
    u8SequenceRecordedFlag = TRUE;
    setStoredSequenceLocationFlag(STORED_IN_RAM);
                    
    if(VectrData.u8Control[PLAY] == TRIGGER_AUTO){
        //Initiate a read.
        resetRAMReadAddress();
        u8BufferDataCount = 0;
        VectrData.u8OperatingMode = PLAYBACK;
        u8PlaybackRunFlag = RUN;
        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
    }
    else{//Regular trigger mode ends recording without starting playback and stays in record mode.
        u8PlaybackRunFlag = ENDED;
        setSwitchLEDState(SWITCH_LED_OFF);
    }
}

void setHoldPosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    hold_position_struct.u16XPosition = p_pos_and_gesture_struct->u16XPosition;
    hold_position_struct.u16YPosition = p_pos_and_gesture_struct->u16YPosition;
    hold_position_struct.u16ZPosition = p_pos_and_gesture_struct->u16ZPosition;
}

void armRecording(void){
    u8RecordingArmedFlag = ARMED;
}

void armPlayback(void){
    u8PlaybackArmedFlag = ARMED;
}

void disarmRecording(void){
    u8RecordingArmedFlag = DISARMED;
}

void disarmPlayback(void){
    u8PlaybackArmedFlag = DISARMED;
}

void armOverdub(void){
    u8OverdubArmedFlag = ARMED;
}

void disarmOverdub(void){
    u8OverdubArmedFlag = DISARMED;
}

void switchStateMachine(void){
    uint8_t u8OperatingMode = VectrData.u8OperatingMode;
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
                        if(VectrData.u8Source[PLAY] == SWITCH){
                            VectrData.u8OperatingMode = PLAYBACK;
                            u8PlaybackRunFlag = RUN;
                           // resetRAMReadAddress();//necessary? probably not desirable.
                        }
                        else{//external control = arm playback
                            armPlayback();
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
                   if(VectrData.u8Source[RECORD] == SWITCH){
                       resetSpeed();
                       VectrData.u8OperatingMode = RECORDING;
                       u8RecordRunFlag = TRUE;
                       resetRAMWriteAddress();
                       memBuffer.sample_1.u16XPosition = pos_and_gesture_struct.u16XPosition;
                       memBuffer.sample_1.u16YPosition = pos_and_gesture_struct.u16YPosition;
                       memBuffer.sample_1.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                       u8BufferDataCount = 1;
                       setSwitchLEDState(SWITCH_LED_RED_BLINKING);
                   }else{
                       //Go to record mode and wait for the trigger?
                       armRecording();
                   }
               }
               else{
                   //Main switch in menu mode should back out the menu..full exit is menu double tap
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
                if(VectrData.u8Source[PLAY] == SWITCH || VectrData.u8Control[PLAY] == TRIGGER_AUTO){
                    VectrData.u8OperatingMode = PLAYBACK;
                    u8PlaybackRunFlag = RUN;
                    resetRAMReadAddress();//Recording ends go to the beginning of the sequence
                }
                else{//external control = arm playback
                    armPlayback();
                }
                break;
            case MAIN_SWITCH_PRESSED://RECORDING
                /* REC/SWITCH/TRIGGER - Stop recording on main switch press
                 * REC/SWITCH/GATE - Start recording on main switch press
                 * REC/EXT/TRIGGER - Arm/disarm recording
                 * REC/EXT/GATE - Arm/disarm recording
                 */
                if(VectrData.u8Source[RECORD] == SWITCH){
                    finishRecording();
                }
                else{
                    /*If recording is running, we disarm.
                     If recording is not running, we arm it.*/
                    if(u8RecordRunFlag == TRUE){
                        disarmRecording();
                    }
                    else{
                        armRecording();
                    }
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
                if(VectrData.u8Control[RECORD] == GATE && VectrData.u8Source[RECORD] == SWITCH){
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
                    if(VectrData.u8Source[PLAY] == SWITCH){
                        if(VectrData.u8PlaybackMode != FLIP &&
                           VectrData.u8PlaybackMode != RETRIGGER){
                            if(u8PlaybackRunFlag != RUN){
                                u8PlaybackRunFlag = RUN;
                            }
                            else{
                                u8PlaybackRunFlag = STOP;
                            }
                        }
                        else if(VectrData.u8PlaybackMode == FLIP){//Flip playback mode
                            if(VectrData.u8PlaybackDirection == FORWARD_PLAYBACK){
                                setPlaybackDirection(REVERSE_PLAYBACK);
                            }
                            else{
                                setPlaybackDirection(FORWARD_PLAYBACK);
                            }
                        }
                        else{

                            setRAMRetriggerFlag();
                            //Retrigger playback mode.
                            u8PlaybackRunFlag = RUN;
                        }

                        if(u8PlaybackRunFlag == RUN){
                            setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                        }
                        else{
                            setSwitchLEDState(OFF);
                        }
                    }else{//external control = arm playback
                        /*If recording is running, we disarm.
                         If recording is not running, we arm it.
                         If the playback mode is flip, then we only arm playback
                         to cause the direction to flip with the next trigger.*/
                        if(VectrData.u8PlaybackMode != FLIP){
                            if(u8PlaybackRunFlag == RUN){
                                disarmPlayback();
                            }
                            else{
                                armPlayback();
                            }
                        }else{
                            armPlayback();
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
                    if(VectrData.u8Source[RECORD] == SWITCH){
                       VectrData.u8OperatingMode = RECORDING;
                       u8RecordRunFlag = TRUE;
                       resetRAMWriteAddress(); //Fresh recording
                       memBuffer.sample_1.u16XPosition = pos_and_gesture_struct.u16XPosition;
                       memBuffer.sample_1.u16YPosition = pos_and_gesture_struct.u16YPosition;
                       memBuffer.sample_1.u16ZPosition = pos_and_gesture_struct.u16ZPosition;
                       u8BufferDataCount = 1;
                       setSwitchLEDState(SWITCH_LED_RED_BLINKING);
                    }else{
                       armPlayback();
                    }
               }
               else{
                   setMenuExitFlag();
               }
                break;
            case ENC_SWITCH_RELEASED://PLAYBACK
                /*Switch release only matters for gate record mode.
                  In gate mode, the release of the switch end recording.*/
                if(VectrData.u8Control[PLAY] == GATE && VectrData.u8Source[PLAY] == SWITCH){
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
                if(VectrData.u8Source[OVERDUB] == SWITCH){
                    u8OverdubRunFlag ^= TRUE;

                    if(u8OverdubRunFlag == TRUE){
                        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
                    }
                    else{
                        setSwitchLEDState(OFF);
                    }
                }
//                }else{//External Control - Nothing for overdub
//
//                }
                break;
            case MAIN_SWITCH_PRESSED://OVERDUBBING
                //Main switch press activates/deactivates overdub
                if(VectrData.u8Source[OVERDUB] == SWITCH){
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
                if(VectrData.u8Control[OVERDUB] == GATE && VectrData.u8Source[OVERDUB] == SWITCH){
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
                    if(VectrData.u8Source[PLAY] == SWITCH){
                        if(u8PlaybackRunFlag != RUN){
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
        default:
            break;
        }
    }
}

void resetSpeed(void){
    i16PlaybackData = 0;
    u32PlaybackSpeed = STANDARD_PLAYBACK_SPEED;
    SET_SAMPLE_TIMER_PERIOD(STANDARD_PLAYBACK_SPEED);
    RESET_SAMPLE_TIMER;
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
    }
}

void adjustSpeedModulation(uint16_t u16NewValue){
    u16PlaybackSpeedTableIndex = u16NewValue;
    u32PlaybackSpeed = u32LogSpeedTable[u16NewValue];
    SET_SAMPLE_TIMER_PERIOD(u32PlaybackSpeed);
    RESET_SAMPLE_TIMER;
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
            if(VectrData.u8OverdubStatus[X_OUTPUT_INDEX]){
                setLeftLEDs(HALF_BRIGHTNESS, BLINK);
            }
            else{
                setLeftLEDs(0, OFF);
            }
            if(VectrData.u8OverdubStatus[Y_OUTPUT_INDEX]){
                setTopLEDs(HALF_BRIGHTNESS, BLINK);
            }
            else{
                setTopLEDs(0, OFF);
            }
            if(VectrData.u8OverdubStatus[Z_OUTPUT_INDEX]){
                setRightLEDs(HALF_BRIGHTNESS, BLINK);
            }
            else{
                setRightLEDs(0, OFF);
            }
            break;
        default:
            break;
    }
}

uint8_t getOverdubStatus(uint8_t u8Axis){
    return VectrData.u8OverdubStatus[u8Axis];
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
    return VectrData.u16Quantization[u8Index];
}

void setCurrentQuantization(uint8_t u8Index, uint16_t u16CurrentParameter){
    VectrData.u16Quantization[u8Index] = u16CurrentParameter;
}

/*This function uses the slew parameter to slow the rate of change of the position*/
void slewPosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    uint16_t u16XSlewRate  = VectrData.u16SlewRate[X_OUTPUT_INDEX];
    uint16_t u16YSlewRate  = VectrData.u16SlewRate[Y_OUTPUT_INDEX];
    uint16_t u16ZSlewRate  = VectrData.u16SlewRate[Z_OUTPUT_INDEX];
    uint16_t u16CurrentXPosition = VectrData.u16CurrentXPosition;
    uint16_t u16CurrentYPosition = VectrData.u16CurrentYPosition;
    uint16_t u16CurrentZPosition = VectrData.u16CurrentZPosition;
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
    else{
        u16NewXPosition = u16CurrentXPosition;
    }

    if(i32YDifference > u16YSlewRate){
        u16NewYPosition = u16CurrentYPosition + u16YSlewRate;
    }
    else if(i32YDifference < -u16YSlewRate){
        u16NewYPosition = u16CurrentYPosition - u16YSlewRate;
    }
    else{
        u16NewYPosition = u16CurrentYPosition;
    }

    if(i32ZDifference > u16ZSlewRate){
        u16NewZPosition = u16CurrentZPosition + u16ZSlewRate;
    }
    else if(i32ZDifference < -u16ZSlewRate){
        u16NewZPosition = u16CurrentZPosition - u16ZSlewRate;
    }
    else{
        u16NewZPosition = u16CurrentZPosition;
    }

    VectrData.u16CurrentXPosition = u16NewXPosition;
    VectrData.u16CurrentYPosition = u16NewYPosition;
    VectrData.u16CurrentZPosition = u16NewZPosition;
    p_pos_and_gesture_struct->u16XPosition = u16NewXPosition;
    p_pos_and_gesture_struct->u16YPosition = u16NewYPosition;
    p_pos_and_gesture_struct->u16ZPosition = u16NewZPosition;

}


/*In this function we make a logarithmic or antilogarithmic curve to the output by blending
 together a log table with the linear value to make more or less logarithmic outputs. We
 basically take a fraction of each
 * (32*Linear value + 32*log table value / 64)  = half logarithmic*/
void linearizePosition(pos_and_gesture_data * p_pos_and_gesture_struct){
    uint16_t u16Linearity[NUM_OF_AXES]  = {  VectrData.u16Linearity[X_OUTPUT_INDEX],
                                                VectrData.u16Linearity[Y_OUTPUT_INDEX],
                                                VectrData.u16Linearity[Z_OUTPUT_INDEX]};
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

//    VectrData.u16CurrentXPosition = u16NewPosition[X_OUTPUT_INDEX];
//    VectrData.u16CurrentYPosition = u16NewPosition[Y_OUTPUT_INDEX];
//    VectrData.u16CurrentZPosition = u16NewPosition[Z_OUTPUT_INDEX];
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

        switch(VectrData.u8Range[i]){
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
                //ramp from beginning of loop to end of loop
                break;
        }

        u16CurrentPosition[i] = u32temp;

    }

    p_pos_and_gesture_struct->u16XPosition = u16CurrentPosition[0];
    p_pos_and_gesture_struct->u16YPosition = u16CurrentPosition[1];
    p_pos_and_gesture_struct->u16ZPosition = u16CurrentPosition[2];

}

uint8_t * getDataStructAddress(void){
    return &VectrData.u8Range[0];
}

uint16_t getCurrentRange(uint8_t u8Index){
    return VectrData.u8Range[u8Index];
}

void setCurrentRange(uint8_t u8Index, uint8_t u8NewValue){
    VectrData.u8Range[u8Index] = u8NewValue;
}

uint16_t getCurrentLinearity(uint8_t u8Index){
    return VectrData.u16Linearity[u8Index];
}

void setCurrentLinearity(uint8_t u8Index, uint8_t u8NewValue){
    VectrData.u16Linearity[u8Index] = u8NewValue;
}

uint16_t getCurrentSlewRate(uint8_t u8Index){
    return VectrData.u16SlewRate[u8Index];
}

void setCurrentSlewRate(uint8_t u8Index, uint8_t u8NewValue){
    VectrData.u16SlewRate[u8Index] = u8NewValue;
}

uint8_t getCurrentTrackBehavior(uint8_t u8Index){

}

void setCurrentTrackBehavior(uint8_t u8Index, uint8_t u8NewValue){

}

/*Returns the current source for Record, Playback, or Overdub*/
uint8_t getCurrentSource(uint8_t u8Parameter){
    return VectrData.u8Source[u8Parameter];
}

uint8_t setCurrentSource(uint8_t u8Parameter, uint8_t u8NewSetting){
    if(u8NewSetting >= NUM_OF_SOURCE_SETTINGS){
        u8NewSetting = 0;
    }

    VectrData.u8Source[u8Parameter] = u8NewSetting;

    return u8NewSetting;
}

uint8_t getPlaybackRunStatus(void){
    return u8PlaybackRunFlag;
}

void setPlaybackRunStatus(uint8_t u8NewState){
    u8PlaybackRunFlag = u8NewState;

    //Turn the switch LED on or off
    if(u8PlaybackRunFlag == TRUE){
        setSwitchLEDState(SWITCH_LED_GREEN_BLINKING);
    }
    else{
        setSwitchLEDState(SWITCH_LED_OFF);
    }
}

uint8_t getCurrentControl(uint8_t u8Index){
    return VectrData.u8Control[u8Index];
}

uint8_t setCurrentControl(uint8_t u8Index, uint8_t u8NewSetting){

    VectrData.u8Control[u8Index] = u8NewSetting;

    return u8NewSetting;
}

uint8_t getCurrentLoopMode(void){
    return VectrData.u8PlaybackMode;
}

void setCurrentLoopMode(uint8_t u8NewSetting){
    VectrData.u8PlaybackMode = u8NewSetting;
    if(u8NewSetting == LOOPING){
        setPlaybackDirection(FORWARD_PLAYBACK);
    }
}

uint8_t getCurrentClockMode(void){
    return VectrData.u8ClockMode;
}

void setCurrentClockMode(uint8_t u8NewSetting){
    VectrData.u8ClockMode = u8NewSetting;
}

uint8_t getCurrentModulationMode(void){
    return VectrData.u8ModulationMode;
}

void setCurrentModulationMode(uint8_t u8NewSetting){
    VectrData.u8ModulationMode = u8NewSetting;
}

uint8_t getSequenceRecordedFlag(void){
    return u8SequenceRecordedFlag;
}

void setSequenceRecordFlag(uint8_t u8NewState){
    u8SequenceRecordedFlag = u8NewState;
}

uint8_t getMuteStatus(uint8_t u8Index){
    return VectrData.u8MuteState[u8Index];
}

uint8_t getCurrentSequenceIndex(void){
    return u8SequenceModeIndexes[u8SequenceModeSelectedSequenceIndex];
}

void setNumberOfClockPulses(void){
    u8NumOfClockPulses = 1 << VectrData.u8ClockMode;
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

void setResetFlag(uint8_t u8NewState){
    u8ResetFlag = u8NewState;
}

VectrDataStruct * getVectrDataStart(void){
    return &VectrData;
}