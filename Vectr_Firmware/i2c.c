#include <stdio.h>
#include <stdlib.h>
#include "i2c.h"
#include "peripheral/peripheral.h"
#include "app.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "system_config.h"


uint8_t u16MessageLength;
uint8_t u8Count;
uint8_t u8Data;
uint16_t u16receptionCount;
uint8_t u8TransmissionCount;
uint8_t u8I2CMode;
unsigned char *msg;
uint8_t u8I2CState = SET_START_CONDITION,
            u8MessageLength,
            u8Count;
uint8_t u8ReceiveBuffer[MAX_I2C_BUFFER_LENGTH];


#define LENGTH_OF_CONFIG_MESSAGE   16

unsigned char msgMGC3130Configure[LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE] = {
    LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE, // length
    0x00, // message flags
    0x00, // sequence number
    MGC3130_SET_RUNTIME_PARAMETER, // message id: set runtime parameter
    DATA_OUTPUT_ENABLE_MASK, 0, // parameter id:
    0, 0, // dummy of 16 bits length
    0x1E, 0x00, 0x00, 0x00, /* arg0 (32Bit): Enable all outputs*/
    0xFF, 0xFF, 0xFF, 0xFF, /*Change everything*/
};

unsigned char msgMGC3130InvertNorth[LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE] = {
    LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE, // length
    0x00, // message flags
    0x00, // sequence number
    MGC3130_SET_RUNTIME_PARAMETER, // message id: set runtime parameter
    MSG_ID_CHANNEL_MAPPING_N, 0, // parameter id:
    0, 0, // dummy of 16 bits length
    RX1, 0x00, 0x00, 0x00, /* arg0 (32Bit): Enable all outputs*/
    0x00, 0x00, 0x00, 0x00, /*arg1 not used*/
};

unsigned char msgMGC3130InvertSouth[LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE] = {
    LENGTH_OF_SET_RUNTIME_PARAMETER_MESSAGE, // length
    0x00, // message flags
    0x00, // sequence number
    MGC3130_SET_RUNTIME_PARAMETER, // message id: set runtime parameter
    MSG_ID_CHANNEL_MAPPING_S, 0, // parameter id:
    0, 0, // dummy of 16 bits length
    RX4, 0x00, 0x00, 0x00, /* arg0 (32Bit): Enable all outputs*/
    0x00, 0x00, 0x00, 0x00, /*arg1 not used*/
};

uint8_t mgc3130_dynamic_extract_pos_and_gest_data(uint8_t * u8ReceiveBuffer, pos_and_gesture_data * position_and_gesture);

void initI2C_MGC3130(int baudRate, int clockFrequency){
    PLIB_I2C_BaudRateSet(I2C_MGC3130, clockFrequency, baudRate);
    PLIB_I2C_Enable(I2C_MGC3130);
}

/*Resets the MGC3130. Hold the reset for 1ms. Then wait for 5ms.*/
void resetMGC3130(void){
    /*Initiate Reset*/
    MGC3130_ACTIVATE_RESET;
    vTaskDelay(1*TICKS_PER_MS);
    MGC3130_CLEAR_RESET;
    vTaskDelay(5*TICKS_PER_MS);

    u8I2CState = SET_START_CONDITION;
}

