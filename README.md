# STM32 Bare-Metal HVAC Ecosystem

![Language](https://img.shields.io/badge/language-C-blue) ![Platform](https://img.shields.io/badge/platform-STM32F103-green) ![HAL](https://img.shields.io/badge/HAL-none-red)

Two-node HVAC automation for an attic bedroom that holds heat year-round, where two adjustable ceiling vents are the only physical control. A ceiling node reports duct temperature over a framed UART link; a control node compares it against room temperature, drives the vent servos, logs every decision to microSD, watches the room for occupancy, and streams telemetry to an ESP8266 serving a live dashboard over WiFi. Register-level C against RM0008 throughout — no HAL, no LL.

Validated on the bench, not installed in the room — mounting is future work, not an omission.

---

## Features

- Register-level drivers for I2C, SPI, three USARTs, TIM2, EXTI, and clock configuration
- Self-framing wired protocol with checksums, acknowledged commands, and byte-timeouts on every receive
- Reactive control that opens a vent only when the room needs it **and** the duct air can actually help
- Three USARTs live at once on one F103 — debug, inter-node link, WiFi bridge
- SG90 servo control via TIM2 hardware PWM, 0.5–2.5 ms pulse range
- SSD1306 OLED with room, duct, setpoint, vent state, and a `WON'T HELP` indicator
- FatFS logging to microSD over a custom diskio layer — one CSV record per control cycle
- PIR occupancy on EXTI with a 24-hour no-motion shutdown
- ESP8266 gateway serving an auto-refreshing dashboard and a JSON endpoint
- 1 Hz control loop paced by a hardware timer, not a counted delay
- Retired-but-exonerated nRF24 driver kept in-tree after a two-week falsification postmortem

---

## Hardware

**Control node** — STM32F103C8T6, near the door. Hub, logging, display, gateway bridge.

| Component | Interface | Pins |
|---|---|---|
| BMP280 (room temp) | I2C1 | PB6 (SCL), PB7 (SDA) |
| SSD1306 OLED | I2C1 | PB6 (SCL), PB7 (SDA) |
| MicroSD (ADA254) | SPI1 | PA5 (SCK), PA6 (MISO), PA7 (MOSI), PA4 (CS) |
| HC-SR501 PIR | GPIO + EXTI0 | PB0 |
| Ceiling node link | USART2 | PA2 (TX), PA3 (RX) |
| ESP8266 bridge | USART3 | PB10 (TX), PB11 (RX) |
| CP2102 debug | USART1 | PA9 (TX), PA10 (RX) |

**Ceiling node** — STM32F103C8T6, at the vents. Sensing and actuation.

| Component | Interface | Pins |
|---|---|---|
| BMP280 (duct temp) | I2C1 | PB6 (SCL), PB7 (SDA) |
| SG90 servos ×2 | TIM2 CH1/CH2 PWM | PA0, PA1 |
| Control node link | USART2 | PA2 (TX), PA3 (RX) |
| CP2102 debug | USART1 | PA9 (TX), PA10 (RX) |

**Gateway** — ESP8266 NodeMCU (LoLin, FT232R bridge), SoftwareSerial on D5 (RX) / D6 (TX) at 9600 8-N-1.

The two STM32s are wired TX→RX crossed on USART2 with a shared ground. A third node for the bathroom vent replicates the ceiling node with one servo and slots into the same topology whenever it's built.

---

## Control Logic

Setpoint 25.56 °C (78.0 °F), deadband 0.85 °C, helping margin 1.10 °C — all compile-time constants. Four states: `CLOSED`, `COOLING`, `HEATING`, `AWAY`.

| Room vs setpoint | Duct vs room | Action |
|---|---|---|
| Above deadband | colder by margin | **open** — cooling |
| Above deadband | not colder | closed — *vents won't help* |
| Below deadband | warmer by margin | **open** — heating |
| Below deadband | not warmer | closed — *vents won't help* |
| Within deadband | — | closed — satisfied |
| no motion for 24 h | — | closed — `AWAY` |

Opening takes two conditions, not one. A hot room with hot duct air means the furnace is running, so the vent stays shut rather than blowing it in — that case is what the `WON'T HELP` line on the OLED exists to make visible.

Occupancy is a 24-hour shutdown rather than moment-to-moment gating, and the long horizon is what makes it simple. At ten minutes there's a real tension — closing a vent fights a room you're about to walk back into — but nobody re-enters seconds after a full day away, so closing is unambiguously correct and the question collapses into a single override.

---

## How It Works

Both nodes run a 1 Hz loop off TIM2. The ceiling node samples its BMP280 and sends a telemetry frame; the control node samples its own, receives that frame, and runs the table above. On a state change it sends a servo command and waits for the ACK.

Frames are `[0xAA][type][payload][checksum]` — telemetry fire-and-forget, commands acknowledged. Every receive is bounded by a byte-timeout instead of blocking, so a node whose peer has gone quiet keeps running its own loop. That was added before the WiFi bridge existed, and it's why three USARTs coexist on the control node without deadlocking.

The PIR stamps `last_motion` from an EXTI interrupt. Crossing the 24-hour timeout forces the vents shut and the state to `AWAY`; any motion releases it. Each cycle appends a record to `log.csv`:

```
timestamp, temp, tempVent, state, wontHelp, last_motion
```

Temperatures are hundredths of a degree Celsius everywhere in the firmware and converted only where they're displayed. The control node also writes one line per cycle to USART3:

```
T <temp>, <tempVent>, <state>, <setpoint>\n
```

The ESP8266 assembles that byte stream into complete lines, parses each into a struct, and keeps the last good reading. `/` serves a self-contained HTML page; `/data` serves the current reading:

```json
{"room":2891,"duct":2962,"state":0,"set":2556}
```

The page polls `/data` every two seconds and redraws itself, so the browser drives the refresh and the ESP stays a passive responder. Before the first complete line arrives `/data` answers **503** rather than serving zeros that would render as a convincing 32.0 °F.

---

## Quick Start

**Requirements:** STM32CubeIDE, ST-Link V2, Arduino IDE with the ESP8266 board package, a FAT32 microSD card of 2 GB or less, and a WiFi network.

1. Open `hvac_ceiling_node/` and `hvac_control_node/` as separate STM32CubeIDE projects and flash each over SWD.
2. Format the SD card FAT32 and insert it before powering the control node — the file is opened once at startup.
3. In `hvac_esp_gateway/`, copy `secrets_example.h` to `secrets.h` and fill in your WiFi credentials. It's gitignored and stays on your machine.
4. Flash `hvac_esp_gateway.ino`, open the serial monitor at 115200, and note the IP printed after it joins. Close the monitor before re-flashing — it holds the COM port.
5. Open that IP in a browser on the same network.

⚠️ **The SD driver is SDSC-only.** Block reads and writes send byte addresses and `SD_Init` never reads the OCR to learn otherwise, so cards above 2 GB won't address correctly. The fix is small — CMD58 after ACMD41, read CCS, make the multiply conditional — and was deferred on schedule, not difficulty.

---

## Why Wired — The Wireless Postmortem

The wireless link worked once — 2026-06-29, live packets with auto-ack — then died and never returned. Two weeks of debugging ended with the firmware proven correct and the transport cut anyway:

| Hypothesis | Test | Result |
|---|---|---|
| Driver/firmware bug | Datasheet audit; diff vs reference driver; firmware-swap bisect | **Exonerated** |
| Config not landing | Spaced single-register readbacks, both nodes | All values landed |
| TX power margin | Swept -18dBm → 0dBm | No change |
| Data rate / crystal tolerance | 2Mbps, 1Mbps, 250kbps | No change |
| Dead module batch | Two module types, two orders, incl. a never-powered pair | Identical failure |
| Anything radiating? | CONT_WAVE carrier, 0dBm at 1m, receiver polling RPD | **RPD:0 — no RF, ever** |

SI24R1-class counterfeits whose SPI reads lie while writes land — a flawless protocol engine driving dead RF front-ends. The radio was transport, not the point, so it was cut for a 3-wire UART link. The driver stays in-tree, exonerated, ready if real modules ever replace the clones.

---

## Design Notes

The postmortem has an uncomfortable sequel. Months later the ESP8266 wouldn't enumerate, and the same method got pointed at it — a ledger of candidates, each tested and eliminated — concluding the board was dead. The board was fine. The silkscreen had been read as a CH340G, every test hunted for a CH340G, and the FT232R sitting on the right COM port appeared in the ledger's own notes and was dismissed as a phantom. The method was rigorous about the candidates it chose and never questioned the assumption that chose them. Discipline downstream of a bad premise just makes the wrong answer arrive with better evidence.

The recurring fix is that a measurement almost always exists and is almost always cheaper than the reasoning it would replace. The 1 Hz loop was once a counted busy-wait labelled *one-second delay*; the first timestamp against a real clock measured it at 5.84 seconds. A serial port that refused to open was diagnosed by asking the OS which process held the handle rather than theorising about drivers.

One defect ships knowingly. The ESP link runs on bit-banged `SoftwareSerial`, and WiFi interrupts disturb its timing — about one line in a hundred arrives with a digit missing. The assembler guards against truncated and overlong lines, but a dropped digit inside a number is still a valid number, so `sscanf` accepts it and no guard fires. Structural checks catch structural damage; this is semantic, and it needs a range check or a hardware UART.

---

## Status

- Phase 0 — Mechanical validation ✅
- Phase 1 — Single-node POC: BMP280 + servo PWM ✅
- Phase 2 — nRF24 wireless link ✅ (retired — see postmortem)
- Phase 3 — Wired UART link, framed protocol ✅
- Phase 4 — Reactive control, OLED, timer-paced ✅
- Phase 5 — ESP8266 gateway: dashboard + JSON ✅ (2026-08-20)
- Phase 6 — Occupancy & logging: PIR, 24 h shutdown, FatFS CSV ✅ (2026-08-19)
- Installation — mount in the room and let it run ⬜

Every component in the inventory is operational; what's left is mechanical. Next up are lever-arm fabrication and mounting, a mutable setpoint with a receive path on USART3 so the dashboard can command rather than only observe, and a hardware UART to close the corruption above. Further out: an NTP-set RTC for wall-clock timestamps, weather-forecast input, the bathroom node, and a wireless revival if provenance modules ever turn up.

---

## Repo Structure

```
stm32-hvac-ecosystem/
├── hvac_ceiling_node/     # Ceiling node firmware (STM32F103C8T6)
├── hvac_control_node/     # Control node firmware (STM32F103C8T6)
└── hvac_esp_gateway/      # ESP8266 WiFi gateway (Arduino framework)
```

The two STM32 nodes are independent build targets with no shared source path, so the framing functions live in both — deliberately. A shared `frame.c` inside one project does nothing for the other, and one in each is two copies under a different name; a `common/` directory only earns its place if the shared surface grows past a pair of functions. The obligation is to keep the copies identical.

Drivers are adapted from [stm32-imu-logger](https://github.com/rohaanbrar-stack/stm32-imu-logger), with one exception worth noting: the SPI driver was **not** copied back, because this project's version drives CS high before configuring the pin and adds a trailing busy wait, making it a strict superset.

---

## Author

Rohaan Brar — embedded systems learning project, Purdue CompE.
