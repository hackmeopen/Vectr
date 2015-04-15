/*******************************************************************************
  MPLAB Harmony Application 
  
  File Name:
    app.c

  Summary:
    Application Template

  Description:
    This file contains the application logic.
 *******************************************************************************/


// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2013 released Microchip Technology Inc.  All rights reserved.

Microchip licenses to you the right to use, modify, copy and distribute
Software only when embedded on a Microchip microcontroller or digital signal
controller that is integrated into your product or third party product
(pursuant to the sublicense terms in the accompanying license agreement).

You should refer to the license agreement accompanying this Software for
additional information regarding your rights and obligations.

SOFTWARE AND DOCUMENTATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE.
IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER
CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR
OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR
CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF
SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
(INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *******************************************************************************/
// DOM-IGNORE-END


// *****************************************************************************
// *****************************************************************************
// Section: Included Files 
// *****************************************************************************
// *****************************************************************************

#include "app.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "peripheral/peripheral.h"
#include "peripheral/int/plib_int.h"
#include "i2c.h"
#include "dac.h"
#include "leds.h"
#include "mem.h"
#include "menu.h"
#include "system_config.h"
#include "master_control.h"



/*Semaphores*/

xSemaphoreHandle xSemaphoreSPITxSemaphore;
xSemaphoreHandle xSemaphoreSPIMemTxSemaphore;
xSemaphoreHandle xSemaphoreSPIMemRxSemaphore;
xSemaphoreHandle xSemaphoreSwitchPressed;
xSemaphoreHandle xSemaphoreEncSwitchPressed;
xSemaphoreHandle xSemaphoreMGC3130DataReady;
xSemaphoreHandle xSemaphoreI2CHandler;
xSemaphoreHandle xSemaphoreSampleTimer;
xSemaphoreHandle xSemaphoreDMA_0_RX;
xSemaphoreHandle xSemaphoreDMA_0_TX;
xSemaphoreHandle xSemaphoreDMA_3_RX;
xSemaphoreHandle xSemaphoreDMA_2_TX;

void vTIM3InterruptHandler(void);
void vTIM5InterruptHandler(void);
void vI2C1InterruptHandler(void);
void vEXTI1InterruptHandler(void);
void vEXTI0InterruptHandler(void);
void vSPI1InterruptHandler(void);
void vSPI2InterruptHandler(void);
void vDMA0InterruptHandler(void);
void vDMA1InterruptHandler(void);
void vDMA2InterruptHandler(void);
void vDMA3InterruptHandler(void);
void vTaskI2CMGC3130(void * pvParameters);
void vTaskSPIDAC(void * pvParameters);
void vTaskSPIMemory(void *pvParameters);
void vTaskLEDs(void * pvParameters);
void vTaskSwitch(void * pvParameters);
void vTaskIOHandler(void* pvParameters);
void vTaskMasterControl(void* pvParameters);


xQueueHandle xIOPinChangeQueue;
xQueueHandle xSPIDACQueue;//Sends data to the DAC outputs
xQueueHandle xLEDQueue;//Sends data to the LEDs
xQueueHandle xPositionQueue;//Position data from the MGC3130
xQueueHandle xRAMReadQueue;
xQueueHandle xRAMWriteQueue;
xQueueHandle xMemInstructionQueue;//Tells the memory task what to do.
xQueueHandle xSwitchQueue;//Pass the switch states
xQueueHandle xIOEventQueue;//Pass IO events to the master control function.
xQueueHandle xADCQueue;//Pass data from the ADC to the Modulation routine.

void __attribute__( (interrupt(ipl3), vector(12))) vTIM3InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(20))) vTIM5InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(32))) vI2C1InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(3))) vEXTI0InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(30))) vSPI1InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(35))) vSPI2InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(15))) vEXTI3InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(19))) vEXTI4InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(33))) vPinChangeInterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(42))) vDMA0InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(43))) vDMA1InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(44))) vDMA2InterruptWrapper( void );
void __attribute__( (interrupt(ipl3), vector(45))) vDMA3InterruptWrapper( void );


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine
// *****************************************************************************
// *****************************************************************************

