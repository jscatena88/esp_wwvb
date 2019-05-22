#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stdint.h>
#include <time.h>


typedef uint8_t time_enc[60];

void phaseEncTime(time_enc d_out, time_t time_in);

static inline void phaseSyncWordInsert(time_enc d_out);

void ampEncTime(time_enc d_out, time_t time_in);

bool isLeapYear( uint16_t year);

uint8_t dstCalc( uint16_t month, uint16_t dom, uint16_t dow);