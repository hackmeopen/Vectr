#include "mem.h"
#include "system_config.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "app.h"
#include "master_control.h"

static uint8_t u8SequenceLocationFlag = STORED_IN_RAM;
static uint32_t u32SequencePlaybackPosition = 0; //Where are we in playback
static uint32_t u32FlashReadAddress;
static uint32_t u32RAMReadAddress;
static uint32_t u32SequenceStartAddress;
static uint32_t u32SequenceEndAddress;
static uint32_t u32EndofRAMRecordingAddress;
static uint32_t u32StartOfCurrentFlashSectorAddress, u32EndOfCurrentFlashSectorAddress;
static uint32_t u32RAMWriteAddress;
static uint8_t u8RAMRetriggerFlag;
static uint8_t u8FlashFileTableWriteFlag;//Does the file table need to be written?
static uint8_t u8FlashSettingsTableWriteFlag;//Does the settings table need to be written?
static uint8_t u8DMAState;//Which state is the DMA set for?RAM?Flash?File Table?
static uint8_t u8FileTableWriteFlag = FALSE;
static uint8_t u8FileTableWriteState;
static uint8_t u8FlashTimer = 0;
static uint8_t u8FlashWriteEnabledFlag = FALSE;
static uint32_t u32FirstSampleAddress;

static uint8_t u8SPI_MEM_TX_Buffer[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static uint8_t u8SPI_MEM_RX_Buffer[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

typedef struct{
    uint8_t u8SPIMessage[4];
    uint8_t u8LoadStoreBuffer[FLASH_SECTOR_SIZE];
}LoadStoreBufferStruct;

LoadStoreBufferStruct LoadStoreBuffer;

//Local copy of the file table and the settings table.
//They have to be written back when something changes.
static uint8_t u8FlashEnableMsg = 0x06;
static uint8_t u8FlashDummyRegister[8] = {0,0,0,0,0,0,0,0};//Place to store stuff when we don't care what we receive

static uint8_t u8FlashWriteDisableMsg = FLASH_CMD_WRITE_DISABLE;

#define LENGTH_OF_SECTOR_ERASE_MSG  4
static uint8_t u8FlashSectorEraseMsg[LENGTH_OF_SECTOR_ERASE_MSG] = {0x20,0,0,0};

#define LENGTH_OF_WRITE_START_MSG   5 //Writes a command, address, and the first byte.
static uint8_t u8WriteStartMsg[LENGTH_OF_WRITE_START_MSG] = {FLASH_CMD_AUTO_INCREMENT_WR, 0,0,2,0};

#define LENGTH_OF_READ_START_MSG   4
static uint8_t u8ReadStartMsg[LENGTH_OF_WRITE_START_MSG] = {FLASH_CMD_READ, 0,0,2};

#define LENGTH_OF_FLASH_WRITE_MSG    2
static uint8_t u8FlashWriteMsg[LENGTH_OF_FLASH_WRITE_MSG] = {FLASH_CMD_AUTO_INCREMENT_WR, 0};

static uint8_t u8EnableWriteStatusRegMsg = FLASH_CMD_EN_WRITE_STATUS_REG;

static uint8_t u8WriteStatusRegMsg[2] = {FLASH_CMD_WRITE_STATUS_REG, 0};


enum{
    ENABLE_SECTOR_ERASE = 0,
    ERASE_SECTOR,
    WAIT_FOR_ERASE_TO_COMPLETE,
    ENABLE_TABLE_WRITE,
    SEND_FIRST_COMMAND,
    SEND_BYTE,
    COMPLETE_WRITE
};

/*This table contains a copy of the file table from the Flash.
*It contains information about the contents of the flash and the settings
 * for each sequence.
*/
static file_table ftFileTable;

static void setRAMCurrentAddress(uint32_t u32RAMNextAvailableAddress);
static void clearSPIBuffer(void);
void flashLocateNextSector(uint8_t u8PlaybackDirection);

/*The memory SPI bus transmits 32 bits at a time*/

void spi_mem_init(int baudRate, int clockFrequency){

    PLIB_SPI_OutputDataPhaseSelect(SPI_MEM, SPI_OUTPUT_DATA_PHASE_ON_ACTIVE_TO_IDLE_CLOCK);

    PLIB_SPI_BufferClear(SPI_MEM);

    PLIB_SPI_BaudRateSet(SPI_MEM, clockFrequency, baudRate);

    PLIB_SPI_CommunicationWidthSelect(SPI_MEM, SPI_COMMUNICATION_WIDTH_8BITS);

    PLIB_SPI_MasterEnable(SPI_MEM);

    PLIB_SPI_Enable(SPI_MEM);

    //Configure the DMA for standard memory reads and writes
    //TX DMA
    PLIB_DMA_Enable(DMA_ID_0);
    PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
    PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) u8SPI_MEM_TX_Buffer);
    PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (volatile unsigned int) (SPI_MEM_TX_DESTINATION_ADDRESS));
    PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, LENGTH_OF_DMA_MESSAGE);
    PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXCellSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXTriggerEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, DMA_CHANNEL_TRIGGER_TRANSFER_START);
    PLIB_DMA_ChannelXStartIRQSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, DMA_TRIGGER_SPI_2_TRANSMIT);
    PLIB_DMA_ChannelXINTSourceEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);
    
    //RX DMA
    //Trigger the DMA when data is received
    PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8SPI_MEM_RX_Buffer);
    PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (volatile unsigned int) (SPI_MEM_RX_SOURCE_ADDRESS));
    PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, LENGTH_OF_DMA_MESSAGE);
    PLIB_DMA_ChannelXCellSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 1);
    PLIB_DMA_ChannelXTriggerEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, DMA_CHANNEL_TRIGGER_TRANSFER_START);
    PLIB_DMA_ChannelXStartIRQSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, DMA_TRIGGER_SPI_2_RECEIVE);
    PLIB_DMA_ChannelXINTSourceEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, DMA_INT_BLOCK_TRANSFER_COMPLETE);
}

uint8_t getDMAState(void){
    return u8DMAState;
}

void setDMAState(uint8_t u8NewDMAState){
    
    u8DMAState = u8NewDMAState;
    
    switch(u8DMAState){
        case DMA_RAM:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) u8SPI_MEM_TX_Buffer);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, LENGTH_OF_DMA_MESSAGE);
            
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8SPI_MEM_RX_Buffer);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, LENGTH_OF_DMA_MESSAGE);
            break;

        case DMA_RAM_FULL_SECTOR:
            
