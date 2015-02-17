/* 
 * File:   master_control.h
 * Author: matthewheins
 *
 * Created on April 27, 2014, 9:01 PM
 */

#ifndef MASTER_CONTROL_H
#define	MASTER_CONTROL_H

enum{
    ENC_SWITCH_PRESSED = 0,
    ENC_SWITCH_RELEASED,
    MAIN_SWITCH_PRESSED,
    MAIN_SWITCH_RELEASED
};

#define NUMBER_OF_OUTPUTS       3
#define MAXIMUM_OUTPUT_VALUE    0xFFFF
#define HALF_OUTPUT_VALUE       0x7FFF
#define ONE_VOLT_OUTPUT_VALUE   0x9996
#define HALF_VOLT_OUTPUT_VALUE  0x8CC4
#define ZERO_VOLT_OUTPUT_VALUE  0x7FFF
#define QUARTER_OUTPUT_VALUE    0x3FFF
#define MIN_OUTPUT_VALUE        0

#define NUM_OF_AXES         3
#define X_OUTPUT_INDEX      0
#define Y_OUTPUT_INDEX      1
#define Z_OUTPUT_INDEX      2
#define CENTER_INDEX        3

#define INITIAL_SPEED_INDEX 511
#define MINIMUM_SPEED_INDEX 0
#define MAXIMUM_SPEED_INDEX 1023


#define NUM_OF_PLAYBACK_SETTINGS    3
enum{
    RECORD = 0,
    PLAY,
    OVERDUB
};

enum{
    SWITCH = 0,
    EXTERNAL
};

enum{
    TRIGGER = 0,
    GATE,
    TRIGGER_AUTO
};

#define NUM_OF_SOURCE_SETTINGS  2
#define NUM_OF_CONTROL_SETTINGS 2

//Setting armed or disarmed states
enum{
    NOT_ARMED = 0,
    ARMED,
    DISARMED
};

/*IO Message Types - These messages are sent by the IO Handler to
 * cause the state of the device to change. They are aggregated on one queue
 * to reduce processing overhead.
 */

typedef struct{
    uint16_t u16messageType;//Which input?
    uint16_t u16message;//What happened?
}io_event_message;

enum{
    RECORD_IN_EVENT = 0,
    PLAYBACK_IN_EVENT,
    HOLD_IN_EVENT,
    SYNC_IN_EVENT,
    JACK_DETECT_IN_EVENT
};

#define GATE_WENT_HIGH  2

enum{
    TRIGGER_WENT_HIGH = 0,
    TRIGGER_WENT_LOW,
    NO_TRIGGER
};

enum{
    NO_HOLD_EVENT = 0,
    HOLD_ACTIVATE,
    HOLD_DEACTIVATE
};

//Track Behavior Modes
#define MAXIMUM_TRACK_BEHAVIOR_INDEX    4
enum{
    HOLD = 0,
    TRACKLIVE,
    LIVE,
    ZERO,
    ENVELOPE_ZERO
};

enum{
    STOP = 0,
    RUN,
    ENDED,
    PAUSED
};

//Playback modes
enum{
    LOOPING = 0,
    PENDULUM,
    FLIP,
    ONESHOT,
    RETRIGGER
};

//Playback directions
enum{
    FORWARD_PLAYBACK = 0,
    REVERSE_PLAYBACK
};

//Gate Modes
enum{
    HAND_GATE = 0,
    RESET_GATE,
    PLAY_GATE,
    RECORD_GATE,
    QUANT_GATE
};

#define MAXIMUM_QUANTIZATION_INDEX  4
//Quantization Modes
enum{
    NO_QUANTIZATION = 0,
    CHROMATIC,
    MAJOR,
    PENTATONIC,
    OCTAVE
};

//Modulation modes.
enum{
    SPEED,
    QUANTIZED_SPEED,
    SCRUB,
    TRIM,
    MIRROR
};

//Memory definitions
#define NUM_OF_DATA_ITEMS_FOR_BUFFER    2

enum{
    READ_RAM = 0,
    WRITE_RAM,
    WRITETHENREAD_RAM,
    READ_FLASH_FILE_TABLE,
    WRITE_FLASH_FILE_TABLE,

};

//Sequence Mode
#define MAX_NUM_OF_SEQUENCES    4
#define NO_SEQUENCE_MASK    0xFF
#define RAM_SEQUENCE_MASK   0xFE

/*Sequence States - These are */
enum{
    NO_SEQUENCE_STORED = 0,
    SEQUENCE_STORED,
    SEQUENCE_SELECTED
};

#define MAX_NUM_OF_CLOCK_PULSES 16
#define CLOCK_PULSE_COUNT_INCREMENT 6

//Number of clock pulse settings
enum{
    CLOCK_PULSE_1 = 0,
    CLOCK_PULSE_2,
    CLOCK_PULSE_4,
    CLOCK_PULSE_8,
    CLOCK_PULSE_16
};

/*Air Scratch constants*/
#define SCRATCH_SLOWDOWN_CONSTANT   10
#define SCRATCH_SPEEDUP_CONSTANT    10

/*Quantization*/
//1 VOLT PER OCTAVE = .083333V/NOTE
//16 BIT = 65535 STEPS FOR 10 VOLTS = 120 NOTES
//1 NOTE = 546.133 STEPS PER NOTE
#define CHROMATIC_NOTE_SPACING


#define MIN_SLEW_RATE   0
#define MID_SLEW_RATE   1024
#define MAX_SLEW_RATE   2048
#define SLEW_RATE_LED_INCREMENT 512
#define SLEW_RATE_INCREMENT 16
#define SLEW_RATE_LED_SCALING   1 

