# stm32-hvac-ecosystem

![Language](https://img.shields.io/badge/language-C-blue) ![Platform](https://img.shields.io/badge/platform-STM32F103-green) ![HAL](https://img.shields.io/badge/HAL-none-red)

Bare-metal STM32 HVAC automation system — servo-driven vent control with temperature sensing and a two-node wired link. All STM32 firmware written in register-level C (RM0008), no HAL.

## Problem

Attic bedroom accumulates heat year-round. Two adjustable ceiling vents are the only physical control. This system automates them: a sensor node at the vents reports duct temperature to a central control node, which drives servos to open/close the vents based on whether the duct air is actually cooler than the room.

## Architecture

Two STM32F103C8T6 (Blue Pill) nodes connected over a wired UART link. The control node relays data to an ESP8266 gateway for WiFi/app access.

| Node | Location | Controls | Sensors |
|---|---|---|---|
| Ceiling | Bedroom ceiling vents | 2x SG90 servo | BMP280 (duct temp) |
| Control | Near door | Hub, SD logging, OLED, buttons | BMP280 (room ambient), PIR |

**Data path:** Ceiling STM32 → UART → Control STM32 → UART → ESP8266 → WiFi → app

A third node (bathroom vent) is deferred — it replicates the Ceiling node with one servo and slots into the existing hub topology whenever it's built.

## Why Wired? — The Wireless Postmortem

The wireless phase was achieved: on 2026-06-29 the two nodes exchanged live packets with auto-acknowledgment confirmed, completing Phase 2. The link then died and never returned. Two weeks of debugging ended with the firmware proven correct and the transport cut anyway:

| Hypothesis | Test | Result |
|---|---|---|
| Driver/firmware bug | Datasheet audit; diff vs known-good reference driver; firmware-swap bisect | **Exonerated** |
| Config not landing | Spaced single-register readbacks, both nodes | All values landed |
| TX power margin | Swept -18dBm → 0dBm | No change |
| Data rate / crystal tolerance | 2Mbps, 1Mbps, 250kbps | No change |
| Dead module batch | Two module types, two separate orders, incl. a never-powered pair | Identical failure |
| Supply problem | DC metered at module VCC pins under TX load | 3.3V steady |
| Anything radiating? | CONT_WAVE carrier test — 0dBm carrier at 1m, receiver polling RPD | **RPD:0 — no RF, ever** |

The modules are SI24R1-class counterfeits, and on this silicon **SPI reads are unreliable while writes land** — register dumps were misdirection; real verdicts came from behavior (retry counters, RPD) and widely-spaced single reads. Final picture: a flawless protocol engine and zero detectable RF across every module, rate, and power — dead RF front-ends. With a fixed deadline and the radio being transport rather than the point, wireless was cut for a 3-wire UART link. The nRF24 driver stays in the repo, proven against a reference implementation, ready to swap back in if provenance modules (e.g. Ebyte E01-ML01DP5) ever replace the clones.

## Repo Structure

```
stm32-hvac-ecosystem/
├── hvac_ceiling_node/     # Ceiling node firmware (STM32F103C8T6)
├── hvac_control_node/     # Control node firmware (STM32F103C8T6)
└── common/                # Shared drivers (I2C, USART, clock) — coming
```

I2C, USART, and clock drivers are adapted from [stm32-imu-logger](https://github.com/rohaanbrar-stack/stm32-imu-logger). Each node's `nRF24.c` is the retired-but-exonerated wireless driver (see postmortem).

## Status

- Phase 0 — Mechanical validation ✅
- Phase 1 — Single-node POC (BMP280 + servo PWM on Ceiling node) ✅
- Phase 2 — nRF24 wireless 2-node link ✅ (2026-06-29 — later retired; see postmortem)
- Phase 3 — Wired UART inter-node link 🟡 In progress
- Phase 4 — Reactive control logic + OLED ⬜
- Phase 5 — ESP8266 gateway + web interface ⬜

## Later Plans

- Weather forecast integration via ESP8266 — proactively open/close vents based on outdoor conditions before the room heats up
- AI-driven thermostat intelligence — learned model predicts optimal vent positions and recommends AC unit temperature when duct air alone won't cool the room
- Mobile app / local web dashboard for manual override and live temp monitoring
- PIR-based shutoff — stop cooling an empty room
- Bathroom vent node (Ceiling node replica, 1 servo)
- Wireless revival with provenance nRF24 modules — driver is ready and waiting

## Hardware

- 2x STM32F103C8T6 Blue Pill
- 3x SG90 servo
- 2x BMP280 (temp + pressure, I2C)
- 1x ESP8266 NodeMCU (WiFi gateway, Arduino framework)
- 1x SSD1306 OLED (I2C)
- 1x HC-SR501 PIR sensor
- ST-Link V2, CP2102 USB-TTL
- ~~nRF24L01+ modules~~ — retired (see postmortem)

## Author

Rohaan Brar — embedded systems learning project, Purdue CompE.