uint8_t mgc3130StateMachine(pos_and_gesture_data * pos_and_gesture_struct){
    uint8_t u8Success = FALSE;
    int i;

    //Send the start bit 
    xSemaphoreTake(xSemaphoreMGC3130DataReady,10*TICKS_PER_MS);
    if(TS_LINE_IS_LOW){
        /*Pull the TS Line Low*/
        MGC3130_TS_PIN_LOW;
        /*Check to see if the Send the start bit.*/
        if(PLIB_I2C_BusIsIdle(I2C_MGC3130)){
            PLIB_I2C_MasterStart(I2C_MGC3130);
        }

        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Start transmitted
        /*Send the slave address*/
        if(PLIB_I2C_TransmitterIsReady(I2C_MGC3130)){
            PLIB_I2C_TransmitterByteSend(I2C_MGC3130, MGC3130_ADDRESS_READ);
        }

        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Address transmitted
        /*Get the I2C module to clock out to receive a byte.*/
        if(PLIB_I2C_TransmitterByteWasAcknowledged(I2C_MGC3130)){
            PLIB_I2C_MasterReceiverClock1Byte(I2C_MGC3130);

        }

        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Message Length Received.
        /*The first byte from the MGC3130 is the length of the message*/
        /*The first byte from the MGC3130 is the length of the message*/
        u8ReceiveBuffer[0] = PLIB_I2C_ReceivedByteGet(I2C_MGC3130);
        u8MessageLength = u8ReceiveBuffer[0];

        if(u8MessageLength > 0 && u8MessageLength < 0xFF){
            for(i = 1; i<u8MessageLength; i++){
                PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, true);
                xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Ack completed
                PLIB_I2C_MasterReceiverClock1Byte(I2C_MGC3130);
                xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Byte Received
                u8ReceiveBuffer[i] = PLIB_I2C_ReceivedByteGet(I2C_MGC3130);
            }
            PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, false);
        }
        else{
            PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, false);
        }

        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);

        PLIB_I2C_MasterStop(I2C_MGC3130);

        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Stop bit complete
        MGC3130_TS_PIN_HIGH;


        if(u8MessageLength > 0){
            u8Success = mgc3130_dynamic_extract_pos_and_gest_data(u8ReceiveBuffer, pos_and_gesture_struct);
            return u8Success;
        }
        else{
            u8Success = FALSE;
        }
    
    }

    return u8Success;//Initialized as false.
}

uint8_t mgc3130_dynamic_extract_pos_and_gest_data(uint8_t * u8ReceiveBuffer, pos_and_gesture_data * position_and_gesture){

    uint8_t u8CurrentOffset = MGC3130_DATA_START_BYTE;//Data start offset is 8.
    uint8_t u8SystemStatus = u8ReceiveBuffer[MGC3130_SYSTEM_INFO];
    uint16_t    u16XPosition,
                u16YPosition,
                u16ZPosition;

    uint16_t u16OutputConfigWord = u8ReceiveBuffer[MGC3130_DATA_OUTPUT_CONFIG_BYTE_2] <<8;
    u16OutputConfigWord += u8ReceiveBuffer[MGC3130_DATA_OUTPUT_CONFIG_BYTE_1];

    if(u16OutputConfigWord & MGC3130_DSP_STATUS_PRESENT_MASK){
        //Read this?
        u8CurrentOffset += MGC3130_DSP_INFO_LENGTH;
    }

    if(u16OutputConfigWord & MGC3130_GESTURE_PRESENT_MASK){
        position_and_gesture->u16Gesture = u8ReceiveBuffer[u8CurrentOffset];
        position_and_gesture->u16Gesture += u8ReceiveBuffer[u8CurrentOffset+1]<<8;
        u8CurrentOffset += MGC3130_GESTURE_LENGTH;
    }
    else{
        position_and_gesture->u16Gesture = 0;
    }

    if(u16OutputConfigWord & MGC3130_TOUCH_PRESENT_MASK){
        position_and_gesture->u16Touch = u8ReceiveBuffer[u8CurrentOffset];
        position_and_gesture->u16Touch += u8ReceiveBuffer[u8CurrentOffset+1]<<8;
        u8CurrentOffset += MGC3130_TOUCH_LENGTH;
    }
    else{
        position_and_gesture->u16Touch = 0;
    }

    if(u16OutputConfigWord & MGC3130_AIRWHEEL_PRESENT_MASK){
        position_and_gesture->u16Airwheel = u8ReceiveBuffer[u8CurrentOffset];
        u8CurrentOffset += MGC3130_AIRWHEEL_LENGTH;
    }
    else{
        position_and_gesture->u16Airwheel = 0;
    }

    if(u16OutputConfigWord & MGC3130_POSITION_PRESENT_MASK){
        if(u8SystemStatus & MGC3130_POSITION_VALID_MASK){

            u16XPosition = u8ReceiveBuffer[u8CurrentOffset];
            u8CurrentOffset++;
            u16XPosition += u8ReceiveBuffer[u8CurrentOffset]<<8;
            u8CurrentOffset++;
            u16YPosition = u8ReceiveBuffer[u8CurrentOffset];
            u8CurrentOffset++;
            u16YPosition += u8ReceiveBuffer[u8CurrentOffset]<<8;
            u8CurrentOffset++;
            u16ZPosition = u8ReceiveBuffer[u8CurrentOffset];
            u8CurrentOffset++;
            u16ZPosition += u8ReceiveBuffer[u8CurrentOffset]<<8;

            u16ZPosition = MGC3130_MAX_VALUE - u16ZPosition;//Invert Z

            if((u16XPosition != 0) || (u16YPosition != 0) || (u16ZPosition != 0)){
                position_and_gesture->u16XPosition = u16XPosition;
                position_and_gesture->u16YPosition = u16YPosition;
                position_and_gesture->u16ZPosition = u16ZPosition;
            }
            else{
                position_and_gesture->u16XPosition = 0;
                position_and_gesture->u16YPosition = 0;
                position_and_gesture->u16ZPosition = 0;
                return FALSE;
            }
        }
        else{
            position_and_gesture->u16XPosition = 0;
            position_and_gesture->u16YPosition = 0;
            position_and_gesture->u16ZPosition = 0;
            return FALSE;
        }
    }

    return TRUE;

}