#ifdef RAM_SIZE_512K
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) (&LoadStoreBuffer.u8SPIMessage[1]));
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, sizeof(LoadStoreBuffer)-1);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) (&LoadStoreBuffer.u8SPIMessage[1]));
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, sizeof(LoadStoreBuffer) - 1);
#else
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) (&LoadStoreBuffer.u8SPIMessage[0]));
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, sizeof(LoadStoreBuffer));

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) (&LoadStoreBuffer.u8SPIMessage[0]));
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, sizeof(LoadStoreBuffer));
#endif
            break;
            
        case DMA_FLASH_WRITE_ENABLE:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8FlashEnableMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 1);
            
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 1);
            break;

        case DMA_FLASH_SECTOR_READ:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) (&LoadStoreBuffer.u8SPIMessage[0]));
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, sizeof(LoadStoreBuffer));

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) (&LoadStoreBuffer.u8SPIMessage[0]));
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, sizeof(LoadStoreBuffer));
            break;

        case DMA_FLASH_WRITE_START:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8WriteStartMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 5);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 5);
            break;

        case DMA_FLASH_READ:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) u8SPI_MEM_TX_Buffer);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, LENGTH_OF_FLASH_READ_DMA_MESSAGE);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8SPI_MEM_RX_Buffer);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, LENGTH_OF_FLASH_READ_DMA_MESSAGE);
            break;

        case DMA_FLASH_SECTOR_ERASE:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8FlashSectorEraseMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, LENGTH_OF_SECTOR_ERASE_MSG);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, LENGTH_OF_SECTOR_ERASE_MSG);
            break;
            
        case DMA_FLASH_FILE_TABLE_READ:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &ftFileTable);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, sizeof(file_table));

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) &ftFileTable);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, sizeof(file_table));
            break;

        case DMA_FLASH_WRITE:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8FlashWriteMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, LENGTH_OF_FLASH_WRITE_MSG);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) &u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, LENGTH_OF_FLASH_WRITE_MSG);
            break;

        case DMA_FLASH_FILE_TABLE_WRITE_START:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &ftFileTable);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 5);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) &u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 5);
            break;

        case DMA_FLASH_WRITE_FINISH:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8FlashWriteDisableMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 1);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 1);
            break;

        case DMA_FLASH_ENABLE_WRITE_STATUS_REGISTER:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8EnableWriteStatusRegMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 1);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 1);
            break;

        case DMA_FLASH_WRITE_STATUS_REGISTER:
            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_TX_DMA_CHANNEL);
            PLIB_DMA_ChannelXSourceStartAddressSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, (uint32_t) &u8WriteStatusRegMsg);
            PLIB_DMA_ChannelXSourceSizeSet(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL, 2);

            PLIB_DMA_ChannelXDisable(DMA_ID_0,SPI_MEM_RX_DMA_CHANNEL);
            PLIB_DMA_ChannelXDestinationStartAddressSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, (uint32_t) u8FlashDummyRegister);
            PLIB_DMA_ChannelXDestinationSizeSet(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL, 2);
            break;

        default:
            
            break;
    }
}


/*Store a sequence presently in RAM in Flash with the supplied index.
 Returns a failure/success flag.*/
uint8_t flashStoreSequence(uint8_t u8SequenceIndex, uint32_t u32SequenceLength){
    uint32_t u32RemainingSequenceBytes = u32SequenceLength;
    uint8_t u8SectorsWritten = 0;//How many sector chunks have we written.
    uint32_t u32BytesWritten = 0;
    uint8_t u8CurrentSector = 1;
    uint8_t * p_u8Data;
    uint8_t * p_u8EndOfData;
    uint8_t u8CurrentSequenceIndex = 0;//Which part of the sequence is it?
    uint8_t u8PlaybackRunStatus;
    uint8_t u8WriteSequenceStartAddressFlag = FALSE;
    uint8_t u8RemainderBytes;//Make sure the number of bytes stored falls on a multiple of chunk

    /* Procedure:
     * 1. Look to see if there is enough space to store the new sequence. If not, indicate an error.
     * 1. Check if there's already a sequence stored with the given sequence index.
     * 2. If there is, remove the references in the local file table.
     * 3. Find an open flash sector.
     * 5. Read one sector's worth of data from the RAM.
     * 6. Write it to the Flash.
     * 7. Repeat until the end of the sequence is reached.
     */

    if(u8FlashWriteEnabledFlag == FALSE){
        flashClearWriteProtection();
    }

    //Look to see if there's already a sequence stored under this index and erase it in the file table.
    removeSequenceFromFileTable(u8SequenceIndex);//Will increase space if it's in there.

    //Is there enough space?
    if(u32SequenceLength > ftFileTable.u32AvailableMemorySpace){
        return 0;//failure
    }

    //Stop playback if it's running.
    u8PlaybackRunStatus = getPlaybackRunStatus();

    setPlaybackRunStatus(STOP);

    /*For each sector, read out a sector from RAM. Then, erase the sector in Flash and
     write it over to Flash.*/

    while(u32RemainingSequenceBytes != 0){

        u32RAMReadAddress = u8SectorsWritten*FLASH_UTILIZED_SECTOR_SIZE;

        readRAMSector(&LoadStoreBuffer.u8SPIMessage[0], u32RAMReadAddress);

        //Find somewhere to put it.
        while(ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex != FLASH_SECTOR_UNASSIGNED){
            u8CurrentSector++;
        }

        if(u8WriteSequenceStartAddressFlag == FALSE){
          //  u32SequenceStartAddress = u8CurrentSector*FLASH_SECTOR_SIZE;
            ftFileTable.u32SequenceStartAddress[u8SequenceIndex] = u8CurrentSector*FLASH_SECTOR_SIZE;;
            u8WriteSequenceStartAddressFlag = TRUE;
        }


        //Check to see if this is the last sector.
        //Determine the end of the write in Flash
        if(u32RemainingSequenceBytes > FLASH_UTILIZED_SECTOR_SIZE){
            p_u8EndOfData = &LoadStoreBuffer.u8LoadStoreBuffer[0] + FLASH_UTILIZED_SECTOR_SIZE;
            u32BytesWritten += FLASH_UTILIZED_SECTOR_SIZE;
            u32RemainingSequenceBytes -= FLASH_UTILIZED_SECTOR_SIZE;
            ftFileTable.flash_file_table[u8CurrentSector].u32FileLength = FLASH_UTILIZED_SECTOR_SIZE;
        }
        else{
            u8RemainderBytes = u32RemainingSequenceBytes%NUMBER_OF_DATA_BYTES;
            u32RemainingSequenceBytes -= u8RemainderBytes;//Round the number of bytes down to a multiple of the packet size.
            p_u8EndOfData = &LoadStoreBuffer.u8LoadStoreBuffer[0] + u32RemainingSequenceBytes;
            ftFileTable.u32SequenceEndAddress[u8SequenceIndex] = u8CurrentSector*FLASH_SECTOR_SIZE +  u32RemainingSequenceBytes;
            u32BytesWritten += u32RemainingSequenceBytes;
            ftFileTable.flash_file_table[u8CurrentSector].u32FileLength = u32RemainingSequenceBytes;
            u32RemainingSequenceBytes = 0;
        }

        flashEnableWrite();

        eraseFlashFileSector(u8CurrentSector);

        //Wait until the sector is erased.
        vTaskDelay(25*TICKS_PER_MS);

        flashEnableWrite();

        p_u8Data = &LoadStoreBuffer.u8LoadStoreBuffer[0];//Point to the start of the file table
        //Start the write.
        startFlashSectorWrite(u8CurrentSector, *p_u8Data);

        p_u8Data++;

        /*Write the bytes over to flash.
         */

        setDMAState(DMA_FLASH_WRITE);

        //Write until we reach the end of the file table.
        while(p_u8Data != p_u8EndOfData){
            flashWriteByte(*p_u8Data);
            p_u8Data++;
        }

        flashFinishWrite();

        //Store the change in the file table.
        ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex = u8SequenceIndex;
        ftFileTable.flash_file_table[u8CurrentSector].u8SequencePartIndex = u8CurrentSequenceIndex;
        

        u8CurrentSequenceIndex++;
        u8SectorsWritten++;
   }

    ftFileTable.u32AvailableMemorySpace -= FLASH_SECTOR_SIZE*u8SectorsWritten;

    //Set the sequence size in the table.
    ftFileTable.u32SequenceLengthTable[u8SequenceIndex] =  u32BytesWritten;

    //Copy the current settings to the table.
    copyCurrentSettingsToFileTable(u8SequenceIndex+1);//The zero storage is default. Add 1.

    //Write the updated file table.
    writeFlashFileTable();

    u32RAMReadAddress = RESET_RAM_ADDRESS;

    setPlaybackRunStatus(u8PlaybackRunStatus);//Put playback in the state it was in.

    //Notify of the end!!!!!!!!!!!!! Might be more involved to change the state
    //of the device.
    return 1;
}