/******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    MasterControlInit();


    /*Initialize the RTOS semaphores, tasks, and queues*/
    
    vSemaphoreCreateBinary(xSemaphoreSPITxSemaphore);
    vSemaphoreCreateBinary(xSemaphoreSPIMemTxSemaphore);
    vSemaphoreCreateBinary(xSemaphoreSPIMemRxSemaphore);
    vSemaphoreCreateBinary(xSemaphoreSwitchPressed);
    vSemaphoreCreateBinary(xSemaphoreEncSwitchPressed);
    vSemaphoreCreateBinary(xSemaphoreMGC3130DataReady);
    vSemaphoreCreateBinary(xSemaphoreI2CHandler);
    vSemaphoreCreateBinary(xSemaphoreSampleTimer);
    vSemaphoreCreateBinary(xSemaphoreDMA_0_RX);
    vSemaphoreCreateBinary(xSemaphoreDMA_0_TX);
    vSemaphoreCreateBinary(xSemaphoreDMA_3_RX);
    vSemaphoreCreateBinary(xSemaphoreDMA_2_TX);

    xSPIDACQueue = xQueueCreate(3,sizeof(pos_and_gesture_data));
    xPositionQueue = xQueueCreate(3,sizeof(pos_and_gesture_data));
    xLEDQueue = xQueueCreate(3,sizeof(pos_and_gesture_data));
    xIOPinChangeQueue = xQueueCreate(2,sizeof(portChangeStruct));
    xRAMReadQueue = xQueueCreate(2,sizeof(memory_data_packet));
    xRAMWriteQueue = xQueueCreate(2,sizeof(memory_data_packet));
    xMemInstructionQueue = xQueueCreate(4, sizeof(uint8_t));
    xSwitchQueue = xQueueCreate(2, sizeof(uint8_t));
    xIOEventQueue = xQueueCreate(5, sizeof(io_event_message));
    xADCQueue = xQueueCreate(2, sizeof(uint16_t));

    xTaskCreate(vTaskSwitch,/*Function Name*/
		(const signed char *)"Switch Handler",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_SWITCH_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
               );

    /*MGC3130 Task*/
    xTaskCreate(vTaskI2CMGC3130,/*Function Name*/
		(const signed char *)"MGC3130 Handler",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_I2C_MGC3130_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
               );

    xTaskCreate(vTaskSPIDAC,/*Function Name*/
		(const signed char *)"SPI_DAC_ADC",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_SPI_DAC_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
                );
    xTaskCreate(vTaskLEDs,/*Function Name*/
		(const signed char *)"LED_Handler",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_LEDS_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
               );
    xTaskCreate(vTaskMasterControl,/*Function Name*/
		(const signed char *)"Master Control",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_MASTER_CONTROL_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
               );

    xTaskCreate(vTaskSPIMemory,/*Function Name*/
		(const signed char *)"LED_Handler",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_SPI_MEM_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
               );
    
    xTaskCreate(vTaskIOHandler,/*Function Name*/
		(const signed char *)"IO_Handler",/*Handle*/
                150,/*Stack Depth*/
		NULL,/*Task Parameter*/
		TASK_IO_HANDLER_PRIORITY,/*Priority*/
		NULL/*Task Handle*/
               );
}

void vTaskMasterControl(void* pvParameters){

    RCON = 0;
    vTaskDelay(1500*TICKS_PER_MS);

    ENABLE_SAMPLE_TIMER_INT;
    ENABLE_CLOCK_TIMER_INT;
    setSwitchLEDState(SWITCH_LED_GREEN);

    for(;;){

        MasterControlStateMachine();
    }
}

/*This task handles the I2C interaction with the MGC3130.
 * MGC3130 messages are interpreted as they come in.
 */
void vTaskI2CMGC3130(void * pvParameters){
    pos_and_gesture_data pos_and_gesture_struct = {0,0,0,0};
    uint8_t u8Success;

    /*Reset the MGC3130 and get the startup stuff out of the way.*/
    resetMGC3130();
    xSemaphoreTake(xSemaphoreMGC3130DataReady,portMAX_DELAY);
    xSemaphoreTake(xSemaphoreI2CHandler,portMAX_DELAY);
    vTaskDelay(500*TICKS_PER_MS);//Wait for the reset to take effect and the MGC3130 to be running.
    ENABLE_I2C_MGC3130_INT;
    configureMGC3130(msgMGC3130Configure);
    CLEAR_MGC3130_DATA_READY_INT;
    PLIB_INT_ExternalFallingEdgeSelect(INT_ID_0, INT_EXTERNAL_INT_SOURCE0);
    ENABLE_MGC3130_DATA_READY_INT;

    for(;;){

        /*Run the state machine. If the reading was clean, */
        u8Success = mgc3130StateMachine(&pos_and_gesture_struct);
        if(u8Success){
            setHandPresentFlag(TRUE);
            xQueueSend(xPositionQueue, &pos_and_gesture_struct, 0);
        }
        else{
            setHandPresentFlag(FALSE);
        }
    }
}

