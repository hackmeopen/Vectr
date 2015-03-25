/* 
 * File:   quantization_tables.h
 * Author: matth
 *
 * Created on June 18, 2014, 11:42 AM
 */

#ifndef QUANTIZATION_TABLES_H
#define	QUANTIZATION_TABLES_H

#define LENGTH_OF_CHROMATIC_SCALE   121
#define LENGTH_OF_MINOR_SCALE       71
#define LENGTH_OF_PENTATONIC_SCALE  51
#define LENGTH_OF_OCTAVE_SCALE      11


extern const uint32_t u32QuantizedSpeedTable[15];
extern const uint16_t u16LogTable[1024];
extern const uint16_t u16ChromaticScale[LENGTH_OF_CHROMATIC_SCALE];
extern const uint16_t u16MinorScale[LENGTH_OF_MINOR_SCALE];
extern const uint16_t u16PentatonicScale[LENGTH_OF_PENTATONIC_SCALE];
const uint16_t u16OctaveScale[LENGTH_OF_OCTAVE_SCALE];
extern const uint32_t u32LogSpeedTable[	1024];
#define LENGTH_OF_LOG_TABLE     1024
#define BIT_SHIFT_TO_LOG_TABLE 6
#define MAX_LOG_TABLE_VALUE 65535
#define LENGTH_OF_SPEED_TABLE   1024
#define ZERO_SPEED_INDEX    511 //Standard speed.
#define ZERO_SPEED_VALUE    239999
#define FASTEST_SPEED       14999
#define SLOWEST_SPEED       3839999

#endif	/* QUANTIZATION_TABLES_H */

