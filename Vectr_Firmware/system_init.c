/*******************************************************************************
 System Initialization File

  File Name:
    system_init.c

  Summary:
    System Initialization.

  Description:
    This file contains source code necessary to initialize the system.
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

#include "app.h"
#include "peripheral/int/plib_int.h"
#include "FreeRTOS.h"
#include "system_config.h"
#include "mem.h"



// ****************************************************************************
// ****************************************************************************
// Section: Configuration Bits
// ****************************************************************************
// ****************************************************************************

/*
 System PLL Output Clock Divider (FPLLODIV)     = Divide by 1
 PLL Multiplier (FPLLMUL)                       = Multiply by 20
 PLL Input Divider (FPLLIDIV)                   = Divide by 2
 Watchdog Timer Enable (FWDTEN)                 = Disabled
 Clock Switching and Monitor Selection (FCKSM)  = Clock Switch Enable,
                                                  Fail Safe Clock Monitoring Enable
 Peripheral Clock Divisor (FPBDIV)              = Divide by 2
 */




// Enable the below section if you are using PIC32MX devices and disable the configuration bit settings for PIC32MZ 

#pragma config FSRSSEL = PRIORITY_7     // Shadow Register Set Priority Select (SRS Priority 7)
#pragma config PMDL1WAY = OFF            // Peripheral Module Disable Configuration (Allow only one reconfiguration)
#pragma config IOL1WAY = OFF             // Peripheral Pin Select Configuration (Allow only one reconfiguration)
#pragma config FUSBIDIO = OFF           // USB USID Selection (Controlled by Port Function)
#pragma config FVBUSONIO = OFF          // USB VBUS ON Selection (Controlled by Port Function)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider (2x Divider)
#pragma config UPLLEN = OFF              // USB PLL Enable (Enabled)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)

// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = HS             // Primary Oscillator Configuration (HS osc mode)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_2           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/1)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Enable, FSCM Enabled)
#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config WINDIS = OFF             // Watchdog Timer Window Enable (Watchdog Timer is in Non-Window Mode)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
#pragma config FWDTWINSZ = WISZ_25      // Watchdog Timer Window Size (Window Size is 25%)

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is Disabled)
#pragma config JTAGEN = OFF             // JTAG Enable (JTAG Disabled)
#pragma config ICESEL = ICS_PGx1        // ICE/ICD Comm Channel Select (Communicate on PGEC1/PGED1)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)


// *****************************************************************************
// *****************************************************************************
// Section: Driver Initialization Data
// *****************************************************************************
// *****************************************************************************



// ****************************************************************************
// ****************************************************************************
// Section: System Initialization
// ****************************************************************************
// ****************************************************************************

  
/*******************************************************************************
  Function:
    void SYS_Initialize ( SYS_INIT_DATA *data )

  Summary:
    Initializes the board, services, drivers, application and other modules

  Description:
    This routine initializes the board, services, drivers, application and other
    modules as configured at build time.  In a bare-metal environment (where no
    OS is supported), this routine should be called almost immediately after
    entering the "main" routine.

  Precondition:
    The C-language run-time environment and stack must have been initialized.

  Parameters:
    data        - Pointer to the system initialzation data structure containing
                  pointers to the board, system service, and driver
                  initialization routines
  Returns:
    None.

  Example:
    <code>
    SYS_INT_Initialize(NULL);
    </code>

  Remarks:
    Basic System Initialization Sequence:

    1.  Initilize minimal board services and processor-specific items
        (enough to use the board to initialize drivers and services)
    2.  Initialize all supported system services
    3.  Initialize all supported modules
        (libraries, drivers, middleware, and application-level modules)
    4.  Initialize the main (static) application, if present.

    The order in which services and modules are initialized and started may be
    important.

    For a static system (a system not using the Harmony's dynamic implementation
    of the initialization and "Tasks" services) this routine is implemented
    for the specific configuration of an application.
 */


void SYS_Initialize ( void* data )
{

    SYSTEMConfigPerformance(SYS_CLOCK);

    /* Initialize the BSP */
    BSP_Initialize ( );

    PLIB_INT_MultiVectorSelect(INT_ID_0);

    /*Initialize Peripherals*/
    initI2C_MGC3130(I2C_MGC3130_BAUD_RATE, PERIPHERAL_CLOCK);
    
    spi_dac_dma_init(SPI_DAC_BAUD_RATE, PERIPHERAL_CLOCK);

    spi_mem_init(SPI_MEM_BAUD_RATE, PERIPHERAL_CLOCK);

    /* Initialize the Application */
    APP_Initialize ( );

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_T3, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_T3, INT_SUBPRIORITY_LEVEL3);

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_T5, configMAX_SYSCALL_INTERRUPT_PRIORITY-1);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_T5, INT_SUBPRIORITY_LEVEL3);

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_I2C1_MASTER, configMAX_SYSCALL_INTERRUPT_PRIORITY-1);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_I2C1_MASTER, INT_SUBPRIORITY_LEVEL2);
    
    /*MGC3130 External Interrupt configuration - External Interrupt 0*/
    PLIB_INT_ExternalFallingEdgeSelect(INT_ID_0, INT_EXTERNAL_INT_SOURCE0);
    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_INT0, configMAX_SYSCALL_INTERRUPT_PRIORITY-1);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_INT0, INT_SUBPRIORITY_LEVEL3);

    PLIB_INT_ExternalFallingEdgeSelect(INT_ID_0, INT_EXTERNAL_INT_SOURCE3);
    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_INT3, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_INT3, INT_SUBPRIORITY_LEVEL3);

    PLIB_INT_ExternalRisingEdgeSelect(INT_ID_0, INT_EXTERNAL_INT_SOURCE4);
    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_INT4, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_INT4, INT_SUBPRIORITY_LEVEL3);

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_CHANGE_NOTICE_E, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_CHANGE_NOTICE_E, INT_SUBPRIORITY_LEVEL2);
    PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_E);
    ENABLE_PORT_E_CHANGE_INTERRUPT;
    PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_F);
    ENABLE_PORT_F_CHANGE_INTERRUPT;
    PLIB_PORTS_Read(PORTS_ID_0, PORT_CHANNEL_D);
    ENABLE_PORT_D_CHANGE_INTERRUPT;

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_DMA0, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_DMA0, INT_SUBPRIORITY_LEVEL1);

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_DMA1, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_DMA1, INT_SUBPRIORITY_LEVEL1);

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_DMA2, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_DMA2, INT_SUBPRIORITY_LEVEL1);

    PLIB_INT_VectorPrioritySet(INT_ID_0, INT_VECTOR_DMA3, configMAX_SYSCALL_INTERRUPT_PRIORITY-2);
    PLIB_INT_VectorSubPrioritySet(INT_ID_0, INT_VECTOR_DMA3, INT_SUBPRIORITY_LEVEL1);
}
/*******************************************************************************/
/*******************************************************************************
 End of File
*/