void vTaskSPIDAC(void * pvParameters){

    xSemaphoreTake(xSemaphoreDMA_3_RX, 0);

    ENABLE_SPI_DACADC_DMA_TX_INT;
    ENABLE_SPI_DACADC_DMA_RX_INT;

    for(;;){
        dacDMA();
    }
}

void vTaskLEDs(void * pvParameters){
    uint8_t u8LEDState = BLUE_GROUP1;
    pos_and_gesture_data pos_and_gesture_struct;

    /*Clear*/
    clear_led_shift_registers();
    CLEAR_SR_CLK;
    turnOffAllLEDs();
    
    setLEDAlternateFuncFlag(TRUE);

    while(runPowerUpSequence()){
        ledStateMachine();
    }

    setLEDAlternateFuncFlag(FALSE);

    //Turn off the switch LED.
    setSwitchLEDState(SWITCH_LED_OFF);

    for(;;){
        ledStateMachine();
    }
}

void vTaskSPIMemory(void * pvParameters){
    memory_data_packet mem_data_struct;
    uint8_t u8Command;
    uint8_t u8MessageReceivedFlag;
    

    xSemaphoreTake(xSemaphoreSPIMemTxSemaphore, portMAX_DELAY);
    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);
    configureRAM();

    ENABLE_SPI_MEM_DMA_TX_INT;
    ENABLE_SPI_MEM_DMA_RX_INT;

    readFlashFileTable();

    /*If the unit is new, the file table needs to be established.
     If the unit has been updated to contain new settings, then the
     file table needs to be restructured.*/
    if(fileTableIsNotInitialized()){
        initializeFileTable();
    }
    else if(fileTableIsNotCurrent()){
        updateFileTable();
    }

    LoadSettingsFromFileTable();

    for(;;){

        /*This routine handles commands from the master control routine and also
         writes data to the Flash when the bus is not otherwise occupied. The point
         is to be able to store settings and to do so without interrupting playback.*/


        /*Read from the ram followed by a write when necessary.*/
        u8MessageReceivedFlag = xQueueReceive(xMemInstructionQueue, &u8Command, 7*TICKS_PER_MS);

        //If we received a command, act on it
        if(u8MessageReceivedFlag == TRUE){
            switch(u8Command){
                case READ_RAM:
                    read_DMA(&mem_data_struct);
                    xQueueSend(xRAMReadQueue, &mem_data_struct, 0);
                    break;
                case WRITE_RAM:
                    xQueueReceive(xRAMWriteQueue, &mem_data_struct, portMAX_DELAY);
                    writeRAM_DMA(&mem_data_struct, FALSE);
                    break;
                case WRITETHENREAD_RAM:
                    xQueueReceive(xRAMWriteQueue, &mem_data_struct, portMAX_DELAY);
                    writeRAM_DMA(&mem_data_struct, TRUE);
                    readRAM_DMA(&mem_data_struct);
                    xQueueSend(xRAMReadQueue, &mem_data_struct, 0);
                    break;
                case READ_FLASH_FILE_TABLE:
                    readFlashFileTable();
                    break;
                case WRITE_FLASH_FILE_TABLE:
                    writeFlashFileTable();
                    break;
                default:
                    break;
            }
        }

        //If we need to write the file table, we'll do so here.
        if(getFileTableWriteFlag() == TRUE){
            WriteFileTablePiecewise();
        }
    }
}


