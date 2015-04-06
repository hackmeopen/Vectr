/* 
 * File:   mem.h
 * Author: matthewheins
 *
 * Created on February 23, 2014, 9:23 PM
 */

#ifndef MEM_H
#define	MEM_H

#include "app.h"
#include "master_control.h"

enum{
    STORED_IN_RAM = 0,
    STORED_IN_FLASH
};


/*RAM Instructions*/
#define RAM_READ_COMMAND    0x03
#define RAM_WRITE_COMMAND   0x02
#define RAM_READ_MODE_REGISTER_COMMAND  0x05
#define RAM_WRITE_MODE_REGISTER_COMMAND 0x01

#define RAM_BYTE_MODE_MASK          0x00
#define RAM_PAGE_MODEMASK           0x80
#define RAM_SEQUENTIAL_MODE_MASK    0x40

#define RESET_RAM_ADDRESS   0


#define NUMBER_OF_DATA_ITEMS    6
#define NUMBER_OF_DATA_BYTES    12 //X,Y,Z Position for two samples

#define RAM_SIZE_512K 1


#ifdef RAM_SIZE_512K
    #define LENGTH_OF_RAM_ADDRESS   2 //BYTES
    #define LENGTH_OF_DMA_MESSAGE   15
    #define HIGHEST_RAM_ADDRESS 0xFA00//1Mbit/8
    #define FIRST_PACKET_START  3
    #define FIRST_PACKET_END    9
    #define SECOND_PACKET_START 9
    #define SECOND_PACKET_END   14
#else
    #define LENGTH_OF_RAM_ADDRESS   3 //BYTES
    #define LENGTH_OF_DMA_MESSAGE   16
    #define HIGHEST_RAM_ADDRESS 0x1F400//1Mbit/8
    #define FIRST_PACKET_START  4
    #define FIRST_PACKET_END    10
    #define SECOND_PACKET_START 10
    #define SECOND_PACKET_END   15
#endif


#define LENGTH_OF_FLASH_READ_DMA_MESSAGE    16

#define FLASH_SECTOR_UNASSIGNED   0xFF
#define MAXIMUM_NUMBER_OF_STORED_SEQUENCES  5
#define SEQUENCE_NOT_STORED_LENGTH  0xFFFFFFFF

/*Flash Definitions*/
#define FLASH_TOTAL_SIZE    0x1FFFF
#define FLASH_SECTOR_SIZE   0x1000
#define FLASH_UTILIZED_SECTOR_SIZE 0xFFC //The last 4 bytes don't fit with the packet size
#define BITS_OF_MEMORY_SECTOR_SIZE  12
#define LENGTH_OF_FLASH_ADDRESS     3
#define FIRST_STORAGE_SECTOR_INDEX  1
#define FILE_TABLE_INDEX            0xFE
#define FLASH_MAX_LENGTH_OF_FILE_TABLE    0x3FF
#define FLASH_FILE_TABLE_SECTOR 0
#define FLASH_NUMBER_OF_MEMORY_SECTORS     32
#define MAX_NUMBER_OF_FILE_TABLE_ENTRIES    FLASH_MAX_LENGTH_OF_FILE_TABLE/sizeof(flash_file_table_entry)
#define FLASH_MANU_ID_ADDRESS               0
#define FLASH_DEVICE_ID_ADDRESS             1
#define FLASH_FILE_TABLE_START_ADDRESS      2
#define FLASH_TOTAL_USED_MEMORY_OFFSET              0 //How much memory is used
#define FLASH_NUM_BYTES_TOTAL_USED_MEMORY           4
#define FLASH_TOTAL_LENGTH_OF_FILE_TABLE_OFFSET     FLASH_TOTAL_USED_MEMORY_OFFSET+FLASH_NUM_BYTES_TOTAL_USED_MEMORY//The complete length of the file table address
#define FLASH_NUM_BYTES_LENGTH_OF_FILE_TABLE        4//32 bit
#define FLASH_START_OF_FILE_TABLE                   FLASH_TOTAL_LENGTH_OF_FILE_TABLE_OFFSET+FLASH_NUM_BYTES_LENGTH_OF_FILE_TABLE
#define FLASH_SETTINGS_STORAGE_START_OFFSET         0x3FF
#define FLASH_NUMBER_OF_STORED_SETTINGS             5 //1 current settings and 4 stored settings
#define FLASH_LENGTH_OF_SETTINGS_STORAGE            FLASH_NUMBER_OF_STORED_SETTINGS*sizeof(VectrDataStruct)
#define FLASH_CURRENT_SETTING_START_OFFSET          FLASH_SETTINGS_STORAGE_START_OFFSET
#define FLASH_SEQUENCE_1_SETTING_START_OFFSET       FLASH_CURRENT_SETTING_START_OFFSET + sizeof(VectrDataStruct)
#define FLASH_SEQUENCE_2_SETTING_START_OFFSET       FLASH_SEQUENCE_1_SETTING_START_OFFSET + sizeof(VectrDataStruct)
#define FLASH_SEQUENCE_3_SETTING_START_OFFSET       FLASH_SEQUENCE_2_SETTING_START_OFFSET + sizeof(VectrDataStruct)
#define FLASH_SEQUENCE_4_SETTING_START_OFFSET       FLASH_SEQUENCE_3_SETTING_START_OFFSET + sizeof(VectrDataStruct)

