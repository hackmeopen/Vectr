/*******************************************************************************
  MPLAB Harmony Application

  Application Header

  File Name:
    app.h

  Summary:
	Application definitions. 

  Description:
	 This file contains the  application definitions.
*******************************************************************************/

//DOM-IGNORE-BEGIN
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
//DOM-IGNORE-END

#ifndef _APP_HEADER_H
#define _APP_HEADER_H

#define PLIB_DISABLE_OPTIMIZATION_WARNING 1

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

void APP_Initialize ( void );
void APP_Tasks ( void );
void SYS_Initialize ( void* data );
void SYS_Tasks ( void );
void setClockTimerTriggerCount(uint32_t u32NewTriggerCount);
void resetClockTimer(void);
void setClockEnableFlag(uint8_t u8NewState);

#define FALSE   0
#define TRUE    1

typedef struct{
	uint16_t u16XPosition;
	uint16_t u16YPosition;
	uint16_t u16ZPosition;
	uint16_t u16Gesture;
        uint16_t u16Touch;
        uint16_t u16Airwheel;
}pos_and_gesture_data;

typedef struct{
    pos_and_gesture_data sample_1;
    pos_and_gesture_data sample_2;
}memory_data_packet;

/*Priorities*/
#define TASK_MASTER_CONTROL_PRIORITY    4
#define TASK_SWITCH_PRIORITY            4
#define TASK_I2C_MGC3130_PRIORITY       4
#define TASK_SPI_DAC_PRIORITY           3
#define TASK_LEDS_PRIORITY              3
#define TASK_SPI_MEM_PRIORITY           3
#define TASK_IO_HANDLER_PRIORITY        3

/*Interrupt Priorities*/


extern xSemaphoreHandle xSemaphoreMGC3130DataReady;
extern xSemaphoreHandle xSemaphoreI2CHandler;
extern xSemaphoreHandle xSemaphoreSPITxSemaphore;
extern xSemaphoreHandle xSemaphoreSPIMemTxSemaphore;
extern xSemaphoreHandle xSemaphoreSPIMemRxSemaphore;
extern xSemaphoreHandle xSemaphoreSampleTimer;
extern xSemaphoreHandle xSemaphoreDMA_0_RX;
extern xSemaphoreHandle xSemaphoreDMA_0_TX;
extern xSemaphoreHandle xSemaphoreDMA_3_RX;
extern xSemaphoreHandle xSemaphoreDMA_2_TX;

extern xQueueHandle xSPIDACQueue;
extern xQueueHandle xLEDQueue;
extern xQueueHandle xPositionQueue;
extern xQueueHandle xRAMReadQueue;
extern xQueueHandle xRAMWriteQueue;
extern xQueueHandle xMemInstructionQueue;//Tells the memory task what to do.
extern xQueueHandle xSwitchQueue;
extern xQueueHandle xIOEventQueue;
extern xQueueHandle xADCQueue;//Pass data from the ADC to the Modulation routine.

#endif /* _APP_HEADER_H */

/*******************************************************************************
 End of File
 */



