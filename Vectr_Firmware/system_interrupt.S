/*******************************************************************************
 System Initialization File

  File Name:
    system_interrupt.c

  Summary:
    System Interrupts handlers.

 Important :
 This demo uses an evaluation license which is meant for demonstration only and
 that customers desiring development and production on OPENRTOS must procure a
 suitable license.

  Description:
    This file contains a definitions of the raw ISRs required to support the
    interrupt sub-system.
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
Copyright (c) 2011-2012 released Microchip Technology Inc.  All rights reserved.

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

#include <xc.h>
#include <sys/asm.h>
#include "ISR_Support.h"

	.set	nomips16
 	.set 	noreorder

 	.extern vI2C1InterruptHandler
    .extern vEXTI0InterruptHandler
    .extern vEXTI3InterruptHandler
    .extern vEXTI4InterruptHandler
    .extern vSPI1InterruptHandler
    .extern vSPI2InterruptHandler
    .extern vSPI2InterruptHandler
    .extern vPinChangeInterruptHandler
    .extern vTIM3InterruptHandler
    .extern vTIM5InterruptHandler
    .extern vDMA0InterruptHandler
    .extern vDMA1InterruptHandler
    .extern vDMA2InterruptHandler
    .extern vDMA3InterruptHandler
	.extern xISRStackTop
 	.global	vI2C1InterruptWrapper
    .global vEXTI0InterruptWrapper
    .global vEXTI3InterruptWrapper
    .global vEXTI4InterruptWrapper
    .global vSPI1InterruptWrapper
    .global vSPI2InterruptWrapper
    .global vPinChangeInterruptWrapper
    .global vTIM3InterruptWrapper
    .global vTIM5InterruptWrapper
    .global vDMA0InterruptWrapper
    .global vDMA1InterruptWrapper
    .global vDMA2InterruptWrapper
    .global vDMA3InterruptWrapper

	.set	noreorder
	.set 	noat
	.ent	vI2C1InterruptWrapper

vI2C1InterruptWrapper:

	portSAVE_CONTEXT
	jal vI2C1InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vI2C1InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vEXTI0InterruptWrapper

vEXTI0InterruptWrapper:

    portSAVE_CONTEXT
	jal vEXTI0InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vEXTI0InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vEXTI3InterruptWrapper

vEXTI3InterruptWrapper:

    portSAVE_CONTEXT
	jal vEXTI3InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vEXTI3InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vEXTI4InterruptWrapper

vEXTI4InterruptWrapper:

    portSAVE_CONTEXT
	jal vEXTI4InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vEXTI4InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vSPI1InterruptWrapper

vSPI1InterruptWrapper:

    portSAVE_CONTEXT
	jal vSPI1InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vSPI1InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vSPI2InterruptWrapper

vSPI2InterruptWrapper:

    portSAVE_CONTEXT
	jal vSPI2InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vSPI2InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vPinChangeInterruptWrapper

vPinChangeInterruptWrapper:

    portSAVE_CONTEXT
	jal vPinChangeInterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vPinChangeInterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vTIM3InterruptWrapper

vTIM3InterruptWrapper:

    portSAVE_CONTEXT
	jal vTIM3InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vTIM3InterruptWrapper

    .set	noreorder
	.set 	noat
	.ent	vTIM5InterruptWrapper

vTIM5InterruptWrapper:

    portSAVE_CONTEXT
	jal vTIM5InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vTIM5InterruptWrapper

.set	noreorder
	.set 	noat
	.ent	vDMA0InterruptWrapper

vDMA0InterruptWrapper:

    portSAVE_CONTEXT
	jal vDMA0InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vDMA0InterruptWrapper

.set	noreorder
	.set 	noat
	.ent	vDMA1InterruptWrapper

vDMA1InterruptWrapper:

    portSAVE_CONTEXT
	jal vDMA1InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vDMA1InterruptWrapper

.set	noreorder
	.set 	noat
	.ent	vDMA2InterruptWrapper

vDMA2InterruptWrapper:

    portSAVE_CONTEXT
	jal vDMA2InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vDMA2InterruptWrapper

.set	noreorder
	.set 	noat
	.ent	vDMA3InterruptWrapper

vDMA3InterruptWrapper:

    portSAVE_CONTEXT
	jal vDMA3InterruptHandler
	nop
	portRESTORE_CONTEXT

	.end	vDMA3InterruptWrapper