#define SECTOR_ERASE_FLASH_TIMER_RESET  3 //length of time for event in 10ms increments

/*Status Register Bits*/
#define FLASH_STATUS_BUSY               0
#define FLASH_STATUS_WRITE_EN           1
#define FLASH_STATUS_BLOCK_PROT_0       2
#define FLASH_STATUS_BLOCK_PROT_1       3
#define FLASH_STATUS_AUTO_INCREMENT     6
#define FLASH_STATUS_BLOCK_PROT_LOCK    7

#define FLASH_CMD_EN_WRITE_STATUS_REG   0x50
#define FLASH_CMD_WRITE_STATUS_REG      0x01
#define FLASH_CMD_WRITE_ENABLE          0x06
#define FLASH_CMD_CHIP_ERASE            0x60
#define FLASH_CMD_BLOCK_ERASE           0x52
#define FLASH_CMD_SECTOR_ERASE          0x20
#define FLASH_CMD_WRITE_DISABLE         0x04
#define FLASH_CMD_AUTO_INCREMENT_WR     0xAF
#define FLASH_CMD_BYTE_PROGRAM          0x02
#define FLASH_CMD_READ                  0x03
#define FLASH_CMD_READ_STATUS_REG       0x05

/*The file system is set up to store up to 4 files.
 The start of the flash contains 1 entry for each of the four files that contains
 the total length of each file. Following the lengths are file table entries.
 Each file table entry is a beginning and end of a file section with */

/*Flash looks like:
 * 0 - Manufacturer ID
 * 1 - Device ID
 * There are 32 sectors. The file table has an entry for each.
 * The first byte in the entry tells which sequence it belongs to.
 * The second byte tells where the sector belongs in the sequence.
 * 
 * The first sector is used for the file system.
 * If the sector 0 entry is FF, the Flash is uninitialized. Initialize it to 0.
 * All others should be FF. This cuts down on the write cycles. FF means the sector is unassigned.
 *
 */
enum{
    DMA_RAM = 0,
    DMA_RAM_FULL_SECTOR,
    DMA_FLASH_WRITE_ENABLE,
    DMA_FLASH_WRITE_START,
    DMA_FLASH_WRITE_FINISH,
    DMA_FLASH_READ,
    DMA_FLASH_SECTOR_READ,
    DMA_FLASH_SECTOR_ERASE,
    DMA_FLASH_FILE_TABLE_READ,
    DMA_FLASH_WRITE,
    DMA_FLASH_SETTINGS_TABLE_READ,
    DMA_FLASH_SETTINGS_TABLE_WRITE,
    DMA_FLASH_FILE_TABLE_WRITE_START,
    DMA_FLASH_ENABLE_WRITE_STATUS_REGISTER,
    DMA_FLASH_WRITE_STATUS_REGISTER

};