/*Load the sequence from flash to RAM.*/
uint8_t flashLoadSequence(uint8_t u8SequenceIndex){
    uint8_t u8CurrentSector = 1;
    uint32_t u32SequenceLength = ftFileTable.u32SequenceLengthTable[u8SequenceIndex];
    uint32_t u32Address = 0;
    uint8_t u8PlaybackRunStatus;

    /* Procedure:
     * 1. Find where the sequence is stored in Flash.
     * 2. Read out the sector from Flash.
     * 3. Write it to RAM.
     * 4. Rinse, lather, repeat.
     */

    //Stop playback if it's running.
    u8PlaybackRunStatus = getPlaybackRunStatus();

    setPlaybackRunStatus(STOP);

    //Load the sequence until it's done.
    while(u32SequenceLength > 0){

        //Find the parts of the sequence in order. They always move from the beginning of flash to the end.
        while(ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex != u8SequenceIndex){
            u8CurrentSector++;
        }

        //Now we have found a sector. Read it from Flash.
        flashReadSector(u8CurrentSector);

        //Now write it to RAM
        ramWriteSector(u32Address);
        
        u32Address += FLASH_UTILIZED_SECTOR_SIZE;

        if(u32Address > u32SequenceLength){
            u32SequenceLength = 0;
        }
        else
        {
            u32SequenceLength -= FLASH_UTILIZED_SECTOR_SIZE;
        }

        u8CurrentSector++;
    }

    //Load the settings for the recorded sequence.
    loadSettingsFromFileTable(u8SequenceIndex+1);

    //Set the end of recording address.
    u32SequenceEndAddress = ftFileTable.u32SequenceLengthTable[u8SequenceIndex];//Offset for the default

    setPlaybackRunStatus(u8PlaybackRunStatus);//Put playback in the state it was in.

    //Set the flag to let the system know that a sequence is available for playback.
    setSequenceRecordFlag(TRUE);

    return 1;
}

/*Write one sector of flash into RAM.*/
void ramWriteSector(uint32_t u32Address){

    if(u8FlashWriteEnabledFlag == FALSE){
        flashClearWriteProtection();
    }

    setDMAState(DMA_RAM_FULL_SECTOR);

    CLEAR_RAM_SPI_EN;
#ifdef RAM_SIZE_512K
    LoadStoreBuffer.u8SPIMessage[1] = RAM_WRITE_COMMAND;
    LoadStoreBuffer.u8SPIMessage[2] = u32Address>>8;
    LoadStoreBuffer.u8SPIMessage[3] = u32Address;
#else
    LoadStoreBuffer.u8SPIMessage[0] = RAM_WRITE_COMMAND;
    LoadStoreBuffer.u8SPIMessage[1] = u32Address>>16;
    LoadStoreBuffer.u8SPIMessage[2] = u32Address>>8;
    LoadStoreBuffer.u8SPIMessage[3] = u32Address;
#endif

    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    //If enabling the DMA didn't trigger it. I was seeing one bit less than desired transmitted.
    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
        SPI_MEM_DMA_RX_START;
    }

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
    }

    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);

    SET_RAM_SPI_EN;
}

/*Move through the file table erase a sequence from the file table references.
 Keep track of how much space is liberated.*/
void removeSequenceFromFileTable(uint8_t u8SequenceIndex){
    int i;
    uint8_t u8SectorsErased = 0;

    for(i=FIRST_STORAGE_SECTOR_INDEX;i<FLASH_NUMBER_OF_MEMORY_SECTORS-1;i++){
        if(ftFileTable.flash_file_table[i].u8SequenceIndex == u8SequenceIndex){
            ftFileTable.flash_file_table[i].u8SequenceIndex = FLASH_SECTOR_UNASSIGNED;
            ftFileTable.flash_file_table[i].u32FileLength = 0xFFFFFFFF;
            ftFileTable.flash_file_table[i].u8SequencePartIndex = 0xFF;
            u8SectorsErased++;
        }
    }

    //Give the memory back to the file system.
    ftFileTable.u32AvailableMemorySpace += u8SectorsErased*FLASH_SECTOR_SIZE;
}

//Read out the file table from Flash.
void readFlashFileTable(void){

    //Set up the DMA to read the file table section.
    setDMAState(DMA_FLASH_FILE_TABLE_READ);

    ftFileTable.u8SPIMessage[0] = FLASH_CMD_READ;
    ftFileTable.u8SPIMessage[1] = 0;
    ftFileTable.u8SPIMessage[2] = 0;
    ftFileTable.u8SPIMessage[3] = FLASH_FILE_TABLE_START_ADDRESS;

    //Read out the file table
    CLEAR_FLASH_SPI_EN;

    //Start the DMA for the read.
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
        SPI_MEM_DMA_RX_START;
    }

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
    }

    //Hold up SPI interaction until it's done.
    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);

    SET_FLASH_SPI_EN;
}

//Check to see if the Flash has been initialized.
uint8_t fileTableIsNotInitialized(void){
    if(ftFileTable.flash_file_table[0].u8SequenceIndex == FILE_TABLE_INDEX){
        return false;
    }
    else{
        return true;
    }
}

//Set up a new device.
void initializeFileTable(void){
    uint8_t * p_u8DataStructItem;
    uint8_t * p_u8FileTableItem;
    int i;

    //Sector 0 is for the file table.
    ftFileTable.flash_file_table[0].u8SequenceIndex = FILE_TABLE_INDEX;
    ftFileTable.flash_file_table[0].u8SequencePartIndex = 0;
    ftFileTable.u32AvailableMemorySpace = (FLASH_NUMBER_OF_MEMORY_SECTORS - 1)*FLASH_SECTOR_SIZE;
    ftFileTable.flash_file_table[0].u32FileLength = sizeof(file_table);

    for(i=0;i<MAXIMUM_NUMBER_OF_STORED_SEQUENCES;i++){
        ftFileTable.u32SequenceLengthTable[i] = SEQUENCE_NOT_STORED_LENGTH;
    }

    for(i=1;i<FLASH_NUMBER_OF_MEMORY_SECTORS-1;i++){
        ftFileTable.flash_file_table[i].u32FileLength = 0;
        ftFileTable.flash_file_table[i].u8SequenceIndex = FLASH_SECTOR_UNASSIGNED;
        ftFileTable.flash_file_table[i].u8SequencePartIndex = 0;
    }

    //The first entry in the settings table is the active table.
    //Gotta save it.
    p_u8DataStructItem = getDataStructAddress();
    p_u8FileTableItem = (uint8_t *) &ftFileTable.vdsSettingsTable[0];

    //Copy the structure into the file table.
    for(i=0; i<sizeof(VectrDataStruct); i++){
        *p_u8FileTableItem = *p_u8DataStructItem;
        p_u8FileTableItem++;
        p_u8DataStructItem++;
    }

    //Write the file table
    writeFlashFileTable();

}

void loadSettingsFromFileTable(uint8_t u8Index){
    uint8_t * p_u8DataStructItem = getDataStructAddress();;
    uint8_t * p_u8FileTableItem = (uint8_t *) &ftFileTable.vdsSettingsTable[u8Index];
    int i;

    //Copy the structure into the file table.
    for(i=0; i<sizeof(VectrDataStruct); i++){
        *p_u8DataStructItem = *p_u8FileTableItem;
        p_u8FileTableItem++;
        p_u8DataStructItem++;
    }
}

