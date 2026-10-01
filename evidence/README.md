# Kraken V4 battery support evidence

Receiver: 1532:056c. Synapse USB capture: interface 5, Output report 02.

| Validation | Result |
| --- | --- |
| Synapse vs USB battery | 44% / 0x2c, 46% / 0x2e |
| Linux exact-query replay | 49% |
| HeadsetControl charging | 55%, BATTERY_CHARGING |
| HeadsetControl unplugged | 83%, BATTERY_AVAILABLE |
| HeadsetControl headset off | BATTERY_UNAVAILABLE, bounded timeout |

The ZIP contains a 230-frame battery/charging-only USB capture, screenshots,
original frame numbers and hardware logs. The full USB bus capture is excluded.
Linux hidraw backend tested. Windows/macOS HIDAPI remain untested.