uint8_t mgc3130_extract_pos_and_gest_data(uint8_t * u8ReceiveBuffer, pos_and_gesture_data * position_and_gesture){

	uint32_t u32_flags;
        uint32_t u32TouchData;

	position_and_gesture->u16XPosition = u8ReceiveBuffer[MGC3130_X_POS_HIGH_BYTE]<<8;
	position_and_gesture->u16XPosition += u8ReceiveBuffer[MGC3130_X_POS_LOW_BYTE];

	position_and_gesture->u16YPosition = u8ReceiveBuffer[MGC3130_Y_POS_HIGH_BYTE]<<8;
	position_and_gesture->u16YPosition += u8ReceiveBuffer[MGC3130_Y_POS_LOW_BYTE];

	position_and_gesture->u16ZPosition = u8ReceiveBuffer[MGC3130_Z_POS_HIGH_BYTE]<<8;
	position_and_gesture->u16ZPosition += u8ReceiveBuffer[MGC3130_Z_POS_LOW_BYTE];

	/*The Z parameter gets inverted so that 0 is max distance.*/
	position_and_gesture->u16ZPosition = MGC3130_MAX_VALUE - position_and_gesture->u16ZPosition;

        //Get the gestures, touch, and airwheel data
	position_and_gesture->u16Touch = u8ReceiveBuffer[MGC3130_TOUCH_BYTE_2]<<8;
	position_and_gesture->u16Touch += u8ReceiveBuffer[MGC3130_TOUCH_BYTE_1];
        position_and_gesture->u16Airwheel = u8ReceiveBuffer[MGC3130_AIRWHEEL_BYTE_1];

        return 1;
}

void zero_pos_and_gest_data(pos_and_gesture_data * position_and_gesture){
	position_and_gesture->u16XPosition = MGC3130_NO_DATA_MASK;
	position_and_gesture->u16YPosition = MGC3130_NO_DATA_MASK;
	position_and_gesture->u16ZPosition = MGC3130_NO_DATA_MASK;
	position_and_gesture->u16Gesture = MGC3130_NO_DATA_MASK;
        position_and_gesture->u16Touch = MGC3130_NO_DATA_MASK;
        position_and_gesture->u16Airwheel = MGC3130_NO_DATA_MASK;
}

void setMGC3130Inverted(void){

}