void copyCurrentSettingsToFileTable(uint8_t u8Index){
    uint8_t * p_u8DataStructItem = getDataStructAddress();;
    uint8_t * p_u8FileTableItem = (uint8_t *) &ftFileTable.vdsSettingsTable[u8Index];
    int i;

    //Copy the structure into the file table.
    for(i=0; i<sizeof(VectrDataStruct); i++){
        *p_u8FileTableItem = *p_u8DataStructItem;
        p_u8FileTableItem++;
        p_u8DataStructItem++;
    }
}

VectrDataStruct * setFileTableDataPointer(uint8_t u8Index){
    return &ftFileTable.vdsSettingsTable[u8Index+1];
}

//Write the flash file table all at once.
void writeFlashFileTable(void){
    uint8_t * p_u8FileTableData;
    uint8_t * p_u8EndofFileTable;

    /* Steps in this process.
     * 1. Write Enable
     * 2. Erase the sector
     * 3. Wait 25ms for this to be over.
     * 4. Write Enable
     * 5. Send first auto-increment byte with address = 5 bytes
     * 6. Send each auto-incrment byte with the command = 2 bytes
     * 7. End with the write disable command to terminate the write.
     */

    //Make sure the write protection bits are cleared
    if(u8FlashWriteEnabledFlag == FALSE){
        flashClearWriteProtection();
    }

    flashEnableWrite();

    eraseFlashFileSector(FLASH_FILE_TABLE_SECTOR);

    //Wait until the sector is erased.
    vTaskDelay(27*TICKS_PER_MS);

    flashEnableWrite();

    ftFileTable.u8SPIMessage[0] = FLASH_CMD_AUTO_INCREMENT_WR;
    ftFileTable.u8SPIMessage[1] = 0;
    ftFileTable.u8SPIMessage[2] = 0;
    ftFileTable.u8SPIMessage[3] = FLASH_FILE_TABLE_START_ADDRESS;

    //Start the write.
    startFileTableWrite();

    /*Write the bytes.
     * One at a time.
     * Check for the DMA source done flag to end to exit.
     * Should transfer two bytes at a time.
     * First byte is
     */
    
    setDMAState(DMA_FLASH_WRITE);

    p_u8FileTableData = (uint8_t *) &ftFileTable.u32AvailableMemorySpace + 1;//Point to the start of the file table + 2
    p_u8EndofFileTable = (uint8_t *) &ftFileTable + sizeof(file_table);
    
    //Write until we reach the end of the file table.
    while(p_u8FileTableData != p_u8EndofFileTable){

        flashWriteByte(*p_u8FileTableData);

        p_u8FileTableData++;
        
    }

    //All done.
    flashFinishWrite();
}

//Write the file table in chunks to not disrupt playback.
void WriteFileTablePiecewise(void){
    static uint8_t * p_u8FileTableData;
    static uint8_t * p_u8EndofFileTable;


    if(u8FlashWriteEnabledFlag == FALSE){
        flashClearWriteProtection();
    }

    switch(u8FileTableWriteState){
        case ENABLE_SECTOR_ERASE:
            flashEnableWrite();
            u8FileTableWriteState = ERASE_SECTOR;
            break;
        case ERASE_SECTOR:
            eraseFlashFileSector(FLASH_FILE_TABLE_SECTOR);
            u8FlashTimer = SECTOR_ERASE_FLASH_TIMER_RESET;
            u8FileTableWriteState = WAIT_FOR_ERASE_TO_COMPLETE;
            break;
        case WAIT_FOR_ERASE_TO_COMPLETE:
            if(u8FlashTimer == 0){
                u8FileTableWriteState = ENABLE_TABLE_WRITE;
            }
            break;
        case ENABLE_TABLE_WRITE:
            flashEnableWrite();
            u8FileTableWriteState = SEND_FIRST_COMMAND;
            break;
        case SEND_FIRST_COMMAND:
            ftFileTable.u8SPIMessage[0] = FLASH_CMD_AUTO_INCREMENT_WR;
            ftFileTable.u8SPIMessage[1] = 0;
            ftFileTable.u8SPIMessage[2] = 0;
            ftFileTable.u8SPIMessage[3] = FLASH_FILE_TABLE_START_ADDRESS;
            startFileTableWrite();
            p_u8FileTableData = &ftFileTable.u8SPIMessage[3] + 2;//Point to the start of the file table + 2
            p_u8EndofFileTable = &ftFileTable.u8SPIMessage[3] + 1 + sizeof(ftFileTable);
            u8FileTableWriteState = SEND_BYTE;
            break;
        case SEND_BYTE:
            /*Maybe write a few bytes here!!!!!!!!!!!!!!!!!!!!!!*/
            setDMAState(DMA_FLASH_WRITE);
            flashWriteByte(*p_u8FileTableData);
            p_u8FileTableData++;

            //Write until we reach the end of the file table.
            if(p_u8FileTableData != p_u8EndofFileTable){
                u8FileTableWriteState = COMPLETE_WRITE;
            }
            break;
        case COMPLETE_WRITE:
            flashFinishWrite();
            u8FileTableWriteState = ENABLE_SECTOR_ERASE;
            u8FileTableWriteFlag = FALSE;
            break;
        default:
            break;
    }
}

void flashClearWriteProtection(void){
    setDMAState(DMA_FLASH_ENABLE_WRITE_STATUS_REGISTER);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);

    setDMAState(DMA_FLASH_WRITE_STATUS_REGISTER);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);

    u8FlashWriteEnabledFlag = TRUE;

}

/*Did the file table change and need to be stored?*/
uint8_t getFileTableWriteFlag(void){
    return u8FileTableWriteFlag;
}

/*Set the flag to get a file table write going.*/
void setFileTableWriteFlag(void){
    u8FileTableWriteFlag = TRUE;
}

/*This timer is used to count down events like a sector erase which takes
 25ms*/
void setFlashTimer(uint8_t u8Count){
    u8FlashTimer = u8Count;
}

uint8_t getFlashTimer(void){
    return u8FlashTimer;
}

/*Standard routine for starting up the SPI DMA*/
void DMARun(void){
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
        SPI_MEM_DMA_RX_START;
    }

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
    }

    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);
}

/*Disable the write to complete the auto-increment write procedure.*/
void flashFinishWrite(void){
    setDMAState(DMA_FLASH_WRITE_FINISH);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;
}

void flashWriteByte(uint8_t u8Byte){
    CLEAR_FLASH_SPI_EN;

    u8FlashWriteMsg[1] = u8Byte;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);
}

void startFileTableWrite(void){

    setDMAState(DMA_FLASH_FILE_TABLE_WRITE_START);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);
}

void flashReadSector(uint8_t u8Sector){
    uint32_t u32Address = u8Sector*FLASH_SECTOR_SIZE;

    LoadStoreBuffer.u8SPIMessage[0] =  FLASH_CMD_READ;
    LoadStoreBuffer.u8SPIMessage[1] = u32Address>>16;
    LoadStoreBuffer.u8SPIMessage[2] = u32Address>>8;
    LoadStoreBuffer.u8SPIMessage[3] = u32Address;

    setDMAState(DMA_FLASH_SECTOR_READ);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;
}

void startFlashSectorWrite(uint8_t u8Sector, uint8_t u8FirstDataByte){
    uint32_t u32Address = u8Sector*FLASH_SECTOR_SIZE;

    setDMAState(DMA_FLASH_WRITE_START);

    u8WriteStartMsg[0] = FLASH_CMD_AUTO_INCREMENT_WR;
    u8WriteStartMsg[1] = u32Address>>16;
    u8WriteStartMsg[2] = u32Address>>8;
    u8WriteStartMsg[3] = u32Address;
    u8WriteStartMsg[4] = u8FirstDataByte;

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);
}

