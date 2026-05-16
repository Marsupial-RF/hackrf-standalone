/*
 * hackrf-standalone: power-on TX loop mode.
 *
 * The normal HackRF TX path waits for USB to fill usb_samp_buffer. This module
 * fills that same 32 KiB sample ring from standalone_iq.h instead, and keeps the
 * M4 producer counter ahead of the M0 SGPIO consumer counter.
 */
#include "standalone_hackrf.h"

#if STANDALONE_HACKRF_ENABLE

#include <stdint.h>
#include <string.h>

#include <fixed_point.h>
#include <leds.h>
#include <m0_state.h>
#include <radio.h>
#include <sgpio.h>
#include <streaming.h>
#include <transceiver_mode.h>

#include "../usb_api_transceiver.h"
#include "../usb_buffer.h"
#include "standalone_iq.h"

#if (STANDALONE_HACKRF_REFILL_CHUNK_BYTES > USB_SAMP_BUFFER_SIZE)
#error "STANDALONE_HACKRF_REFILL_CHUNK_BYTES must be <= USB_SAMP_BUFFER_SIZE"
#endif

#if ((STANDALONE_HACKRF_REFILL_CHUNK_BYTES & 31U) != 0U)
#error "STANDALONE_HACKRF_REFILL_CHUNK_BYTES must be a multiple of 32"
#endif

/* IQ bytes are interleaved I,Q, so the byte count must be even and nonzero. */
typedef char standalone_iq_length_must_be_even[
	((DATA_TO_TRANSMIT_IN_LOOP_BYTE_LEN & 1U) == 0U) ? 1 : -1];

typedef char standalone_iq_must_not_be_empty[
	(DATA_TO_TRANSMIT_IN_LOOP_BYTE_LEN > 0U) ? 1 : -1];

static uint64_t standalone_total_filled;

static uint32_t min_u32(const uint32_t a, const uint32_t b)
{
	return (a < b) ? a : b;
}

static void standalone_hackrf_error_halt(void)
{
	while (true) {
		led_on(LED1);
		led_on(LED3);
	}
}

static void copy_iq_pattern_to_sample_ring(uint64_t absolute_start, uint32_t length)
{
	const uint32_t iq_len = DATA_TO_TRANSMIT_IN_LOOP_BYTE_LEN;

	if (iq_len < 2U || (iq_len & 1U)) {
		standalone_hackrf_error_halt();
	}

	uint32_t pattern_index = (uint32_t) (absolute_start % iq_len);
	uint32_t ring_index = ((uint32_t) absolute_start) & USB_SAMP_BUFFER_MASK;

	while (length > 0U) {
		const uint32_t bytes_until_ring_wrap = USB_SAMP_BUFFER_SIZE - ring_index;
		const uint32_t bytes_until_pattern_wrap = iq_len - pattern_index;
		const uint32_t run = min_u32(
			length,
			min_u32(bytes_until_ring_wrap, bytes_until_pattern_wrap));

		memcpy(&usb_samp_buffer[ring_index],
		       &data_to_transmit_in_loop[pattern_index],
		       run);

		length -= run;
		ring_index += run;
		pattern_index += run;

		if (ring_index >= USB_SAMP_BUFFER_SIZE) {
			ring_index = 0U;
		}
		if (pattern_index >= iq_len) {
			pattern_index = 0U;
		}
	}
}

void standalone_hackrf_configure_radio(void)
{
	/* Same as hackrf_transfer -f. */
	radio_reg_write(
		&radio,
		RADIO_BANK_ACTIVE,
		RADIO_FREQUENCY_RF,
		STANDALONE_HACKRF_FREQ_HZ * FP_ONE_HZ);
	radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_FREQUENCY_IF, RADIO_UNSET);
	radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_FREQUENCY_LO, RADIO_UNSET);
	radio_reg_write(&radio, RADIO_BANK_ACTIVE, RADIO_IMAGE_REJECT, RADIO_UNSET);

	/* Same as hackrf_transfer -s. */
	radio_reg_write(
		&radio,
		RADIO_BANK_ACTIVE,
		RADIO_SAMPLE_RATE,
		STANDALONE_HACKRF_SAMPLE_RATE_HZ * SR_FP_ONE_HZ);

	/* Same as hackrf_transfer -b. */
	radio_reg_write(
		&radio,
		RADIO_BANK_ACTIVE,
		RADIO_BB_BANDWIDTH_TX,
		STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ);
	radio_reg_write(
		&radio,
		RADIO_BANK_ACTIVE,
		RADIO_BB_BANDWIDTH_RX,
		STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ);

	/* Same as hackrf_transfer -x. */
	radio_reg_write(
		&radio,
		RADIO_BANK_ACTIVE,
		RADIO_GAIN_TX_IF,
		STANDALONE_HACKRF_TXVGA_GAIN_DB);

	/* Same as hackrf_transfer -a. */
	radio_reg_write(
		&radio,
		RADIO_BANK_ACTIVE,
		RADIO_GAIN_TX_RF,
		STANDALONE_HACKRF_RF_AMP_ENABLE);
}

void standalone_hackrf_tx_mode(uint32_t seq)
{
	standalone_total_filled = 0U;

	/* Fill the entire 32 KiB sample ring before enabling SGPIO streaming. */
	copy_iq_pattern_to_sample_ring(standalone_total_filled, USB_SAMP_BUFFER_SIZE);
	standalone_total_filled += USB_SAMP_BUFFER_SIZE;

	/* transceiver_startup() resets m0_count/m4_count, so publish m4_count after it. */
	transceiver_startup(TRANSCEIVER_MODE_TX);
	m0_state.m4_count = (uint32_t) standalone_total_filled;

	baseband_streaming_enable(&sgpio_config);

	while (transceiver_request.seq == seq) {
		const uint32_t queued =
			((uint32_t) standalone_total_filled) - m0_state.m0_count;

		if (queued <=
		    (USB_SAMP_BUFFER_SIZE - STANDALONE_HACKRF_REFILL_CHUNK_BYTES)) {
			copy_iq_pattern_to_sample_ring(
				standalone_total_filled,
				STANDALONE_HACKRF_REFILL_CHUNK_BYTES);
			standalone_total_filled += STANDALONE_HACKRF_REFILL_CHUNK_BYTES;

			/* Publish newly written bytes only after the ring has been filled. */
			m0_state.m4_count = (uint32_t) standalone_total_filled;
		}

		radio_update(&radio);
	}

	transceiver_shutdown();
}

#endif /* STANDALONE_HACKRF_ENABLE */
