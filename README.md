## ESP-IDF Project

```bash
$ idf.py --version
ESP-IDF v6.1
```

## Schematics

```mermaid
graph LR
    MCU[ESP32-S3]
    ENC[Rotary encoder]
    BZ[Passive buzzer]
    SV[Servo]
    PSU[External 5V supply]

    MCU ---|"GPIO2 <-> CLK"| ENC
    MCU ---|"GPIO1 <-> DT"| ENC
    MCU ---|"GPIO42 <-> SW"| ENC
    MCU ===|"3V3 <-> +"| ENC
    MCU -.-|GND| ENC

    MCU ---|"GPIO41 <-> SIGNAL"| BZ
    MCU -.-|GND| BZ

    MCU ---|"GPIO15 <-> Signal"| SV
    PSU ===|"5V <-> VCC"| SV
    PSU -.-|GND| SV

    PSU -.-|GND| MCU
```

## Commands

```shell
# Build project
$ idf.py app

# Flash firmware
$ idf.py flash

# Monitor
$ idf.py monitor

# Clean build files
$ idf.py fullclean
```
