#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stdint.h>
#include <time.h>


typedef uint8_t time_amp_enc[60];

void ampEncTime(time_amp_enc d_out, time_t time_in);

bool isLeapYear( uint16_t year);

uint8_t dstCalc( uint16_t month, uint16_t dom, uint16_t dow);