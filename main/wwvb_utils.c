#include "wwvb_utils.h"

void ampEncTime(time_amp_enc d_out, time_t time_in) {
    struct tm *time_in_struct;
    time_in_struct = gmtime(&time_in);

    time_t test; 

    uint8_t M = 2;
    //By convention 0 = 0, 1 =1, 2 = Marker
    d_out[0] = M;
    //pack minutes
    uint8_t minutes = time_in_struct->tm_min;
    uint8_t min_singles = minutes % 10;
    uint8_t min_tens = minutes / 10;
    d_out[1] = min_tens >> 2 && 1;
    d_out[2] = min_tens >> 1 && 1;
    d_out[3] = min_tens && 1;
    d_out[4] = 0; //by definition
    d_out[5] = min_singles >> 3 && 1;
    d_out[6] = min_singles >> 2 && 1;
    d_out[7] = min_singles >> 1 && 1;
    d_out[8] = min_singles && 1;

    d_out[9] = M;
    d_out[10] = 0; //by definition
    d_out[11] = 0; //by definition

    //Pack hours
    uint8_t hours = time_in_struct->tm_hour;
    uint8_t hour_singles = hours % 10;
    uint8_t hours_tens = hours / 10;
    d_out[12] = hours_tens >> 1 && 1;
    d_out[13] = hours_tens && 1;
    d_out[14] = 0; //by definition
    d_out[15] = hour_singles >> 3 && 1;
    d_out[16] = hour_singles >> 2 && 1;
    d_out[17] = hour_singles >> 1 && 1;
    d_out[18] = hour_singles && 1;

    d_out[19] = M;
    d_out[20] = 0; //by definition
    d_out[21] = 0; //by definition

    uint16_t days = time_in_struct->tm_yday;
    uint8_t day_singles = days % 10;
    uint8_t day_tens = (days / 10) % 10;
    uint8_t day_hundreds = days / 100;
    d_out[22] = day_hundreds >> 1 && 1;
    d_out[23] = day_hundreds && 1;
    d_out[24] = 0; //by definition
    d_out[25] = day_tens >> 3 && 1;
    d_out[26] = day_tens >> 2 && 1;
    d_out[27] = day_tens >> 1 && 1;
    d_out[28] = day_tens && 1;
    d_out[29] = M; //by definition
    d_out[30] = day_singles >> 3 && 1;
    d_out[31] = day_singles >> 2 && 1;
    d_out[32] = day_singles >> 1 && 1;
    d_out[33] = day_singles && 1;

    d_out[34] = 0; //by definition
    d_out[35] = 0; //by definition

    //DUT1 sign, just set to negative randomly
    d_out[36] = 0;
    d_out[37] = 1;
    d_out[38] = 0;

    d_out[39] = M;

    //DUT1 correction, just setting this to zero
    d_out[40] = 0;
    d_out[41] = 0;
    d_out[42] = 0;
    d_out[43] = 0;

    d_out[44] = 0; //by definition

    //year packing
    uint16_t years = time_in_struct->tm_year;
    uint8_t year_singles = years % 10;
    uint8_t year_tens = (years / 10) % 10;
    d_out[45] = year_tens >> 3 && 1;
    d_out[46] = year_tens >> 2 && 1;
    d_out[47] = year_tens >> 1 && 1;
    d_out[48] = year_tens && 1;
    d_out[49] = M;
    d_out[50] = year_singles >> 3 && 1;
    d_out[51] = year_singles >> 2 && 1;
    d_out[52] = year_singles >> 1 && 1;
    d_out[53] = year_singles && 1;

    d_out[54] = 0; //by definition
    //Leap year indicator
    d_out[55] = isLeapYear(years) ? 1 : 0;

    //Leap second, just making this zero
    d_out[56] = 0;

    //DST info
    uint8_t DST_info = dstCalc(time_in_struct->tm_mon, time_in_struct->tm_mday, time_in_struct->tm_wday);
    d_out[57] = DST_info >> 1;
    d_out[58] = DST_info && 1;


    d_out[59] = M;
}

bool isLeapYear( uint16_t year) {
    if (year % 400 == 0)
        return true;
    else if (year % 100 == 0)
        return false;
    else if (year % 4 == 0)
        return true;
    else
        return false;
    
}

uint8_t dstCalc( uint16_t month, uint16_t dom, uint16_t dow) {
    // according to NIST
    // begins at 2:00 a.m. on the second Sunday of March
    // ends at 2:00 a.m. on the first Sunday of November

    /* From WWVB spec
    * 00 = DST not in effect.
    * 10 = DST begins today.
    * 11 = DST in effect.
    * 01 = DST ends today.
    */

    if (month <= 2 || 12 <= month) return 0;
    if (4 <= month && month <= 10) return 3;

    if (month == 3) {
        if (dom - dow == 7)
            return 2;
        else 
            return (dom - dow > 7) ? 3 : 0;
    } else {
        // month == 11
        if (dom - dow == 0)
            return 1;
        else
            return (dom - dow < 0) ? 3 : 0;
    }
}