#define LINEARITY_MIN       0   //Pure log
#define LINEARITY_STRAIGHT  64  //Linear
#define LINEARITY_MAX       127 //Pure antilog
#define LINEARITY_LED_INCREMENT 32
#define BIT_LENGTH_OF_LINEARITY_STRAIGHT    6
#define LINEARITY_LED_SCALING   16 //MAX BRIGHTNESS divided by the size of
/*Record, Playback, and Overdub all have */

#define MAXIMUM_RANGE_INDEX 4
enum{
    RANGE_BI_5V = 0,
    RANGE_BI_2_5V,
    RANGE_UNI_5V,
    RANGE_UNI_1V,
    RAMP
};

//Operating Modes
enum{
    LIVE_PLAY=0,
    RECORDING,
    PLAYBACK,
    OVERDUBBING,
    SEQUENCING,
    MUTING,
    AIR_SCRATCHING
};

//Performance Modes
enum{
    NO_PERFORMANCE_ACTIVE = 0,
    MENU_MODE,
    OVERDUB_MODE,
    MUTE_MODE,
    SEQUENCER_MODE,
    AIR_SCRATCH_MODE
};

#define DEFAULT_SETTINGS_TIME_LENGTH    30000

typedef struct{
    uint8_t u8Range[NUMBER_OF_OUTPUTS];//Keep this as the first entry...things could get messed up.
    uint16_t u16CurrentXPosition;
    uint16_t u16CurrentYPosition;
    uint16_t u16CurrentZPosition;
    uint16_t u16SlewRate[NUMBER_OF_OUTPUTS];
    uint16_t u16Linearity[NUMBER_OF_OUTPUTS];
    uint8_t u8Source[NUM_OF_PLAYBACK_SETTINGS];
    uint8_t u8Control[NUM_OF_PLAYBACK_SETTINGS];
    uint8_t u8PlaybackMode;//Looping or whatnot
    uint8_t u8PlaybackDirection;//forward or reverse
    uint8_t u8ModulationMode;//Speed or Scrub
    uint8_t u8HoldBehavior[NUMBER_OF_OUTPUTS];//Hold the outputs
    uint8_t u8OverdubStatus[NUMBER_OF_OUTPUTS];
    uint8_t u16Quantization[NUMBER_OF_OUTPUTS];
    uint8_t u8MuteState[NUMBER_OF_OUTPUTS];
    uint8_t u8ClockMode;
    uint8_t u8GateMode;
    uint8_t u8NumRecordClocks;//The number of clocks to be counted for auto recording and also
                                //the number of clock multiples for external recording.
}VectrDataStruct;


void MasterControlInit(void);
void setKeyPressFlag(void);
void setEncKeyPressFlag(void);
uint16_t getCurrentRange(uint8_t u8Index);
void setCurrentRange(uint8_t u8Index, uint8_t u8NewValue);
void setMenuModeFlag(uint8_t u8NewMode);
uint8_t getCurrentSource(uint8_t u8Index);
uint8_t setCurrentSource(uint8_t u8Index, uint8_t u8NewValue);
uint8_t getCurrentControl(uint8_t u8Index);
uint8_t setCurrentControl(uint8_t u8Index, uint8_t u8NewValue);
uint8_t getCurrentLoopMode(void);
void setCurrentLoopMode(uint8_t u8NewSetting);
uint8_t getCurrentClockMode(void);
void setCurrentClockMode(uint8_t u8NewSetting);
uint8_t getCurrentModulationMode(void);
void setCurrentModulationMode(uint8_t u8NewSetting);
uint8_t getOverdubStatus(uint8_t u8Axis);
void setLivePlayActivationFlag(void);
uint8_t getHoldState(void);
uint8_t getPlaybackDirection(void);
void setPlaybackDirection(uint8_t u8NewDirection);
uint8_t getPlaybackMode(void);
void setHandPresentFlag(uint8_t u8NewState);
void setPulseTimerExpiredFlag(void);
uint8_t getCurrentQuantization(uint8_t u8Index);
void setCurrentQuantization(uint8_t u8Index, uint16_t u16CurrentParameter);
uint8_t * getDataStructAddress(void);
uint8_t getSequenceRecordedFlag(void);
uint8_t getPlaybackRunStatus(void);
void setPlaybackRunStatus(uint8_t u8NewState);
uint8_t getSequenceModeState(uint8_t u8SequenceIndex);
void enterOverdubMode(void);
void enterAirScratchMode(void);
void enterMuteMode(void);
void enterSequencerMode(void);
uint8_t getMuteStatus(uint8_t u8Index);
uint8_t getCurrentSequenceIndex(void);
uint16_t getCurrentLinearity(uint8_t u8Index);
void setCurrentLinearity(uint8_t u8Index, uint8_t u8NewValue);
uint16_t getCurrentSlewRate(uint8_t u8Index);
void setCurrentSlewRate(uint8_t u8Index, uint16_t u16NewValue);
uint8_t getCurrentTrackBehavior(uint8_t u8Index);
void setCurrentTrackBehavior(uint8_t u8Index, uint8_t u8NewValue);
uint32_t getNextClockPulseIndex(void);
uint8_t getGatePulseFlag(void);
void setGatePulseFlag(uint8_t u8NewState);
VectrDataStruct * getVectrDataStart(void);
void clearNextClockPulseIndex(void);
uint32_t calculateNextClockPulse(void);
uint8_t getCurrentGateMode(void);
void setCurrentGateMode(uint8_t u8NewState);
void setResetFlag(void);
uint8_t getResetFlag(void);
void clearResetFlag(void);
void defaultSettings(void);
uint8_t getCurrentRecordClocks(void);
void setCurrentRecordClocks(uint8_t u8NewSetting);
uint8_t getRecordSource(void);


#endif	/* MASTER_CONTROL_H */

