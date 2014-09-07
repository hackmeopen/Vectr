/*This file operates the quad digital to analog converter.*/
/*To get this thing up and running..
 Send data to the DAC.
 Configure a timer to trigger the set pin.*/

#include "app.h"
#include "dac.h"
#include "peripheral/peripheral.h"
#include "system_config.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

static uint8_t u8SPIState = SEND_DAC_A_DATA;
static uint16_t u16ADCData = 0;
static uint16_t u16FIFOCount = 0;
static uint16_t u16ADCData1, u16ADCData2, u16ADCData3;
#define LENGTH_OF_DACADC_MESSAGE    3
static uint8_t u8SPI_DACADC_Buffer[LENGTH_OF_DACADC_MESSAGE] = {0,0,0};

void runDACADC_DMA(void);



void spi_dac_init(int baudRate, int clockFrequency){

    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_SPI_1_TRANSMIT);
    
    /*Set when the data changes on the SPI bus*/
    /*Verify this is correct.*/
    PLIB_SPI_OutputDataPhaseSelect(SPI_DAC, SPI_OUTPUT_DATA_PHASE_ON_IDLE_TO_ACTIVE_CLOCK);

    PLIB_SPI_BufferClear(SPI_DAC);

    PLIB_SPI_BaudRateSet(SPI_DAC, clockFrequency, baudRate);

    PLIB_SPI_CommunicationWidthSelect(SPI_DAC, SPI_COMMUNICATION_WIDTH_8BITS);

    PLIB_SPI_FIFOEnable(SPI_DAC);

    PLIB_SPI_MasterEnable(SPI_DAC);

    PLIB_SPI_Enable(SPI_DAC);
}

void spi_dac_dma_init(int baudRate, int clockFrequency){
    PLIB_INT_SourceFlagClear(INT_ID_0, INT_SOURCE_SPI_1_TRANSMIT);

    /*Set when the data changes on the SPI bus*/
    /*Verify this is correct.*/
    PLIB_SPI_OutputDataPhaseSelect(SPI_DAC, SPI_OUTPUT_DATA_PHASE_ON_IDLE_TO_ACTIVE_CLOCK);

    PLIB_SPI_BufferClear(SPI_DAC);

    PLIB_SPI_BaudRateSet(SPI_DAC, clockFrequency, baudRate);

    PLIB_SPI_CommunicationWidthSelect(SPI_DAC, SPI_COMMUNICATION_WIDTH_8BITS);

    PLIB_SPI_MasterEnable(SPI_DAC);

    PLIB_SPI_Enable(SPI_DAC);

    //Configure the DMA for standard memory reads and writes
    //TX DMA
    PLIB_DMA_Enable(DMA_ID_0);
    PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_DACADC_TX_DMA_CHANNEL);
    PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, (uint32_t) u8SPI_DACADC_Buffer);
    PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, (volatile unsigned int) (SPI_DACADC_TX_DESTINATION_ADDRESS));
    PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, LENGTH_OF_DACADC_MESSAGE);
    PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXCellSizeSet(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXTriggerEnable(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, DMA_CHANNEL_TRIGGER_TRANSFER_START);
    PLIB_DMA_ChannelXStartIRQSet(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, DMA_TRIGGER_SPI_1_TRANSMIT);
    PLIB_DMA_ChannelXINTSourceEnable(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);

    //RX DMA
    //Trigger the DMA when data is received
    PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_DACADC_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, (uint32_t) u8SPI_DACADC_Buffer);
    PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, (volatile unsigned int) (SPI_DACADC_RX_SOURCE_ADDRESS));
    PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, LENGTH_OF_DACADC_MESSAGE);
    PLIB_DMA_ChannelXCellSizeSet(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXTriggerEnable(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, DMA_CHANNEL_TRIGGER_TRANSFER_START);
    PLIB_DMA_ChannelXStartIRQSet(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, DMA_TRIGGER_SPI_1_RECEIVE);
    PLIB_DMA_ChannelXINTSourceEnable(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);

}