/*This routine handles the encoder switch and the main switch presses.*/
void vTaskSwitch(void *pvParameters){
    uint8_t u8EncSwitchState;
    uint8_t u8MainSwitchState;
    SET_SWITCH_INTERRUPT_RISING_EDGE;
    CLEAR_SWITCH_INTERRUPT;
    ENABLE_SWITCH_INTERRUPT;

    xSemaphoreTake(xSemaphoreSwitchPressed, 0);
    xSemaphoreTake(xSemaphoreEncSwitchPressed, 0);

    for(;;){

        if(xSemaphoreTake(xSemaphoreEncSwitchPressed, 5*TICKS_PER_MS)){
            u8EncSwitchState = ENC_SWITCH_PRESSED;
            xQueueSend(xSwitchQueue, &u8EncSwitchState, portMAX_DELAY);
            vTaskDelay(10*TICKS_PER_MS);
        }

        if(xSemaphoreTake(xSemaphoreSwitchPressed, 5*TICKS_PER_MS)){
            u8MainSwitchState = MAIN_SWITCH_PRESSED;
            xQueueSend(xSwitchQueue, &u8MainSwitchState, portMAX_DELAY);
            vTaskDelay(10*TICKS_PER_MS);
        }

        if((u8MainSwitchState == MAIN_SWITCH_PRESSED) && !READ_SWITCH){
            u8MainSwitchState = MAIN_SWITCH_RELEASED;
            xQueueSend(xSwitchQueue, &u8MainSwitchState, portMAX_DELAY);
            CLEAR_SWITCH_INTERRUPT;
            ENABLE_SWITCH_INTERRUPT;
        }

        if((u8EncSwitchState == ENC_SWITCH_PRESSED) && READ_ENC_SWITCH){
            u8EncSwitchState = ENC_SWITCH_RELEASED;
            xQueueSend(xSwitchQueue, &u8EncSwitchState, portMAX_DELAY);
            CLEAR_ROTARY_ENC_SWITCH_INT;
            ENABLE_ROTARY_ENC_SWITCH_INT;
        }
    }
}


/*This time and the stuff below counts to generate clock outputs
 and also counts the time between incoming record and playback
 clock inputs.*/
static uint32_t u32ClockTimer;
static uint32_t u32ClockTimerTriggerCount;
static uint8_t u8ClockPulseFlag;
static uint8_t u8GatePulseFlag;
static uint8_t u8ClockEnableFlag = TRUE;
static uint32_t u32RecClockCount;
static uint32_t u32LastRecClockCount;
static uint32_t u32AirwheelClockTimerTriggerCount;
static uint32_t u32AirwheelClockTimer;

void setClockPulseFlag(void){
    u8ClockPulseFlag = TRUE;
}

void setClockTimerTriggerCount(uint32_t u32NewTriggerCount){
    u32ClockTimerTriggerCount = u32NewTriggerCount;
}

void setAirwheelClockTimerTriggerCount(uint32_t u32NewTriggerCount){
    u32AirwheelClockTimerTriggerCount = u32NewTriggerCount;
}

void resetClockTimer(void){
    u32ClockTimer = 0;
}

void setClockEnableFlag(uint8_t u8NewState){
    u8ClockEnableFlag = u8NewState;
}

void resetRecClockCount(void){
    u32RecClockCount = 0;
}

uint32_t getRecClockCount(void){
    return u32RecClockCount;
}

uint32_t getLastRecClockCount(void){
    return u32LastRecClockCount;
}

