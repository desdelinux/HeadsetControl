# Razer Kraken V4 battery protocol

Device tested: wireless receiver `1532:056c`, HID interface 5, vendor usage page
`0xff14`, usage 1. Wired PID `056b` and other Razer models are not supported by
this implementation. The device also exposes `0xff13` and standard LampArray
collections; battery queries use the `0xff14` Output report.

Traffic was captured with Linux usbmon while Synapse 4 ran in a Windows 11 VM
with USB passthrough. Synapse release notes reported version
`4.0.86.2609150817`. Battery readings at 44% and 46% matched the captured payload,
and replaying the battery query from Linux returned 49% as charging progressed.
The C++ implementation subsequently returned 55% / BATTERY_CHARGING with external
power, then 83% / BATTERY_AVAILABLE after disconnecting the external charger.
With the headset switched off and receiver attached, the query timed out and
HeadsetControl reported BATTERY_UNAVAILABLE without displaying a cached percentage.

## Requests

Both requests are exactly 64 bytes including report ID 2. Synapse uses
`SET_REPORT` with `bmRequestType=0x21`, `bRequest=0x09`, `wValue=0x0202`,
`wIndex=5`, `wLength=64`: an Output report, not a Feature report.

| Offset | Battery query | Charging query |
| --- | --- | --- |
| 0–10 | `02 00 60 00 00 00 04 00 00 80 21` | `02 00 60 00 00 00 04 00 00 80 2a` |
| 11–61 | All zero | All zero |
| 62 | `c7` | `cc` |
| 63 | `00` | `00` |

These requests are copied from captured Synapse packets. No checksum-generation
rule, initialization sequence or Feature request is needed for the implementation.

## Responses

Responses arrive through interrupt IN endpoint `0x84`, also 64 bytes including
the report ID. The first 13 bytes are:

```
02 02 60 00 00 00 05 00 80 80 <command> 01 01
```

Byte 13 is the battery percentage (command `21`) or charging flag (command `2a`,
0 without external power, 1 while charging). Captured battery values include
`2c` (44%), `2d` (45%), `2e` (46%); Linux replay returned `31` (49%).

Asynchronous notifications begin `02 0a 60`, include an incrementing field at
offset 3, and use `02` instead of `01` at offset 11. They must not be accepted
as synchronous replies. Other reports may also arrive on the same HID node.
The implementation sends each query once and reads until a matching reply or
a fixed deadline. It checks the full response length and value range.

## Validation limits

- Retain minimal captures for the PR.
- Linux HIDAPI hidraw has been tested; HIDAPI on macOS/Windows
  has not been tested on hardware.

## Sidetone (follow-up branch)

Synapse toggles monitoring with captured opcode `98` (payload byte13=1/0).
It sets the native level with opcode `99`. A fifth capture of actual Synapse
slider actions on 2026-09-30 observed every native level from 0 through 15.
The implementation stores all 16 complete packets, including literal trailing
bytes. No checksum or additional native value is generated.

HeadsetControl's 0..128 input selects OFF for 0. Positive requests select
`max(1, floor(input * 15 / 128))`; the returned level is
`ceil(native * 128 / 15)`, so reapplying it selects the same packet. This is an
ordinal mapping of captured settings, not a measurement of acoustic gain or
an assertion that the Synapse slider percentage is linear in gain.

On Linux the user heard sidetone only while USB microphone capture was active.
A PipeWire input stream opened with pw-record measured and discarded audio;
no software loopback/playback was used. Mic must be extended and unmuted.
The command alone does not keep the USB capture endpoint open; a voice app
or other capture client must do that. ACKs alone are not acoustic validation.

Second capture2026-09-30: FULL frame8225, repeated8541; UI50 restored8663;
OFF8739. First captureON19610,HALF19618,OFF21508. On a setting error the
implementation attempts to restore OFF with the captured OFF packet.

Battery PR remains separate. Noise cancellation is still not identified.

## Lighting toggle (follow-up branch)

The captured brightness command uses opcodec1, byte13=0 forOFF and64 for
Synapse25%. The boolean lights API enables this captured25% brightness and
disables with the captured0 packet. It does not preserve/read arbitrary
brightness, implement colors/effects, or use LampArray. User confirmed both
physicalOFF and re-illumination on Linux with these exact requests. Responses
match the captured zero acknowledgment; no checksum/range is synthesized.

## Control evidence

[Filtered capture](razer_kraken_v4_controls_evidence.pcap) contains only HID
control/interrupt transactions for sidetone and brightness from three captures.
The [manifest](razer_kraken_v4_controls_evidence.json) records original frame
numbers, capture hashes and all 16 literal sidetone packets. Audio/isochronous
transfers and other USB devices are excluded. Acoustic validation covers
sidetone ON/FULL and OFF, and physical light OFF/ON25%; individual intermediate
sidetone levels have capture and unit-test evidence, not separate listening tests.