typedef struct{
    uint8_t u8SequenceIndex;//Which saved file is the entry associated with? Top four bits are the
    uint8_t u8SequencePartIndex;//Where in the sequence does the sector fit
    uint32_t u32FileLength;//How long is the entry?
}flash_file_table_entry;

typedef struct{
    uint8_t u8SPIMessage[4];
    uint32_t u32AvailableMemorySpace;
    uint32_t u32SequenceLengthTable[MAXIMUM_NUMBER_OF_STORED_SEQUENCES];
    uint32_t u32SequenceStartAddress[MAXIMUM_NUMBER_OF_STORED_SEQUENCES];
    uint32_t u32SequenceEndAddress[MAXIMUM_NUMBER_OF_STORED_SEQUENCES];
    flash_file_table_entry flash_file_table[FLASH_NUMBER_OF_MEMORY_SECTORS];
    VectrDataStruct vdsSettingsTable[MAXIMUM_NUMBER_OF_STORED_SEQUENCES+1];
}file_table;


void spi_mem_init(int baudRate, int clockFrequency);
void RAMHardwareTest(void);
void configureRAM(void);
void readRAM(memory_data_packet * mem_data);
void readRAM_DMA(memory_data_packet * mem_data);
void readFlash_DMA(memory_data_packet * mem_data);
void resetRAMWriteAddress(void);
void resetRAMReadAddress(void);
uint32_t getRAMReadAddress(void);
void setRAMWriteAddress(uint32_t u32Address);
void writeRAM(memory_data_packet * mem_data, uint8_t u8OverdubFlag);
void setRAMRetriggerFlag(void);
void readFlashFileTable(void);
uint8_t fileTableIsNotInitialized(void);
void initializeFileTable(void);
uint8_t getFileTableWriteFlag(void);
void setFileTableWriteFlag(void);
void writeFlashFileTable(void);
void flashWriteByte(uint8_t u8Byte);
void startFileTableWrite(void);
void eraseFlashFileSector(uint8_t u8sector);
void flashEnableWrite(void);
void setFlashTimer(uint8_t u8Count);
uint8_t getFlashTimer(void);
void DMARun(void);
void read_DMA(memory_data_packet * mem_data);
void ramWriteSector(uint32_t u32Address);
void removeSequenceFromFileTable(uint8_t u8SequenceIndex);
void flashFinishWrite(void);
void flashReadSector(uint8_t u8Sector);
void startFlashSectorWrite(uint8_t u8Sector, uint8_t u8FirstDataByte);
void readRAMSector(uint8_t * u8Buffer, uint32_t u32StartReadAddress);
void flashClearWriteProtection(void);
uint32_t getFlashSequenceLength(uint8_t u8SequenceIndex);
uint8_t getIsSequenceProgrammed(uint8_t u8SequenceIndex);
uint32_t getLengthOfRecording(void);
void setFlashReadAddress(uint32_t u32Address);
void resetRAMEndofReadAddress(void);
void setNewReadAddress(uint32_t u32NewReadAddress);
void setNewSequenceEndAddress(uint32_t u32NewSequenceEndAddress);
uint32_t getSequencePlaybackPosition(void);
void changeFlashPlaybackDirection(uint8_t u8NewDirection);
void copyCurrentSettingsToFileTable(uint8_t u8Index);
void loadSettingsFromFileTable(uint8_t u8Index);
VectrDataStruct * setFileTableDataPointer(uint8_t u8Index);
uint32_t getFlashFirstSampleAddress(void);
uint32_t getFlashReadAddress(void);
uint32_t getMemoryStartAddress(void);
uint32_t getEndOfSequenceAddress(void);
uint8_t fileTableIsNotCurrent(void);
void updateFileTable(void);
void setClockSyncFlag(void);
void synchronizeReadWriteRAMAddress(uint32_t u32NewAddress);

#endif	/* MEM_H */