void eraseFlashFileSector(uint8_t u8sector){
    uint32_t u32Address;

    u32Address = FLASH_SECTOR_SIZE * u8sector;

    if(u32Address == 0){
        u32Address = 2;
    }

    //Set the sector address
    u8FlashSectorEraseMsg[1] = u32Address>>16;
    u8FlashSectorEraseMsg[2] = u32Address>>8;
    u8FlashSectorEraseMsg[3] = u32Address;

    setDMAState(DMA_FLASH_SECTOR_ERASE);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);
}

void flashEnableWrite(void){
    setDMAState(DMA_FLASH_WRITE_ENABLE);

    CLEAR_FLASH_SPI_EN;

    DMARun();

    SET_FLASH_SPI_EN;

    vTaskDelay(1);
}

void configureRAM(void){
    uint32_t u32Data;

    vTaskDelay(1*TICKS_PER_MS);
    u32RAMWriteAddress = RESET_RAM_ADDRESS;
    u32RAMReadAddress = RESET_RAM_ADDRESS;
}

void setRAMRetriggerFlag(void){
    u8RAMRetriggerFlag = TRUE;
}

void writeRAM(memory_data_packet * mem_data, uint8_t u8OverdubFlag){
    int i;
    uint16_t * p_u8DataByte = &(mem_data->sample_1.u16XPosition);
    uint8_t u8PlaybackDirection;
    
    /*Write the RAM. 32 bit operation appears broken!!!!!Have to do 8 bit.*/
    /*Set up to write. This procedure writes two samples worth of data.*/

    clearSPIBuffer();//Clear out the read buffer.

    CLEAR_RAM_SPI_EN;
#ifdef RAM_SIZE_512K
    PLIB_SPI_BufferWrite(SPI_MEM, RAM_WRITE_COMMAND);
    PLIB_SPI_BufferWrite(SPI_MEM, u32RAMWriteAddress>>8);
    PLIB_SPI_BufferWrite(SPI_MEM, u32RAMWriteAddress);
#else
    PLIB_SPI_BufferWrite(SPI_MEM, RAM_WRITE_COMMAND);
    PLIB_SPI_BufferWrite(SPI_MEM, u32RAMWriteAddress>>16);
    PLIB_SPI_BufferWrite(SPI_MEM, u32RAMWriteAddress>>8);
    PLIB_SPI_BufferWrite(SPI_MEM, u32RAMWriteAddress);
#endif

    for(i=0; i<3; i++){
        PLIB_SPI_BufferWrite(SPI_MEM, *p_u8DataByte>>8);
        PLIB_SPI_BufferWrite(SPI_MEM, *p_u8DataByte);
        p_u8DataByte++;
    }

    p_u8DataByte = &(mem_data->sample_2.u16XPosition);
    for(i=0; i<3; i++){
        PLIB_SPI_BufferWrite(SPI_MEM, *p_u8DataByte>>8);
        PLIB_SPI_BufferWrite(SPI_MEM, *p_u8DataByte);
        p_u8DataByte++;
    }

    CLEAR_SPI2_TX_INT;
    ENABLE_SPI2_TX_INT;
    //Wait for the transmissions to complete.
    xSemaphoreTake(xSemaphoreSPIMemTxSemaphore, portMAX_DELAY);
    SET_RAM_SPI_EN;

    clearSPIBuffer();//Clear out the read buffer.

    u32RAMWriteAddress += NUMBER_OF_DATA_BYTES;//Advance the write address

    if(u8OverdubFlag == FALSE){
        u32SequenceEndAddress = u32RAMWriteAddress;
        u32EndofRAMRecordingAddress = u32RAMWriteAddress;
    }
    else{
        if(u32RAMWriteAddress >= u32SequenceEndAddress){
            u32RAMWriteAddress = RESET_RAM_ADDRESS;
        }
    }
}

/*Write data to RAM using the DMA.*/
void writeRAM_DMA(memory_data_packet * mem_data, uint8_t u8OverdubFlag){
    int i;
    uint16_t * p_u8DataByte = &(mem_data->sample_1.u16XPosition);
    uint8_t u8PlaybackDirection;

    //Set up the DMA to read the file table section.
    setDMAState(DMA_RAM);

    /*Write the RAM. 32 bit operation appears broken!!!!!Have to do 8 bit.*/
    /*Set up to write. This procedure writes two samples worth of data.*/

    CLEAR_RAM_SPI_EN;
#ifdef RAM_SIZE_512K
    u8SPI_MEM_TX_Buffer[0] = RAM_WRITE_COMMAND;
    u8SPI_MEM_TX_Buffer[1] = u32RAMWriteAddress>>8;
    u8SPI_MEM_TX_Buffer[2] = u32RAMWriteAddress;
#else
    u8SPI_MEM_TX_Buffer[0] = RAM_WRITE_COMMAND;
    u8SPI_MEM_TX_Buffer[1] = u32RAMWriteAddress>>16;
    u8SPI_MEM_TX_Buffer[2] = u32RAMWriteAddress>>8;
    u8SPI_MEM_TX_Buffer[3] = u32RAMWriteAddress;
#endif

    for(i=FIRST_PACKET_START; i<FIRST_PACKET_END; i++){
        u8SPI_MEM_TX_Buffer[i] =  *p_u8DataByte>>8;
        i++;
        u8SPI_MEM_TX_Buffer[i] = *p_u8DataByte;
        p_u8DataByte++;
    }

    p_u8DataByte = &(mem_data->sample_2.u16XPosition);
    for(i=SECOND_PACKET_START; i<SECOND_PACKET_END; i++){
        u8SPI_MEM_TX_Buffer[i] =  *p_u8DataByte>>8;
        i++;
        u8SPI_MEM_TX_Buffer[i] = *p_u8DataByte;
        p_u8DataByte++;
    }
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
     //   SPI_MEM_DMA_TX_START;
        SPI_MEM_DMA_RX_START;
    }
    
    //Trigger the DMA
    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
     // SPI_MEM_DMA_RX_START;
    }

    u32RAMWriteAddress += NUMBER_OF_DATA_BYTES;//Advance the write address

    if(u8OverdubFlag == FALSE){
        u32SequenceEndAddress = u32RAMWriteAddress;
        u32EndofRAMRecordingAddress = u32SequenceEndAddress;
    }
    else{
        if(u32RAMWriteAddress >= u32SequenceEndAddress){
            u32RAMWriteAddress = RESET_RAM_ADDRESS;
        }
    }

    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);

    SET_RAM_SPI_EN;
}

void read_DMA(memory_data_packet * mem_data){
    if(u8SequenceLocationFlag == STORED_IN_RAM){
        readRAM_DMA(mem_data);
    }else{
        readFlash_DMA(mem_data);
    }
}

