# hackrf-standalone

`hackrf-standalone` is a small firmware fork of HackRF USB firmware. Its purpose is simple:

**power the HackRF, and it immediately transmits a short raw IQ sequence in a loop without a host PC streaming samples.**

This is meant for HackRF firmware experiments where the waveform is already known at compile time. The default payload included here is our preferred short IQ sequence, but users can replace it with their own.

## What changed

The normal HackRF TX path expects a USB host to stream samples. This fork keeps the original M0/SGPIO/DAC path, but replaces the USB sample source:

1. The firmware boots.
2. It configures TX frequency, sample rate, baseband filter, TX VGA gain, and RF amp state.
3. It requests TX mode automatically.
4. The M4 fills the normal `usb_samp_buffer` ring with `data_to_transmit_in_loop[]`.
5. The existing M0 SGPIO code consumes that ring and transmits continuously.

No CPLD or FPGA change is required for HackRF One.

## Files to edit

### 1. TX settings

Edit:

```text
firmware/hackrf_usb/standalone_hackrf/standalone_hackrf.h
```

Important settings:

```c
#define STANDALONE_HACKRF_FREQ_HZ                 915000000ULL
#define STANDALONE_HACKRF_SAMPLE_RATE_HZ          20000000ULL
#define STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ   20000000U
#define STANDALONE_HACKRF_TXVGA_GAIN_DB           0U
#define STANDALONE_HACKRF_RF_AMP_ENABLE           0U
```

These map to `hackrf_transfer` like this:

```text
STANDALONE_HACKRF_FREQ_HZ               == hackrf_transfer -f
STANDALONE_HACKRF_SAMPLE_RATE_HZ        == hackrf_transfer -s
STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ == hackrf_transfer -b
STANDALONE_HACKRF_TXVGA_GAIN_DB         == hackrf_transfer -x
STANDALONE_HACKRF_RF_AMP_ENABLE         == hackrf_transfer -a
```

Example equivalent command:

```bash
hackrf_transfer -t payload.iq -f 915000000 -s 20000000 -b 20000000 -x 8 -a 0 -R
```

Equivalent firmware settings:

```c
#define STANDALONE_HACKRF_FREQ_HZ                 915000000ULL
#define STANDALONE_HACKRF_SAMPLE_RATE_HZ          20000000ULL
#define STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ   20000000U
#define STANDALONE_HACKRF_TXVGA_GAIN_DB           8U
#define STANDALONE_HACKRF_RF_AMP_ENABLE           0U
```

For bench testing, start with:

```c
#define STANDALONE_HACKRF_TXVGA_GAIN_DB 0U
#define STANDALONE_HACKRF_RF_AMP_ENABLE 0U
```

Then raise `STANDALONE_HACKRF_TXVGA_GAIN_DB` gradually. Valid TX VGA range is `0..47`. Use `STANDALONE_HACKRF_RF_AMP_ENABLE 1U` only when you really want the RF amp on.

### 2. IQ payload

Edit:

```text
firmware/hackrf_usb/standalone_hackrf/standalone_iq.h
```

The payload is:

```c
static const int8_t data_to_transmit_in_loop[] __attribute__((aligned(4))) = {
    /* signed int8 I,Q,I,Q,... */
};
```

Keep all of these:

- `[]` after `data_to_transmit_in_loop`
- `__attribute__((aligned(4)))`
- an even number of bytes, because the format is interleaved I,Q

At `20 MS/s`, the default 160-byte payload is 80 complex samples, so one complete payload loop is:

```text
80 / 20000000 = 4 microseconds
```

## Ubuntu build dependencies

From a fresh Ubuntu machine:

```bash
sudo apt-get update
sudo apt-get install -y \
  git cmake build-essential \
  gcc-arm-none-eabi \
  dfu-util hackrf \
  python3 python3-yaml
```

If the repo came from a zip and `firmware/libopencm3` is empty or missing:

```bash
cd ~/hackrf/firmware
git clone https://github.com/mossmann/libopencm3.git
```

If the repo is a real git checkout with submodules:

```bash
cd ~/hackrf
git submodule init
git submodule update
```

## Build firmware

From the repo root:

```bash
cd ~/hackrf/firmware/hackrf_usb
rm -rf build
mkdir build
cd build
cmake ..
make -j"$(nproc)"
```

The important output file is:

```text
hackrf_usb.bin
```

## Optional: override settings from the command line

Instead of editing `standalone_hackrf.h`, you can pass values to CMake:

```bash
cd ~/hackrf/firmware/hackrf_usb
rm -rf build
mkdir build
cd build
cmake .. \
  -DSTANDALONE_HACKRF_FREQ_HZ=915000000ULL \
  -DSTANDALONE_HACKRF_SAMPLE_RATE_HZ=20000000ULL \
  -DSTANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ=20000000U \
  -DSTANDALONE_HACKRF_TXVGA_GAIN_DB=8 \
  -DSTANDALONE_HACKRF_RF_AMP_ENABLE=0
make -j"$(nproc)"
```

## Flash firmware

Flash to SPI:

```bash
cd ~/hackrf/firmware/hackrf_usb/build
sudo hackrf_spiflash -w hackrf_usb.bin
```

Then unplug/replug the HackRF. With this fork, it should start transmitting the compiled IQ loop as soon as it boots.

## Temporary DFU RAM test

For a temporary RAM-loaded test instead of permanent SPI flash:

```bash
cd ~/hackrf/firmware/hackrf_usb/build
sudo dfu-util --device 1fc9:000c --alt 0 --download hackrf_usb.dfu
```

DFU mode on HackRF One: hold the DFU button while plugging in or pressing reset, release after the 3V3 LED turns on.

## Restore normal host-streaming behavior

Set:

```c
#define STANDALONE_HACKRF_ENABLE 0
```

or build with:

```bash
cmake .. -DSTANDALONE_HACKRF_ENABLE=0
```

Then rebuild and flash again.

## Responsible RF note

Transmit only on frequencies, bandwidths, and power levels you are allowed to use. For lab work, start at low gain and use a dummy load or attenuator.

## Original HackRF project

This fork is based on the open-source HackRF project by Great Scott Gadgets. Original HackRF documentation is available at HackRF Read the Docs and the upstream HackRF repository.
