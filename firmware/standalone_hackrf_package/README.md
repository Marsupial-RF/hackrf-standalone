# standalone_hackrf drop-in module

Place this directory here:

```text
firmware/hackrf_usb/standalone_hackrf/
```

Edit:

```text
firmware/hackrf_usb/standalone_hackrf/standalone_hackrf.h
firmware/hackrf_usb/standalone_hackrf/standalone_iq.h
```

`standalone_hackrf.h` contains compile-time equivalents of `hackrf_transfer` options:

```text
-f  -> STANDALONE_HACKRF_FREQ_HZ
-s  -> STANDALONE_HACKRF_SAMPLE_RATE_HZ
-b  -> STANDALONE_HACKRF_BASEBAND_FILTER_BW_HZ
-x  -> STANDALONE_HACKRF_TXVGA_GAIN_DB
-a  -> STANDALONE_HACKRF_RF_AMP_ENABLE
```

`standalone_iq.h` contains your signed int8 interleaved I,Q payload array.