void vTIM3InterruptHandler(void)
{
    uint8_t u8TapTempoSetFlag;
    uint8_t u8ExternalAirWheelActiveFlag;

    CLEAR_CLOCK_TIMER_INT;

    if(u8ClockEnableFlag == TRUE){

        u8TapTempoSetFlag = getTapTempoSetFlag();

         /*If the count has reached the trigger count, it's time for a pulse.*/
        //Or if we're in live play mode and tap tempo has been activated.
        if(getPlaybackRunStatus() == RUN || u8TapTempoSetFlag == TRUE){

                u8ExternalAirWheelActiveFlag = getu8ExternalAirWheelActiveFlag();

                /*If record is sync'ed to external then clock pulses are
                 * duplicated from the record input.
                 */
                if((getCurrentSource(RECORD) != EXTERNAL ||
                    getCurrentControl(RECORD) == GATE)
                    || u8TapTempoSetFlag == TRUE){
                    
                    if(u32ClockTimer++ >= u32ClockTimerTriggerCount){
                        SET_LOOP_SYNC_OUT;
                        u8ClockPulseFlag = TRUE;
                        setClockTriggerFlag();//Let master control know a clock edge occurred.
                        u32ClockTimer = 0;
                    }
                }else if(u8ExternalAirWheelActiveFlag == TRUE){
                    //If the clock is sped up, then we need to add clocks. The count for adding clocks
                    //is some even fraction of the clock trigger count.
                    //If the clock is slowed down, then we need to count extra clocks
                    if(u32ClockTimer++ < u32ClockTimerTriggerCount){
                        if(u32AirwheelClockTimer++ >= u32AirwheelClockTimerTriggerCount){
                            handleSwitchLEDClockBlink();
                            SET_LOOP_SYNC_OUT;
                            u8ClockPulseFlag = TRUE;
                            setClockTriggerFlag();//Let master control know a clock edge occurred.
                            u32AirwheelClockTimer = 0;
                        }
                    }else{
                        if(u32ClockTimerTriggerCount <= u32AirwheelClockTimerTriggerCount){
                            SET_LOOP_SYNC_OUT;
                            u8ClockPulseFlag = TRUE;
                            setClockTriggerFlag();//Let master control know a clock edge occurred.
                            u32AirwheelClockTimer = 0;
                        }
                        u32ClockTimer = 0;
                    }
                }
            u32RecClockCount++;
        }
    } 
}

void
vTIM5InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    CLEAR_SAMPLE_TIMER_INT;
    SET_DAC_LDAC;

    if(u8ClockPulseFlag == TRUE){
        u8ClockPulseFlag = FALSE;
        CLEAR_LOOP_SYNC_OUT;
    }

    if(u8GatePulseFlag == TRUE){
        CLEAR_GATE_OUT_PORT;
        u8GatePulseFlag = FALSE;
    }

    xSemaphoreGiveFromISR(xSemaphoreSampleTimer, &xHigherPriorityTaskWoken);

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*
 * I2C1 Interrupt Handler
 * This interrupt handler handles the interrupt from the MGC3130.
 * It moves along the I2C process.
 */
void
vI2C1InterruptHandler(void){
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	uint8_t		u8data;
	uint32_t	u32statusRegister;

    //See if the I2C Master interrupt was the source.
    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_I2C_1_MASTER)){
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_I2C_1_MASTER);
        xSemaphoreGiveFromISR(xSemaphoreI2CHandler,&xHigherPriorityTaskWoken);
    }
    else if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_I2C_1_BUS)){
        /*Check for errors. If an error occurred, start the state over again.*/
        if(PLIB_I2C_ArbitrationLossHasOccurred(I2C_MGC3130)){
            PLIB_I2C_ArbitrationLossClear(I2C_MGC3130);
        }
    }

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*This interrupt handler handles the data ready line from the MGC3130*/
void vEXTI0InterruptHandler(void){

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreMGC3130DataReady,&xHigherPriorityTaskWoken);
    //Clear the interrupt
    CLEAR_MGC3130_DATA_READY_INT;
    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

/*This interrupt handler handles the pressing of the encoder switch*/
void vEXTI3InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemaphoreEncSwitchPressed,&xHigherPriorityTaskWoken);
    CLEAR_ROTARY_ENC_SWITCH_INT;
    DISABLE_ROTARY_ENC_SWITCH_INT;
    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}