/*Read data from RAM using DMA*/
void readRAM_DMA(memory_data_packet * mem_data){
    int i;
    uint16_t u16Data;
    uint16_t * p_u8DataByte; 
    uint8_t u8PlaybackDirection = getPlaybackDirection();
    uint8_t u8PlaybackMode = getPlaybackMode();

    //Set up the DMA to read the file table section.
    setDMAState(DMA_RAM);

    CLEAR_RAM_SPI_EN;
#ifdef RAM_SIZE_512K
    u8SPI_MEM_TX_Buffer[0] = RAM_READ_COMMAND;
    u8SPI_MEM_TX_Buffer[1] = u32RAMReadAddress>>8;
    u8SPI_MEM_TX_Buffer[2] = u32RAMReadAddress;
#else
    u8SPI_MEM_TX_Buffer[0] = RAM_READ_COMMAND;
    u8SPI_MEM_TX_Buffer[1] = u32RAMReadAddress>>16;
    u8SPI_MEM_TX_Buffer[2] = u32RAMReadAddress>>8;
    u8SPI_MEM_TX_Buffer[3] = u32RAMReadAddress;
#endif

    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    //If enabling the DMA didn't trigger it. I was seeing one bit less than desired transmitted.
    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
        SPI_MEM_DMA_RX_START;
    }

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
    }

    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);

    SET_RAM_SPI_EN;

    //If the playback is going in the opposite direction, load them backwards.
    if(u8PlaybackDirection == FORWARD_PLAYBACK){
        
        p_u8DataByte = &(mem_data->sample_1.u16XPosition);
    }
    else{
        p_u8DataByte = &(mem_data->sample_2.u16XPosition);
    }
        
    //Now load in the data.
    for(i=FIRST_PACKET_START; i<FIRST_PACKET_END; i++){
        u16Data = u8SPI_MEM_RX_Buffer[i]<<8;
        i++;
        u16Data += u8SPI_MEM_RX_Buffer[i];
        *p_u8DataByte = u16Data;
        p_u8DataByte++;
    }

    if(u8PlaybackDirection == FORWARD_PLAYBACK){     
        p_u8DataByte = &(mem_data->sample_2.u16XPosition);
    }
    else{
        p_u8DataByte = &(mem_data->sample_1.u16XPosition);
    }
       

    for(i=SECOND_PACKET_START; i<SECOND_PACKET_END; i++){
        u16Data = u8SPI_MEM_RX_Buffer[i]<<8;
        i++;
        u16Data += u8SPI_MEM_RX_Buffer[i];
        *p_u8DataByte = u16Data;
        p_u8DataByte++;
    }
   
    if(u8RAMRetriggerFlag == FALSE){
        /*Determine the next address. The next address varies by the playback algorithm.
         * Looping - At the end, it starts over.
         * Pendulum - At the end, it goes backwards and then at the beginning forwards.
         * Flip - Direction changes with a trigger works like looping with direction change.
         * One-shot - At the end, it stops or a trigger can cause it to stop.
         * Retrig - Trigger causes playback to restart.
         */
        switch(u8PlaybackMode){
            case LOOPING:
            case FLIP:
                if(u8PlaybackDirection == FORWARD_PLAYBACK){
                    u32RAMReadAddress += NUMBER_OF_DATA_BYTES;
                    if(u32RAMReadAddress >= u32SequenceEndAddress){
                        u32RAMReadAddress = RESET_RAM_ADDRESS;
                    }
                }
                else{
                    u32RAMReadAddress -= NUMBER_OF_DATA_BYTES;
                    if(u32RAMReadAddress == RESET_RAM_ADDRESS){
                        u32RAMReadAddress = u32SequenceEndAddress;
                    }
                }
                break;
            case PENDULUM:
                if(u8PlaybackDirection == FORWARD_PLAYBACK){
                    u32RAMReadAddress += NUMBER_OF_DATA_BYTES;
                    if(u32RAMReadAddress >= u32SequenceEndAddress){
                        u8PlaybackDirection = REVERSE_PLAYBACK;
                        setPlaybackDirection(u8PlaybackDirection);
                        u32RAMReadAddress -= NUMBER_OF_DATA_BYTES;
                    }
                }
                else{
                    u32RAMReadAddress -= NUMBER_OF_DATA_BYTES;
                    if(u32RAMReadAddress == RESET_RAM_ADDRESS){
                        u8PlaybackDirection = FORWARD_PLAYBACK;
                        setPlaybackDirection(u8PlaybackDirection);
                        u32RAMReadAddress += NUMBER_OF_DATA_BYTES;
                    }
                }
                break;
            case ONESHOT:
            case RETRIGGER:
                if(u8PlaybackDirection == FORWARD_PLAYBACK){
                    u32RAMReadAddress += NUMBER_OF_DATA_BYTES;
                    //Stop playback at the end.
                    if(u32RAMReadAddress >= u32SequenceEndAddress){
                        u32RAMReadAddress = RESET_RAM_ADDRESS;
                        setPlaybackRunStatus(FALSE);
                    }
                }
                else{
                    u32RAMReadAddress -= NUMBER_OF_DATA_BYTES;
                    //Stop playback at the end.
                    if(u32RAMReadAddress == RESET_RAM_ADDRESS){
                        u32RAMReadAddress = u32SequenceEndAddress;
                        setPlaybackRunStatus(FALSE);
                    }
                }
                break;
            default:
                break;
        }
    }
    else{
        u32RAMReadAddress = RESET_RAM_ADDRESS;
        u8RAMRetriggerFlag = FALSE;
    }

    if(u32RAMReadAddress == RESET_RAM_ADDRESS){
        u32SequencePlaybackPosition = 0;
    }
    else{
        u32SequencePlaybackPosition += 12;
    }

}

