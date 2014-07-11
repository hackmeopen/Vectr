/* 
 * File:   dac.h
 * Author: matthewheins
 *
 * Created on February 10, 2014, 8:40 PM
 */

#ifndef DAC_H
#define	DAC_H

#define LOAD_DAC_A_MASK 0x00
#define LOAD_DAC_B_MASK 0x02
#define LOAD_DAC_C_MASK 0x04
#define LOAD_DAC_C_AND_SHIFT_OUT_MASK 0x24

enum {
    SEND_DAC_A_DATA = 0,
    SEND_DAC_B_DATA,
    SEND_DAC_C_DATA,
    FINISH_TRANSMISSION,
    RECEIVE_ADC_DATA,
    FINISH_RECEPTION
};

#define MAX_ADC_VALUE   4096
#define HALF_ADC_VALUE  2048

void spi_dac_init(int baudRate, int clockFrequency);
void dacStateMachine(void);
void spi_dac_dma_init(int baudRate, int clockFrequency);
void dacDMA(void);


#endif	/* DAC_H */

