/*******************************************************************************
  System Config Performance function implementation

  Company:
    Microchip Technology Inc.

  File Name:
    config_performance.c

  Summary:
    Contains a non-ISP implementation of legacy SYSTEMConfigPerformance 
    function

  Description:
    This file contains a non-ISP implementation of the legacy 
    SYSTEMConfigPerformance function. The file is self contained and does
    not require inclusion of legacy plib.h file.
*******************************************************************************/

//DOM-IGNORE-BEGIN
/******************************************************************************
Copyright (c) 2012 released Microchip Technology Inc.  All rights reserved.

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

//#if defined (__PIC32MX__)

#include <p32xxxx.h>

#define OSC_PB_DIV_2    (1 << _OSCCON_PBDIV_POSITION)
#define OSC_PB_DIV_1    (0 << _OSCCON_PBDIV_POSITION)

#define PB_BUS_MAX_FREQ_HZ      80000000
#define FLASH_SPEED_HZ          30000000 

#define CHE_CONF_PF_ALL      (3 << _CHECON_PREFEN_POSITION)

#ifdef _DMAC
	#define	mSYSTEMUnlock(intStat, dmaSusp)	do{intStat=INTDisableInterrupts(); dmaSusp=DmaSuspend(); \
						SYSKEY = 0, SYSKEY = 0xAA996655, SYSKEY = 0x556699AA;}while(0)
#else
	#define	mSYSTEMUnlock(intStat, dmaSusp)	do{intStat=INTDisableInterrupts(); \
						SYSKEY = 0, SYSKEY = 0xAA996655, SYSKEY = 0x556699AA;}while(0)
#endif	// _DMAC

#ifdef _DMAC
	#define mSYSTEMLock(intStat, dmaSusp)	do{SYSKEY = 0x33333333; DmaResume(dmaSusp); INTRestoreInterrupts(intStat);}while(0)
#else
	#define mSYSTEMLock(intStat, dmaSusp)	do{SYSKEY = 0x33333333; INTRestoreInterrupts(intStat);}while(0)
#endif // _DMAC

#define mBMXDisableDRMWaitState() 	(BMXCONCLR = _BMXCON_BMXWSDRM_MASK)

#define mCheGetCon() CHECON

#define mCheConfigure(val) (CHECON = (val))

void __attribute__ ((nomips16)) CheKseg0CacheOn()
{
	register unsigned long tmp;
	asm("mfc0 %0,$16,0" :  "=r"(tmp));
	tmp = (tmp & ~7) | 3;
	asm("mtc0 %0,$16,0" :: "r" (tmp));
}

int DmaSuspend(void)
{
    int suspSt;
    if(!(suspSt=DMACONbits.SUSPEND))
    {
        DMACONSET=_DMACON_SUSPEND_MASK;		// suspend
        while((DMACONbits.DMABUSY));	// wait to be actually suspended
    }
    return suspSt;
}

void DmaResume(int susp)
{
    if(susp)
    {
        DmaSuspend();
    }
    else
    {
        DMACONCLR=_DMACON_SUSPEND_MASK;		// resume DMA activity
    }
}



unsigned int INTDisableInterrupts(void)
{
    unsigned int status = 0;

    asm volatile("di    %0" : "=r"(status));

    return status;
}

void INTRestoreInterrupts(unsigned int status)
{
    if(status & 0x00000001)
        asm volatile("ei");
    else
        asm volatile("di");
}

void OSCSetPBDIV(unsigned int oscPbDiv)
{
	unsigned int dma_status;
	unsigned int int_status;
	__OSCCONbits_t oscBits;

	mSYSTEMUnlock(int_status, dma_status);
	
	oscBits.w=OSCCON;		// read to be in sync. flush any pending write
	oscBits.PBDIV=0;
	oscBits.w|=oscPbDiv;	
	OSCCON=oscBits.w;		// write back
	oscBits.w=OSCCON;		// make sure the write occurred before returning from this function
	
	mSYSTEMLock(int_status, dma_status);
}

unsigned int SYSTEMConfigPB(unsigned int sys_clock)
{
    unsigned int pb_div;
    unsigned int pb_clock;

    pb_clock = sys_clock;

    if(sys_clock > PB_BUS_MAX_FREQ_HZ)
    {
        pb_div=OSC_PB_DIV_2;
        pb_clock >>= 1;
    }
    else
    {
        pb_div=OSC_PB_DIV_1;
    }

    OSCSetPBDIV(pb_div);

    return pb_clock;
}


unsigned int SYSTEMConfigWaitStatesAndPB(unsigned int sys_clock)
{
#ifdef _PCACHE
    unsigned int wait_states;
#endif
    unsigned int pb_clock;
    unsigned int int_status;

    pb_clock = SYSTEMConfigPB(sys_clock);


    // set the flash wait states based on 1 wait state
    // for every 20 MHz
#ifdef _PCACHE
    wait_states = 0;

    while(sys_clock > FLASH_SPEED_HZ)
    {
        wait_states++;
        sys_clock -= FLASH_SPEED_HZ;
    }

    int_status=INTDisableInterrupts();
    mCheConfigure(wait_states);
    INTRestoreInterrupts(int_status);

#endif

    return pb_clock;
}

unsigned int SYSTEMConfigPerformance(unsigned int sys_clock)
{
    // set up the wait states
    unsigned int pb_clk;
#ifdef _PCACHE
    unsigned int cache_status;
#endif
    unsigned int int_status;

    pb_clk = SYSTEMConfigWaitStatesAndPB(sys_clock);

    int_status=INTDisableInterrupts();

    mBMXDisableDRMWaitState();

#ifdef _PCACHE
    cache_status = mCheGetCon();
    cache_status |= CHE_CONF_PF_ALL;
    mCheConfigure(cache_status);
    CheKseg0CacheOn();
#endif

    INTRestoreInterrupts(int_status);

    return pb_clk;

}
//#endif