void readFlash_DMA(memory_data_packet * mem_data){
    int i;
    uint16_t u16Data;
    uint16_t * p_u8DataByte;
    uint8_t u8PlaybackDirection = getPlaybackDirection();
    uint8_t u8PlaybackMode = getPlaybackMode();

    //Set up the DMA to read the file table section.
    setDMAState(DMA_FLASH_READ);

    CLEAR_FLASH_SPI_EN;
    u8SPI_MEM_TX_Buffer[0] = FLASH_CMD_READ;
    u8SPI_MEM_TX_Buffer[1] = u32FlashReadAddress>>16;
    u8SPI_MEM_TX_Buffer[2] = u32FlashReadAddress>>8;
    u8SPI_MEM_TX_Buffer[3] = u32FlashReadAddress;

    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    //If enabling the DMA didn't trigger it. I was seeing one bit less than desired transmitted.
    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
        SPI_MEM_DMA_RX_START;
    }

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
    }

    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);

    SET_FLASH_SPI_EN;

    //If the playback is going in the opposite direction, load them backwards.
    if(u8PlaybackDirection == FORWARD_PLAYBACK){

        p_u8DataByte = &(mem_data->sample_1.u16XPosition);
    }
    else{
        p_u8DataByte = &(mem_data->sample_2.u16XPosition);
    }

    //Now load in the data.
    for(i=4; i<10; i++){
        u16Data = u8SPI_MEM_RX_Buffer[i]<<8;
        i++;
        u16Data += u8SPI_MEM_RX_Buffer[i];
        *p_u8DataByte = u16Data;
        p_u8DataByte++;
    }

    if(u8PlaybackDirection == FORWARD_PLAYBACK){
        p_u8DataByte = &(mem_data->sample_2.u16XPosition);
    }
    else{
        p_u8DataByte = &(mem_data->sample_1.u16XPosition);
    }

    for(i=10; i<15; i++){
        u16Data = u8SPI_MEM_RX_Buffer[i]<<8;
        i++;
        u16Data += u8SPI_MEM_RX_Buffer[i];
        *p_u8DataByte = u16Data;
        p_u8DataByte++;
    }

    if(u8RAMRetriggerFlag == FALSE){
         /*Determine the next address. The next address varies by the playback algorithm.
         * Looping - At the end, it starts over.
         * Pendulum - At the end, it goes backwards and then at the beginning forwards.
         * Flip - Direction changes with a trigger works like looping with direction change.
         * One-shot - At the end, it stops or a trigger can cause it to stop.
         * Retrig - Trigger causes playback to restart.
         */
        switch(u8PlaybackMode){
            case LOOPING:
            case FLIP:
                if(u8PlaybackDirection == FORWARD_PLAYBACK){
                    u32FlashReadAddress += NUMBER_OF_DATA_BYTES;
                    if(u32FlashReadAddress >= u32SequenceEndAddress){
                        u32FlashReadAddress = u32SequenceStartAddress;
                        u32EndOfCurrentFlashSectorAddress = u32FlashReadAddress + FLASH_UTILIZED_SECTOR_SIZE;
                    }
                    else if(u32FlashReadAddress >= u32EndOfCurrentFlashSectorAddress){
                        flashLocateNextSector(u8PlaybackDirection);
                    }
                }
                else{
                    u32FlashReadAddress -= NUMBER_OF_DATA_BYTES;
                    if(u32FlashReadAddress <= u32SequenceStartAddress){
                        u32FlashReadAddress = u32SequenceEndAddress;
                    }
                    else if(u32FlashReadAddress <= u32StartOfCurrentFlashSectorAddress){
                        flashLocateNextSector(u8PlaybackDirection);
                    }
                }
                break;
            case PENDULUM:
                if(u8PlaybackDirection == FORWARD_PLAYBACK){
                    u32FlashReadAddress += NUMBER_OF_DATA_BYTES;
                    if(u32FlashReadAddress >= u32SequenceEndAddress){
                        u8PlaybackDirection = REVERSE_PLAYBACK;
                        setPlaybackDirection(u8PlaybackDirection);
                        //Set end of the flash sequence to the beginning of the current sector.
                        u32EndOfCurrentFlashSectorAddress -= FLASH_UTILIZED_SECTOR_SIZE;
                    }
                    else if(u32FlashReadAddress >= u32EndOfCurrentFlashSectorAddress){
                        flashLocateNextSector(u8PlaybackDirection);
                    }
                }
                else{
                    u32FlashReadAddress -= NUMBER_OF_DATA_BYTES;
                    if(u32FlashReadAddress <= u32SequenceStartAddress){
                        u8PlaybackDirection = FORWARD_PLAYBACK;
                        setPlaybackDirection(u8PlaybackDirection);
                        //Set end of the flash sequence to the beginning of the current sector.
                        u32EndOfCurrentFlashSectorAddress += FLASH_UTILIZED_SECTOR_SIZE;
                    }
                    else if(u32FlashReadAddress <= u32StartOfCurrentFlashSectorAddress){
                        flashLocateNextSector(u8PlaybackDirection);
                    }
                }
                break;
            case ONESHOT:
            case RETRIGGER:
                if(u8PlaybackDirection == FORWARD_PLAYBACK){
                    u32FlashReadAddress += NUMBER_OF_DATA_BYTES;
                    if(u32FlashReadAddress >= u32SequenceEndAddress){
                        u32FlashReadAddress = u32SequenceStartAddress;
                        u32EndOfCurrentFlashSectorAddress = u32SequenceStartAddress + FLASH_UTILIZED_SECTOR_SIZE;
                        setPlaybackRunStatus(FALSE);
                    }
                    else if(u32FlashReadAddress >= u32EndOfCurrentFlashSectorAddress){
                        flashLocateNextSector(u8PlaybackDirection);
                    }
                }
                else{
                    u32FlashReadAddress -= NUMBER_OF_DATA_BYTES;
                    if(u32FlashReadAddress <= u32SequenceStartAddress){
                        u32FlashReadAddress = u32SequenceEndAddress;
                        u32EndOfCurrentFlashSectorAddress = u32SequenceStartAddress - FLASH_UTILIZED_SECTOR_SIZE;
                        setPlaybackRunStatus(FALSE);
                    }
                    else if(u32FlashReadAddress <= u32StartOfCurrentFlashSectorAddress){
                        flashLocateNextSector(u8PlaybackDirection);
                    }
                }
                break;
            default:
                break;
        }
    }else{
        u32FlashReadAddress = u32SequenceStartAddress;
        u32StartOfCurrentFlashSectorAddress = u32FlashReadAddress;
        u32EndOfCurrentFlashSectorAddress = u32FlashReadAddress + FLASH_UTILIZED_SECTOR_SIZE;
        u8RAMRetriggerFlag = FALSE;
    }

    if(u32FlashReadAddress == u32SequenceStartAddress){
        u32SequencePlaybackPosition = 0;
    }
    else{
        u32SequencePlaybackPosition += 12;
    }
}

uint32_t getSequencePlaybackPosition(void){
    return u32SequencePlaybackPosition;
}

uint32_t getFlashFirstSampleAddress(void){
    return u32FirstSampleAddress;
}

void readRAMSector(uint8_t * u8Buffer, uint32_t u32StartReadAddress){

    setDMAState(DMA_RAM_FULL_SECTOR);

    CLEAR_RAM_SPI_EN;
#ifdef RAM_SIZE_512K
    u8Buffer[1] = RAM_READ_COMMAND;
    u8Buffer[2] = u32StartReadAddress>>8;
    u8Buffer[3] = u32StartReadAddress;
#else
    u8Buffer[0] = RAM_READ_COMMAND;
    u8Buffer[1] = u32StartReadAddress>>16;
    u8Buffer[2] = u32StartReadAddress>>8;
    u8Buffer[3] = u32StartReadAddress;
#endif

    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL);
    PLIB_DMA_ChannelXEnable(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL);

    //If enabling the DMA didn't trigger it. I was seeing one bit less than desired transmitted.
    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_RX_DMA_CHANNEL)){
        SPI_MEM_DMA_RX_START;
    }

    if(!PLIB_DMA_ChannelXBusyIsBusy(DMA_ID_0, SPI_MEM_TX_DMA_CHANNEL)){
        SPI_MEM_DMA_TX_START;
    }

    xSemaphoreTake(xSemaphoreDMA_0_RX, portMAX_DELAY);

    SET_RAM_SPI_EN;
}

void setFlashReadNewSequence(uint8_t u8SequenceIndex){
    u32FlashReadAddress = ftFileTable.u32SequenceStartAddress[u8SequenceIndex];
    u32EndOfCurrentFlashSectorAddress = u32FlashReadAddress + FLASH_UTILIZED_SECTOR_SIZE;
    u32SequenceStartAddress = u32FlashReadAddress;
    u32SequenceEndAddress = ftFileTable.u32SequenceEndAddress[u8SequenceIndex];

    if(getPlaybackDirection == FORWARD_PLAYBACK){
        u32FirstSampleAddress = u32SequenceStartAddress + 12;
    }
    else{
        u32FirstSampleAddress = u32SequenceEndAddress - 12;
    }
}

/*Finds the next sector depending on direction.*/
void flashLocateNextSector(uint8_t u8PlaybackDirection){
    uint8_t u8SequenceIndex = getCurrentSequenceIndex();
    uint8_t u8CurrentSector = u32EndOfCurrentFlashSectorAddress>>BITS_OF_MEMORY_SECTOR_SIZE;//Get current sector.

    u8CurrentSector++;

    if(u8PlaybackDirection == FORWARD_PLAYBACK){

        while(ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex != u8SequenceIndex){
            u8CurrentSector++;
        }
        u32FlashReadAddress = u8CurrentSector*FLASH_SECTOR_SIZE;
        u32StartOfCurrentFlashSectorAddress = u32FlashReadAddress;
        u32EndOfCurrentFlashSectorAddress = u32FlashReadAddress + FLASH_UTILIZED_SECTOR_SIZE;
        
    }else{
        u8CurrentSector--;
        while(ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex != u8SequenceIndex){

            u8CurrentSector--;
        }
        u32StartOfCurrentFlashSectorAddress = u8CurrentSector*FLASH_SECTOR_SIZE;
        u32FlashReadAddress = u32EndOfCurrentFlashSectorAddress + FLASH_UTILIZED_SECTOR_SIZE;
        u32EndOfCurrentFlashSectorAddress = u32FlashReadAddress;
    }
}

uint32_t getFlashReadAddress(void){
    return u32FlashReadAddress;
}

uint32_t getMemoryStartAddress(void){
    return u32SequenceStartAddress;
}