void readMGC3130FirmwareVersion(void){
    uint8_t u8MessageLength;
    uint8_t i;
           
    /*Pull the TS Line Low*/
    MGC3130_TS_PIN_LOW;
    /*Check to see if the Send the start bit.*/
    if(PLIB_I2C_BusIsIdle(I2C_MGC3130)){
        PLIB_I2C_MasterStart(I2C_MGC3130);
    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*Send the slave address*/
    if(PLIB_I2C_TransmitterIsReady(I2C_MGC3130)){
        PLIB_I2C_TransmitterByteSend(I2C_MGC3130, MGC3130_ADDRESS_READ);
    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*Get the I2C module to clock out to receive a byte.*/
    if(PLIB_I2C_TransmitterByteWasAcknowledged(I2C_MGC3130)){
        PLIB_I2C_MasterReceiverClock1Byte(I2C_MGC3130);

    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*The first byte from the MGC3130 is the length of the message*/
    u8MessageLength = PLIB_I2C_ReceivedByteGet(I2C_MGC3130);

    for(i = 1; i<u8MessageLength; i++){
        PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, true);
        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Ack completed
        PLIB_I2C_MasterReceiverClock1Byte(I2C_MGC3130);
        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Byte Received
        PLIB_I2C_ReceivedByteGet(I2C_MGC3130);

    }

    PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, false);


    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);

    PLIB_I2C_MasterStop(I2C_MGC3130);

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Stop bit complete
    MGC3130_TS_PIN_HIGH;
              
}

          

void configureMGC3130(unsigned char * message){
    uint8_t i;
    xSemaphoreTake(xSemaphoreMGC3130DataReady,10);

    /*Pull the TS Line Low*/
        MGC3130_TS_PIN_LOW;
        /*Check to see if the Send the start bit.*/
        if(PLIB_I2C_BusIsIdle(I2C_MGC3130)){
            PLIB_I2C_MasterStart(I2C_MGC3130);
            u8Count = 0;

        }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*Send the slave address*/
    if(PLIB_I2C_TransmitterIsReady(I2C_MGC3130)){
        PLIB_I2C_TransmitterByteSend(I2C_MGC3130, MGC3130_ADDRESS_WRITE);

    }

    /*Transmit the message*/
    for(i=0; i<message[0];i++){
       xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
       if(PLIB_I2C_TransmitterByteWasAcknowledged(I2C_MGC3130)){
            PLIB_I2C_TransmitterByteSend(I2C_MGC3130, message[i]);
          //  xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Wait for transmit
       }
    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);

    PLIB_I2C_MasterStop(I2C_MGC3130);

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Stop bit complete
    MGC3130_TS_PIN_HIGH;

}

uint8_t readStatusMessage(void){
    uint8_t u8MessageLength;
    uint8_t errorByte;
    uint8_t i;


    xSemaphoreTake(xSemaphoreMGC3130DataReady,10*TICKS_PER_MS);
    /*Pull the TS Line Low*/
    MGC3130_TS_PIN_LOW;
    /*Check to see if the Send the start bit.*/
    if(PLIB_I2C_BusIsIdle(I2C_MGC3130)){
        PLIB_I2C_MasterStart(I2C_MGC3130);
    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*Send the slave address*/
    if(PLIB_I2C_TransmitterIsReady(I2C_MGC3130)){
        PLIB_I2C_TransmitterByteSend(I2C_MGC3130, MGC3130_ADDRESS_READ);
    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*Get the I2C module to clock out to receive a byte.*/
    if(PLIB_I2C_TransmitterByteWasAcknowledged(I2C_MGC3130)){
        PLIB_I2C_MasterReceiverClock1Byte(I2C_MGC3130);

    }

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    /*The first byte from the MGC3130 is the length of the message*/
    u8MessageLength = PLIB_I2C_ReceivedByteGet(I2C_MGC3130);

    for(i = 1; i<u8MessageLength; i++){
        PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, true);
        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Ack completed
        PLIB_I2C_MasterReceiverClock1Byte(I2C_MGC3130);
        xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Byte Received


        if(i == 6){
            errorByte = PLIB_I2C_ReceivedByteGet(I2C_MGC3130);
        }
        else{
         PLIB_I2C_ReceivedByteGet(I2C_MGC3130);
        }

    }

    PLIB_I2C_ReceivedByteAcknowledge(I2C_MGC3130, false);


    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);

    PLIB_I2C_MasterStop(I2C_MGC3130);

    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);//Stop bit complete
    MGC3130_TS_PIN_HIGH;

     return errorByte;
}

