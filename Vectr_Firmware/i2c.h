/*
 * File:   i2c.h
 * Author: matthewheins
 *
 * Created on February 2, 2014, 9:53 PM
 */

#ifndef I2C_H
#define	I2C_H

#include "app.h"

#define MAX_I2C_BUFFER_LENGTH	150
#define GESTIC_I2C_ADDR 0x42 // default i2c address of MGC3130
#define MGC3130_ADDRESS_READ   GESTIC_I2C_ADDR<<1 | 0x01
#define MGC3130_ADDRESS_WRITE    GESTIC_I2C_ADDR<<1 & 0XFE
#define MAX_LEN_TO_GESTIC 16 /* maximum length of message to be transferred
                              * to GestIC. Used to setup buffers
                              * Possible values: 4...255*/

/*I2C MGC3130 States*/
enum{
    SET_START_CONDITION = 0,
    SEND_SLAVE_ADDRESS,
    START_RECEIVER,
    GET_MESSAGE_LENGTH,
    CLOCK_MESSAGE_BYTE,
    RECEIVE_MESSAGE_BYTE,
    SEND_STOP_CONDITION
};

#define MGC3130_ADDRESS	0x42<<1
#define MAX_LEN_TO_GESTIC 16 /* maximum length of message to be transferred
                              * to GestIC. Used to setup buffers
                              * Possible values: 4...255*/
#define NUMBER_OF_MGC3130_BYTES	16
#define NUMBER_OF_MGC3130_POSITION_BYTES	20

#define MGC3130_MESSAGE_TYPE_POSITION_DATA	0x98

#define MGC3130_MSG_LENGTH_BYTE 0
#define MGC3130_MSG_FLAG_BYTE   1
#define MGC3130_MESSAGE_ID	3
#define MGC3130_DATA_OUTPUT_CONFIG_BYTE_1   4
#define MGC3130_DATA_OUTPUT_CONFIG_BYTE_2   5
#define MGC3130_TIME_STAMP      6
#define MGC3130_SYSTEM_INFO     7
#define MGC3130_DSP_INFO_LENGTH 2
#define MGC3130_DATA_START_BYTE 8
#define MGC3130_GESTURE_LENGTH  4
#define MGC3130_TOUCH_LENGTH    4
#define MGC3130_AIRWHEEL_LENGTH 2
#define MGC3130_POSITION_LENGTH 6

#define MGC3130_DSP_STATUS_PRESENT_MASK     0x01
#define MGC3130_GESTURE_PRESENT_MASK        0x02
#define MGC3130_TOUCH_PRESENT_MASK          0x04
#define MGC3130_AIRWHEEL_PRESENT_MASK       0x08
#define MGC3130_POSITION_PRESENT_MASK       0x10


#define MGC3130_POSITION_VALID_MASK     0x01
#define MGC3130_AIRWHEEL_VALID_MASK     0x02




#define MGC3130_TOUCH_BYTE_1    12
#define MGC3130_TOUCH_BYTE_2    13

#define MGC3130_AIRWHEEL_BYTE_1 16
#define MGC3130_X_POS_LOW_BYTE	18
#define MGC3130_X_POS_HIGH_BYTE	19
#define MGC3130_Y_POS_LOW_BYTE	20
#define MGC3130_Y_POS_HIGH_BYTE	21
#define MGC3130_Z_POS_LOW_BYTE	22
#define MGC3130_Z_POS_HIGH_BYTE	23


//Gesture masks
#define MGC3130_FLICK_LEFT_TO_RIGHT			0x02
#define MGC3130_FLICK_RIGHT_TO_LEFT			0x03
#define MGC3130_FLICK_SOUTH_TO_NORTH		0x04
#define MGC3130_FLICK_NORTH_TO_SOUTH		0x05
#define MGC3130_CIRCLE_CLOCKWISE			0x06
#define MGC3130_CIRCLE_COUNTER_CLOCKWISE	0x07

#define MGC3130_INVALID_SAMPLE_MASK			0xC0000200

//Touch Masks
#define MGC3130_NO_TOUCH            0x0000
#define MGC3130_TOUCH_BOTTOM        0x0001
#define MGC3130_TOUCH_LEFT          0x0002
#define MGC3130_TOUCH_TOP           0x0004
#define MGC3130_TOUCH_RIGHT         0x0008
#define MGC3130_TOUCH_CENTER        0x0010
#define MGC3130_TAP_BOTTOM          0x0020
#define MGC3130_TAP_LEFT            0x0040
#define MGC3130_TAP_TOP             0x0080
#define MGC3130_TAP_RIGHT           0x0100
#define MGC3130_TAP_CENTER          0x0200
#define MGC3130_DOUBLE_TAP_BOTTOM   0x0400
#define MGC3130_DOUBLE_TAP_LEFT     0x0800
#define MGC3130_DOUBLE_TAP_TOP      0x1000
#define MGC3130_DOUBLE_TAP_RIGHT    0x2000
#define MGC3130_DOUBLE_TAP_CENTER   0x4000

//MGC3130 Message IDs
#define MGC3130_SYSTEM_STATUS           0x15
#define MGC3130_REQUEST_MESSAGE         0x06
#define MGC3130_FW_VERSION_INFO         0x83
#define MGC3130_SET_RUNTIME_PARAMETER   0xA2
#define MGC3130_SENSOR_DATA_OUTPUT      0x91

#define MGC3130_HEADER_SIZE_BYTE    1
#define MGC3130_HEADER_FLAGS_BYTE   2
#define MGC3130_HEADER_SEQ_BYTE     3
#define MGC3130_HEADER_ID_BYTE      4


#define	ENABLE_MGC3130_INTERRUPT	1
#define DISABLE_MGC3130_INTERRUPT 	1

#define MGC3130_NO_DATA_MASK            0xFFFF
#define MGC3130_MAX_VALUE		0xFFFF

#define LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE 16

#define DATA_OUTPUT_ENABLE_MASK 0xA0

#define MSG_ID_CHANNEL_MAPPING_S   0x65
#define MSG_ID_CHANNEL_MAPPING_W   0x66
#define MSG_ID_CHANNEL_MAPPING_N   0x67
#define MSG_ID_CHANNEL_MAPPING_E   0x68
#define MSG_ID_CHANNEL_MAPPING_C   0x69

enum{
    RX0 = 0,
    RX1,
    RX2,
    RX3,
    RX4
};

void resetMGC3130(void);
uint8_t mgc3130_extract_pos_and_gest_data(uint8_t * u8ReceiveBuffer, pos_and_gesture_data * position_and_gesture);
void zero_pos_and_gest_data(pos_and_gesture_data * position_and_gesture);
void initI2C_MGC3130(int baudRate, int clockFrequency);
void initMGC3130(void);
void readMGC3130FirmwareVersion(void);
void configureMGC3130(unsigned char * message);
uint8_t readStatusMessage(void);
uint8_t mgc3130StateMachine(pos_and_gesture_data * pos_and_gesture_struct);

extern unsigned char msgMGC3130Configure[LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE] ;
extern unsigned char msgMGC3130InvertNorth[LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE];
extern unsigned char msgMGC3130InvertSouth[LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE];


#endif	/* I2C_H */