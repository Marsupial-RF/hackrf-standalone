/*
 * hackrf-standalone: power-on TX loop support for HackRF USB firmware.
 *
 * This header is the friendly configuration file. Edit these #defines or pass
 * them through CMake with -DNAME=value.
 */
#pragma once

#include <stdint.h>

/* Set to 0 to compile normal host-driven HackRF firmware again. */
#ifndef STANDALONE_HACKRF_ENABLE
#define STANDALONE_HACKRF_ENABLE 1
#endif

/* Same as hackrf_transfer -f <freq_hz>. Change this before building. */
#ifndef STANDALONE_HACKRF_FREQ_HZ
#define STANDALONE_HACKRF_FREQ_HZ 915000000ULL
#endif

/* Same as hackrf_transfer -s 20000000. HackRF supports 2..20 MHz. */
#ifndef STANDALONE_HACKRF_SAMPLE_RATE_HZ
#define STANDALONE_HACKRF_SAMPLE_RATE_HZ 20000000ULL
#endif

/* Same as hackrf_transfer -b 20000000. */
#ifndef STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ
#define STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ 20000000U
#endif

/* Same as hackrf_transfer -x <0..47>. Start low for testing. */
#ifndef STANDALONE_HACKRF_TXVGA_GAIN_DB
#define STANDALONE_HACKRF_TXVGA_GAIN_DB 0U
#endif

/* Same as hackrf_transfer -a <0|1>. Keep 0 unless you really need the amp. */
#ifndef STANDALONE_HACKRF_RF_AMP_ENABLE
#define STANDALONE_HACKRF_RF_AMP_ENABLE 0U
#endif

/* Must be <= 32768 and a multiple of 32. 8192 is conservative at 20 Msps. */
#ifndef STANDALONE_HACKRF_REFILL_CHUNK_BYTES
#define STANDALONE_HACKRF_REFILL_CHUNK_BYTES 8192U
#endif

#if STANDALONE_HACKRF_TXVGA_GAIN_DB > 47
#error "STANDALONE_HACKRF_TXVGA_GAIN_DB maps to hackrf_transfer -x and must be 0..47"
#endif

#if (STANDALONE_HACKRF_RF_AMP_ENABLE != 0) && (STANDALONE_HACKRF_RF_AMP_ENABLE != 1)
#error "STANDALONE_HACKRF_RF_AMP_ENABLE maps to hackrf_transfer -a and must be 0 or 1"
#endif

#if STANDALONE_HACKRF_SAMPLE_RATE_HZ < 2000000ULL || STANDALONE_HACKRF_SAMPLE_RATE_HZ > 20000000ULL
#error "HackRF sample rate should be in the 2 MHz..20 MHz range"
#endif

#if STANDALONE_HACKRF_ENABLE
void standalone_hackrf_configure_radio(void);
void standalone_hackrf_tx_mode(uint32_t seq);
#endif
