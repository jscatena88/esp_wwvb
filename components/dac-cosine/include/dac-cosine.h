#include "driver/gpio.h"
#include "driver/dac.h"

/*
 * Enable cosine waveform generator on a DAC channel
 */
void dac_cosine_enable(dac_channel_t channel);

/*
 * Set frequency of internal CW generator common to both DAC channels
 *
 * clk_8m_div: 0b000 - 0b111
 * frequency_step: range 0x0001 - 0xFFFF
 *
 */
void dac_frequency_set(int clk_8m_div, int frequency_step);

/*
 * Scale output of a DAC channel using two bit pattern:
 *
 * - 00: no scale
 * - 01: scale to 1/2
 * - 10: scale to 1/4
 * - 11: scale to 1/8
 *
 */
void dac_scale_set(dac_channel_t channel, int scale);

/*
 * Offset output of a DAC channel
 *
 * Range 0x00 - 0xFF
 *
 */
void dac_offset_set(dac_channel_t channel, int offset);

/*
 * Invert output pattern of a DAC channel
 *
 * - 00: does not invert any bits,
 * - 01: inverts all bits,
 * - 10: inverts MSB,
 * - 11: inverts all bits except for MSB
 *
 */
void dac_invert_set(dac_channel_t channel, int invert);

/*
* Get the theoretical frequency of a channel given frequency settings
*
*/

float calc_channel_freq( int clk_8m_div, int frequency_step);