void setFlashReadAddress(uint32_t u32Address){

    //Set the read address
    u32FlashReadAddress = u32Address;

    //Figure out the end of sector address.
    u32EndOfCurrentFlashSectorAddress = u32Address + FLASH_UTILIZED_SECTOR_SIZE;

}

/*With manual reads and writes the buffer needs to be cleared to avoid overruns.*/
void clearSPIBuffer(void){
    uint16_t u16FIFOCount;

    u16FIFOCount = PLIB_SPI_FIFOCountGet(SPI_MEM, SPI_FIFO_TYPE_RECEIVE);

    while(u16FIFOCount > 0){
        PLIB_SPI_BufferRead(SPI_MEM);
        u16FIFOCount--;
    }
}

void resetRAMWriteAddress(void){
    u32RAMWriteAddress = RESET_RAM_ADDRESS;
}

void resetRAMReadAddress(void){
    u32RAMReadAddress = RESET_RAM_ADDRESS;
}

void resetRAMEndofReadAddress(void){
    u32SequenceEndAddress = u32EndofRAMRecordingAddress;
}

uint32_t getRAMReadAddress(void){
    return u32RAMReadAddress;
}

void setRAMWriteAddress(uint32_t u32Address){
    u32RAMWriteAddress = u32Address;
}

uint32_t getFlashSequenceLength(uint8_t u8SequenceIndex){
    return ftFileTable.u32SequenceLengthTable[u8SequenceIndex];
}

uint32_t getActiveSequenceLength(void){
    /*If the sequence is stored in RAM, the length is the end of the sequence.*/
    if(u8SequenceLocationFlag == STORED_IN_RAM){
        return u32EndofRAMRecordingAddress;
    }
    else{
        /*The sequence is stored in Flash, the length is stored in the file table. Figure out the
         sequence number and then return its length.*/
        return getFlashSequenceLength(getCurrentSequenceIndex());
    }
}

void changeFlashPlaybackDirection(uint8_t u8NewDirection){
    if(u8NewDirection == FORWARD_PLAYBACK){
        u32EndOfCurrentFlashSectorAddress += FLASH_SECTOR_SIZE;
    }
    else{
        u32EndOfCurrentFlashSectorAddress -= FLASH_SECTOR_SIZE;
    }
}

uint8_t getIsSequenceProgrammed(uint8_t u8SequenceIndex){
    if(ftFileTable.u32SequenceLengthTable[u8SequenceIndex] != SEQUENCE_NOT_STORED_LENGTH){
        return TRUE;
    }
    else{
        return FALSE;
    }
}

uint8_t getStoredSequenceLocationFlag(void){
    return u8SequenceLocationFlag;
}

void setStoredSequenceLocationFlag(uint8_t u8NewState){
    u8SequenceLocationFlag = u8NewState;
}

uint32_t getLengthOfRecording(void){
    return u32SequenceEndAddress;
}

/*Set the RAM or Flash to a new read address. Make sure the read
 address falls on a two sample boundary. 12 bytes.*/
void setNewReadAddress(uint32_t u32NewReadAddress){
    uint8_t u8Modulus = u32NewReadAddress%12;
    uint32_t u32NewFlashReadAddress;
    uint8_t u8CurrentSequence;
    uint8_t u8CurrentSector;
    uint32_t u32FlashReadAddress;//The actual flash read address
    uint32_t u32FlashBytesAccumulated;//How many bytes of the sequence have been accounted for
    uint32_t u32SectorLength;//The length of the current sector.

    if(u32NewReadAddress > u8Modulus){
        u32NewReadAddress -= u8Modulus;
    }

    if(u8SequenceLocationFlag == STORED_IN_RAM){
        u32RAMReadAddress = u32NewReadAddress;
    }else{
        /*If the sequence is stored in flash, then we have to find the appropriate sector.
         Start with the first sector.
         * If the sector belongs to the sequence, check to see if the new read
         * address is less than the length. If it is, we found the address.
         * Else add the length to the pile and keep searching.
         * Add sectors that don't belong to the sequence to build the address.
         *  */

        u8CurrentSequence = getCurrentSequenceIndex();
        u8CurrentSector = 1;
        u32NewFlashReadAddress = FLASH_SECTOR_SIZE;//The first sector is the file table.
        u32FlashBytesAccumulated = 0;

        while(u32FlashBytesAccumulated < u32NewReadAddress){
            if(ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex == u8CurrentSequence){
                u32SectorLength = ftFileTable.flash_file_table[u8CurrentSector].u32FileLength;
                if(u32SectorLength >= u32NewReadAddress){
                    u32FlashBytesAccumulated = u32NewReadAddress;
                    u32NewFlashReadAddress += u32FlashBytesAccumulated;
                }
                else{
                    u32FlashBytesAccumulated += FLASH_SECTOR_SIZE;
                    u32NewFlashReadAddress += FLASH_SECTOR_SIZE;
                }
            }
            else{
                /*If the sector is not part of the sequence, we still need
                 to account for the bytes.*/
                u32NewFlashReadAddress += FLASH_SECTOR_SIZE;
            }
        }

        u32FlashReadAddress = u32NewFlashReadAddress;
    }
}

void setNewSequenceEndAddress(uint32_t u32NewSequenceEndAddress){
    uint8_t u8Modulus = u32NewSequenceEndAddress%12;
    uint32_t u32CalculatedFlashSequenceEndAddress;
    uint8_t u8CurrentSequence;
    uint8_t u8CurrentSector;
    uint32_t u32FlashBytesAccumulated;//How many bytes of the sequence have been accounted for
    uint32_t u32SectorLength;//The length of the current sector.

    if(u32NewSequenceEndAddress > u8Modulus){
        u32NewSequenceEndAddress -= u8Modulus;
    }

    if(u8SequenceLocationFlag == STORED_IN_RAM){
        u32SequenceEndAddress = u32NewSequenceEndAddress;
    }
    else{
        u8CurrentSequence = getCurrentSequenceIndex();
        u8CurrentSector = 1;
        u32CalculatedFlashSequenceEndAddress = FLASH_SECTOR_SIZE;//The first sector is the file table.
        u32FlashBytesAccumulated = 0;

        while(u32FlashBytesAccumulated < u32NewSequenceEndAddress){
            if(ftFileTable.flash_file_table[u8CurrentSector].u8SequenceIndex == u8CurrentSequence){
                u32SectorLength = ftFileTable.flash_file_table[u8CurrentSector].u32FileLength;
                if(u32SectorLength >= u32NewSequenceEndAddress){
                    u32FlashBytesAccumulated = u32NewSequenceEndAddress;
                    u32CalculatedFlashSequenceEndAddress += u32FlashBytesAccumulated;
                }
                else{
                    u32FlashBytesAccumulated += FLASH_SECTOR_SIZE;
                    u32CalculatedFlashSequenceEndAddress += FLASH_SECTOR_SIZE;
                }
            }
            else{
                /*If the sector is not part of the sequence, we still need
                 to account for the bytes.*/
                u32CalculatedFlashSequenceEndAddress += FLASH_SECTOR_SIZE;
            }
        }

        u32SequenceEndAddress = u32CalculatedFlashSequenceEndAddress;
    }
}

void LoadSettingsFromFileTable(void){
    VectrDataStruct * VectrData;
    uint8_t * p_u8FileTableData;
    uint8_t * p_u8VectrData;
    int i;

    VectrData = getVectrDataStart();
    p_u8VectrData = (uint8_t *) VectrData;
    p_u8FileTableData = (uint8_t*)ftFileTable.vdsSettingsTable;

    for(i=0; i<sizeof(VectrDataStruct);i++){
        *p_u8VectrData = *p_u8FileTableData;
        p_u8VectrData++;
        p_u8FileTableData++;
    }
}