void vEXTI4InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(xSemaphoreSwitchPressed,&xHigherPriorityTaskWoken);
    CLEAR_SWITCH_INTERRUPT;
    DISABLE_SWITCH_INTERRUPT;
    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vSPI1InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_SPI_1_TRANSMIT)){
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_SPI_1_TRANSMIT);
        DISABLE_SPI1_TX_INT;
        xSemaphoreGiveFromISR(xSemaphoreSPITxSemaphore,&xHigherPriorityTaskWoken);
    }
    else if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_SPI_1_RECEIVE)){
       // DISABLE_SPI1_RX_INT;
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_SPI_1_RECEIVE);
        xSemaphoreGiveFromISR(xSemaphoreSPITxSemaphore,&xHigherPriorityTaskWoken);
    }

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vSPI2InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if(PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_SPI_2_TRANSMIT) &&
       PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_SPI_2_TRANSMIT)){
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_SPI_2_TRANSMIT);
        DISABLE_SPI2_TX_INT;
        SET_RAM_SPI_EN;
        xSemaphoreGiveFromISR(xSemaphoreSPIMemTxSemaphore,&xHigherPriorityTaskWoken);
    }
    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_SPI_2_RECEIVE) &&
            PLIB_INT_SourceIsEnabled(INT_ID_0, INT_SOURCE_SPI_2_RECEIVE)){
        DISABLE_SPI2_RX_INT;
        PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_SPI_2_RECEIVE);
        xSemaphoreGiveFromISR(xSemaphoreSPIMemRxSemaphore,&xHigherPriorityTaskWoken);
    }

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vDMA0InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    CLEAR_SPI_MEM_DMA_TX_INT;
    PLIB_DMA_ChannelXINTSourceFlagClear(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);

    xSemaphoreGiveFromISR(xSemaphoreDMA_0_TX, &xHigherPriorityTaskWoken);

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vDMA1InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    CLEAR_SPI_MEM_DMA_RX_INT;
    PLIB_DMA_ChannelXINTSourceFlagClear(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);

    xSemaphoreGiveFromISR(xSemaphoreDMA_0_RX, &xHigherPriorityTaskWoken);

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vDMA2InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    CLEAR_SPI_DACADC_DMA_TX_INT;
    PLIB_DMA_ChannelXINTSourceFlagClear(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);

    xSemaphoreGiveFromISR(xSemaphoreDMA_2_TX, &xHigherPriorityTaskWoken);

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vDMA3InterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    CLEAR_SPI_DACADC_DMA_RX_INT;
    PLIB_DMA_ChannelXINTSourceFlagClear(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);

    xSemaphoreGiveFromISR(xSemaphoreDMA_3_RX, &xHigherPriorityTaskWoken);

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

uint16_t u16PortDState,
        u16PortDLastState;
uint8_t u8PortEState,
        u8PortFState,
        u8PortELastState,
        u8PortFLastState;
static  uint8_t u8PlayInState,
            u8RecordInState,
            u8SyncInState,
            u8HoldState,
            u8ModJackDetectState;
io_event_message event_message;

void vPinChangeInterruptHandler(void){
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_CHANGE_NOTICE_E)){
        u8PortEState = PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_E);

        //Figure out which pin caused the change and handle it
        if(((u8PortELastState & (1<<ROTARY_ENC_A_PIN)) != (u8PortEState & (1<<ROTARY_ENC_A_PIN))) ||
          ((u8PortELastState & (1<<ROTARY_ENC_B_PIN)) != (u8PortEState & (1<<ROTARY_ENC_B_PIN)))){
            encoderHandler(u8PortEState);
        }

        if((u8PortELastState & (1<<RECORD_IN_PIN)) != (u8PortEState & (1<<RECORD_IN_PIN))){
            //Record in starts and stops recording
            u8RecordInState = (u8PortEState & (1<<RECORD_IN_PIN))>>RECORD_IN_PIN;
            event_message.u16messageType = RECORD_IN_EVENT;
            event_message.u16message = u8RecordInState;
            xQueueSendFromISR(xIOEventQueue, &event_message, 0);
            if((u8PortEState & 1<<RECORD_IN_PIN) == 0){
                u32LastRecClockCount = u32RecClockCount;
                u32RecClockCount = 0;
            }
        }

        if((u8PortELastState & (1<<SYNC_IN_PIN)) != (u8PortEState & (1<<SYNC_IN_PIN))){
            //Direction pin reverses playback direction
            u8SyncInState = (u8PortEState & (1<<SYNC_IN_PIN))>>SYNC_IN_PIN;
            if(u8SyncInState == 0){
                event_message.u16messageType = SYNC_IN_EVENT;
                event_message.u16message = 1;
                xQueueSendFromISR(xIOEventQueue, &event_message, 0);
            }
        }

        u8PortELastState = u8PortEState;
        CLEAR_PORT_E_CHANGE_INTERRUPT;
    }
    
    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_CHANGE_NOTICE_F)){
        u8PortFState = PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_F);

        u8PlayInState = (u8PortFState & (1<<PLAY_IN_PIN))>>PLAY_IN_PIN;
        event_message.u16messageType = PLAYBACK_IN_EVENT;
        event_message.u16message = u8PlayInState;
        xQueueSendFromISR(xIOEventQueue, &event_message, 0);

        u8PortFLastState = u8PortFState;
        CLEAR_PORT_F_CHANGE_INTERRUPT;
    }

    if(PLIB_INT_SourceFlagGet(INT_ID_0, INT_SOURCE_CHANGE_NOTICE_D)){
        u16PortDState = PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_D);

        if((u16PortDLastState & (1<<HOLD_PIN)) != (u16PortDState & (1<<HOLD_PIN))){
            //Hold input freezes the input when high, normal running when low but the logic
            //here is inverted.
            u8HoldState = (u16PortDState & (1<<HOLD_PIN))>>HOLD_PIN;
            event_message.u16messageType = HOLD_IN_EVENT;
            event_message.u16message = u8HoldState;
            xQueueSendFromISR(xIOEventQueue, &event_message, 0);
        }
        else if((u16PortDLastState & (1<<JACK_DETECT_PIN)) != (u16PortDState & (1<<JACK_DETECT_PIN))){
            u8ModJackDetectState = (u16PortDState & (1<<JACK_DETECT_PIN))>>JACK_DETECT_PIN;
            event_message.u16messageType = JACK_DETECT_IN_EVENT;
            event_message.u16message = u8ModJackDetectState;
            xQueueSendFromISR(xIOEventQueue, &event_message, 0);
        }
        u16PortDLastState = u16PortDState;
        DISABLE_PORT_D_CHANGE_INTERRUPT;
    }

    //Switch to higher priority task if necessary. This line must be last.
    portEND_SWITCHING_ISR( xHigherPriorityTaskWoken );
}

