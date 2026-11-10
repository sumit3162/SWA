# Hardware notes

## Bed node

| part | notes |
|---|---|
| ESP32 dev board (WROOM-32) | any 30-pin devkit |
| MAX30102 / MAXREFDES117 board | I2C, address 0x57 |
| Li-Po 3.7 V 500 mAh | |
| TP4056 board with protection | the version with DW01 protection, not the bare one |
| 3.3 V LDO | low dropout, see below |
| 2 x 100k resistor | battery divider to GPIO34 |

### Pins (same as `firmware/node/src/config.h`)

| MAX30102 | ESP32 |
|---|---|
| VIN | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |
| INT | GPIO19 (wired, not used yet) |

Battery: VBAT -- 100k -- GPIO34 -- 100k -- GND. The code multiplies the ADC reading by 2.

### Regulator

A 3.7 V cell runs from about 4.2 V full down to about 3.3 V empty. The AMS1117 needs roughly 1.1 V of headroom, so it falls out of regulation almost straight away. Use something like HT7333, AP2112K or MCP1700. Many ESP32 devkits already have an AMS1117 on board; feeding those from the 5 V pin of a boost converter, or bypassing them, is another way around it.

### Sensor placement

Fingertip works best. The sensor must touch the skin lightly, too much pressure kills the pulse signal. The node treats IR below 20000 counts as "no finger" (`setFingerThreshold`).

## Hub

ESP32-S3-BOX-3, nothing to add. Pins used (all in `firmware/hub/src/config.h`):

| function | GPIO |
|---|---|
| I2C SDA / SCL | 8 / 18 |
| I2S MCLK / BCLK / WS / DOUT | 2 / 17 / 45 / 15 |
| speaker amp enable | 46 |
| LCD MOSI / SCK / CS / DC / RST / BL | 6 / 7 / 5 / 4 / 48 / 47 |
| acknowledge button (mute button) | 1 |

## Cost (from the paper)

| item | Rs |
|---|---|
| ESP32-S3-BOX-3 hub | 4,500 |
| sensing node, per bed | 1,200 |
| enclosures and accessories | 800 |
| hub + 2 beds | 7,700 |
