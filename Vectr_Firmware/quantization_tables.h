/* 
 * File:   quantization_tables.h
 * Author: matth
 *
 * Created on June 18, 2014, 11:42 AM
 */

#ifndef QUANTIZATION_TABLES_H
#define	QUANTIZATION_TABLES_H

#define LENGTH_OF_CHROMATIC_SCALE   120
#define LENGTH_OF_MAJOR_SCALE       70
#define LENGTH_OF_PENTATONIC_SCALE  50
#define LENGTH_OF_OCTAVE_SCALE      10


extern const uint32_t u32QuantizedSpeedTable[15];
extern const uint16_t u16LogTable[1024];
extern const uint16_t u16ChromaticScale[LENGTH_OF_CHROMATIC_SCALE];
extern const uint16_t u16MajorScale[70];
extern const uint16_t u16PentatonicScale[50];
const uint16_t u16OctaveScale[10];
extern const uint32_t u32LogSpeedTable[	1024];
#define LENGTH_OF_LOG_TABLE     1024
#define BIT_SHIFT_TO_LOG_TABLE 6
#define MAX_LOG_TABLE_VALUE 65535
#define LENGTH_OF_SPEED_TABLE   1024
#define ZERO_SPEED_INDEX    511 //Standard speed.
#define ZERO_SPEED_VALUE    239999

#endif	/* QUANTIZATION_TABLES_H */