void vTaskIOHandler(void* pvParameters){
    portChangeStruct portChangeData;
    uint8_t u8Flag;
    uint16_t u16PortDState,
            u16PortDLastState;
    uint8_t u8PortEState,
            u8PortFState,
            u8PortELastState,
            u8PortFLastState;
    static  uint8_t u8PlayInState,
                u8RecordInState,
                u8SyncInState,
                u8HoldState,
                u8ModJackDetectState;
    io_event_message event_message;

    encoderInit();

    event_message.u16message = RECORD_IN_EVENT;
    event_message.u16messageType = NO_TRIGGER;

    ENABLE_ROTARY_ENC_SWITCH_INT;

    u8PortELastState = PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_E);
    u8PortFLastState = PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_F);

    for(;;){
        if(xQueueReceive(xIOPinChangeQueue, &portChangeData, portMAX_DELAY)){

            //Which port changed?
            if(portChangeData.u16Port == PORT_CHANNEL_E){



            }
            else if(portChangeData.u16Port == PORT_CHANNEL_F){
                u8PortFState = portChangeData.u16PortState;

                u8PlayInState = (u8PortFState & (1<<PLAY_IN_PIN))>>PLAY_IN_PIN;
                event_message.u16messageType = PLAYBACK_IN_EVENT;
                event_message.u16message = u8PlayInState;
                xQueueSend(xIOEventQueue, &event_message, 0);

                u8PortFLastState = u8PortFState;
            }
            else{
                u16PortDState = portChangeData.u16PortState;

                if((u16PortDLastState & (1<<HOLD_PIN)) != (u16PortDState & (1<<HOLD_PIN))){
                    //Hold input freezes the input when high, normal running when low but the logic
                    //here is inverted.
                    u8HoldState = (u16PortDState & (1<<HOLD_PIN))>>HOLD_PIN;
                    event_message.u16messageType = HOLD_IN_EVENT;
                    event_message.u16message = u8HoldState;
                    xQueueSend(xIOEventQueue, &event_message, 0);
                }
                else if((u16PortDLastState & (1<<JACK_DETECT_PIN)) != (u16PortDState & (1<<JACK_DETECT_PIN))){
                    u8ModJackDetectState = (u16PortDState & (1<<JACK_DETECT_PIN))>>JACK_DETECT_PIN;
                    event_message.u16messageType = JACK_DETECT_IN_EVENT;
                    event_message.u16message = u8ModJackDetectState;
                    xQueueSend(xIOEventQueue, &event_message, 0);
                }
                u16PortDLastState = u16PortDState;
            }
        }


    }
}

/*******************************************************************************
 End of File
 */

