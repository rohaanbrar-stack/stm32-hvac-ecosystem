# stm32-hvac-ecosystem
Bare-metal STM32 HVAC automation system — servo-driven vent control with temperature sensing and wireless coordination across three nodes. All STM32 firmware written in register-level C (RM0008), no HAL.

## Problem
Attic bedroom accumulates heat year-round. Two adjustable ceiling vents are the only physical control. This system automates them: temperature sensors on each vent node report duct temperature to a central control node, which drives servos to open/close vents based on whether the duct air is actually cooler than the room.

## Architecture
Three STM32F103C8T6 (Blue Pill) nodes communicate over nRF24L01+ wireless. The control node relays data to an ESP8266 gateway for WiFi/app access.

| Node | Location | Controls | Sensors |
|---|---|---|---|
| Ceiling | Bedroom ceiling vents | 2x SG90 servo | BMP280 (duct temp) |
| Bathroom | Bathroom ceiling vent | 1x SG90 servo | BMP280 (duct temp) |
| Control | Near door | Hub, SD logging, OLED, buttons | BMP280 (room ambient), PIR |

**Data path:** Ceiling/Bathroom STM32 → nRF24 → Control STM32 → UART → ESP8266 → WiFi → app

## Repo Structure
```
stm32-hvac-ecosystem/
├── hvac_ceiling_node/     # Ceiling node firmware (STM32F103C8T6)
├── hvac_control_node/     # Control node firmware (STM32F103C8T6)
├── hvac_bathroom_node/    # Bathroom node firmware (STM32F103C8T6) — coming
└── common/                # Shared drivers (I2C, USART, clock) — coming
```

I2C, USART, and clock drivers are adapted from [stm32-imu-logger](https://github.com/rohaanbrar-stack/stm32-imu-logger).

## Status
- Phase 0 — Mechanical validation ✅
- Phase 1 — Single-node POC (BMP280 + servo PWM on Ceiling node) ✅
- Phase 2 — nRF24 wireless, 2-node link ✅
- Phase 3 — Three-node star topology 🟡 In progress
- Phase 4 — Reactive control logic + OLED ⬜
- Phase 5 — ESP8266 gateway + web interface ⬜

## Later Plans
- Weather forecast integration via ESP8266 — proactively open/close vents based on outdoor conditions before the room heats up
- AI-driven thermostat intelligence — learned model predicts optimal vent positions and recommends AC unit temperature when duct air alone won't cool the room
- Mobile app / local web dashboard for manual override and live temp monitoring
- PIR-based shutoff — stop cooling an empty room

## Hardware
- 3x STM32F103C8T6 Blue Pill
- 3x SG90 servo
- 3x BMP280 (temp + pressure, I2C)
- 3x nRF24L01+
- 1x ESP8266 NodeMCU (WiFi gateway, Arduino framework)
- 1x SSD1306 OLED (I2C)
- 1x HC-SR501 PIR sensor
- ST-Link V2, CP2102 USB-TTL