void dacStateMachine(void){
    static pos_and_gesture_data pos_and_gesture_struct;

    switch(u8SPIState){
        case SEND_DAC_A_DATA:

            xQueueReceive(xSPIDACQueue, &pos_and_gesture_struct, portMAX_DELAY);

            CLEAR_DAC_LDAC;
            /*Load the Data for DAC A*/
            CLEAR_DAC_SPI_EN;
            PLIB_SPI_BufferWrite(SPI_DAC, LOAD_DAC_A_MASK);
            PLIB_SPI_BufferWrite(SPI_DAC, pos_and_gesture_struct.u16XPosition>>8);
            PLIB_SPI_BufferWrite(SPI_DAC, pos_and_gesture_struct.u16XPosition);
            CLEAR_SPI1_TX_INT;
            ENABLE_SPI1_TX_INT;
            u8SPIState = SEND_DAC_B_DATA;
            break;

        case SEND_DAC_B_DATA:
            xSemaphoreTake(xSemaphoreSPITxSemaphore, portMAX_DELAY);
            SET_DAC_SPI_EN;
            /*Read the buffer to clear the receive buffer*/
            PLIB_SPI_BufferRead(SPI_DAC);
            PLIB_SPI_BufferRead(SPI_DAC);
            PLIB_SPI_BufferRead(SPI_DAC);

            /*Write the B DAC*/
            CLEAR_DAC_SPI_EN;
            PLIB_SPI_BufferWrite(SPI_DAC, LOAD_DAC_B_MASK);
            PLIB_SPI_BufferWrite(SPI_DAC, pos_and_gesture_struct.u16YPosition>>8);
            PLIB_SPI_BufferWrite(SPI_DAC, pos_and_gesture_struct.u16YPosition);
            CLEAR_SPI1_TX_INT;
            ENABLE_SPI1_TX_INT;

            u8SPIState = SEND_DAC_C_DATA;
            break;

        case SEND_DAC_C_DATA:
            xSemaphoreTake(xSemaphoreSPITxSemaphore, portMAX_DELAY);
            SET_DAC_SPI_EN;
            CLEAR_ADC_SPI_EN;
            u16FIFOCount = PLIB_SPI_FIFOCountGet(SPI_DAC, SPI_FIFO_TYPE_RECEIVE);

            while(u16FIFOCount > 0){
                PLIB_SPI_BufferRead(SPI_DAC);
                u16FIFOCount--;
            }

            /*Write the C DAC*/
            CLEAR_DAC_SPI_EN;

            PLIB_SPI_BufferWrite(SPI_DAC, LOAD_DAC_C_MASK);
            PLIB_SPI_BufferWrite(SPI_DAC, pos_and_gesture_struct.u16ZPosition>>8);
            PLIB_SPI_BufferWrite(SPI_DAC, pos_and_gesture_struct.u16ZPosition);
            CLEAR_SPI1_TX_INT;
            ENABLE_SPI1_TX_INT;
          //  ENABLE_SPI1_RX_INT;

            u8SPIState = FINISH_TRANSMISSION;
            break;

        case FINISH_TRANSMISSION:
            xSemaphoreTake(xSemaphoreSPITxSemaphore, portMAX_DELAY);
            
            SET_DAC_SPI_EN;
            SET_ADC_SPI_EN;
        //    xSemaphoreTake(xSemaphoreSPITxSemaphore, portMAX_DELAY);
            /*Read the buffer to clear the receive buffer -
             * Data comes out the ADC with the top 5 bits in the first byte
             * and the bottom 7 bits in the second byte.
             */

            vTaskDelay(2);

            u16ADCData1 = PLIB_SPI_BufferRead(SPI_DAC);
            u16ADCData2 = PLIB_SPI_BufferRead(SPI_DAC);
            u16ADCData3 = PLIB_SPI_BufferRead(SPI_DAC);
            u16ADCData = u16ADCData1 & 0x1F;
            u16ADCData <<= 7;//First 8 bits are
            u16ADCData += (u16ADCData2>>1);
            PLIB_SPI_BufferRead(SPI_DAC);
            u16ADCData &= 0x0FFF;

           // xQueueSend(xADCQueue, &u16ADCData, 0);

            u8SPIState = SEND_DAC_A_DATA;
            break;
        }
}

void dacDMA(void){
    pos_and_gesture_data pos_and_gesture_struct;
    uint16_t u16ADCData;
    static uint8_t u8blah = 0;
    
    xQueueReceive(xSPIDACQueue, &pos_and_gesture_struct, portMAX_DELAY);

    CLEAR_DAC_LDAC;
    /*Load the Data for DAC A*/
    CLEAR_DAC_SPI_EN;
    u8SPI_DACADC_Buffer[0] = LOAD_DAC_A_MASK;
    u8SPI_DACADC_Buffer[1] =  pos_and_gesture_struct.u16XPosition>>8;
    u8SPI_DACADC_Buffer[2] = pos_and_gesture_struct.u16XPosition;
    
    runDACADC_DMA();
    
    xSemaphoreTake(xSemaphoreDMA_3_RX, portMAX_DELAY);
    
    SET_DAC_SPI_EN;
    /*Write the B DAC*/
    u8SPI_DACADC_Buffer[0] = LOAD_DAC_B_MASK;
    u8SPI_DACADC_Buffer[1] =  pos_and_gesture_struct.u16YPosition>>8;
    u8SPI_DACADC_Buffer[2] = pos_and_gesture_struct.u16YPosition;

    CLEAR_DAC_SPI_EN;

    runDACADC_DMA();

    xSemaphoreTake(xSemaphoreDMA_3_RX, portMAX_DELAY);

    SET_DAC_SPI_EN;

    /*Write the C DAC*/
    u8SPI_DACADC_Buffer[0] = LOAD_DAC_C_MASK;
    u8SPI_DACADC_Buffer[1] =  pos_and_gesture_struct.u16ZPosition>>8;
    u8SPI_DACADC_Buffer[2] = pos_and_gesture_struct.u16ZPosition;

    CLEAR_DAC_SPI_EN;
    CLEAR_ADC_SPI_EN;

    runDACADC_DMA();

    xSemaphoreTake(xSemaphoreDMA_3_RX, portMAX_DELAY);

    SET_DAC_SPI_EN;
    SET_ADC_SPI_EN;

    u16ADCData = u8SPI_DACADC_Buffer[0] & 0x1F;
    u16ADCData <<= 7;//First 8 bits are
    u16ADCData += (u8SPI_DACADC_Buffer[1]>>1);
    xQueueSend(xADCQueue, &u16ADCData, 0);

    
}

void runDACADC_DMA(void){
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL);

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_DACADC_RX_DMA_CHANNEL)){
        SPI_DACADC_DMA_RX_START;
    }
    
    //Trigger the DMA
    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_DACADC_TX_DMA_CHANNEL)){
        SPI_DACADC_DMA_TX_START;
    }
}