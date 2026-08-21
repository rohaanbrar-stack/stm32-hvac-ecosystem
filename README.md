# STM32 Bare-Metal HVAC Ecosystem

![Language](https://img.shields.io/badge/language-C-blue) ![Platform](https://img.shields.io/badge/platform-STM32F103-green) ![HAL](https://img.shields.io/badge/HAL-none-red)

A two-node HVAC automation system built on bare-metal STM32F103s. A ceiling node reports duct temperature over a framed UART link; a control node compares it against room temperature, drives the vent servos, logs timestamped records to microSD, watches the room for occupancy, and streams live telemetry to an ESP8266 that serves a dashboard over WiFi. All STM32 firmware is register-level C against RM0008 — no HAL, no LL.

**This is a validated bench system, not a house installation.** Everything below runs on a desk and is demonstrable from a laptop on the same network. Mounting it in the room is deliberate future work, not an omission.

---

## Features

- Register-level drivers for I2C, SPI, three USARTs, TIM2, EXTI, and clock configuration — no HAL, no LL
- Two-node architecture over a self-framing wired UART protocol with checksums and acknowledged commands
- Reactive heat-and-cool control: a vent only opens when the room needs it **and** the duct air can actually help
- Three USARTs running simultaneously on one F103 — debug, inter-node link, and WiFi bridge — with bounded receives so a silent peer can't hang the node
- SG90 servo control via TIM2 hardware PWM, 0.5–2.5 ms pulse range
- SSD1306 OLED showing room, duct, setpoint, vent state, and a `WON'T HELP` indicator
- MicroSD logging through FatFS with a custom diskio layer — CSV records of every control decision
- PIR occupancy sensing on EXTI, with a 24-hour no-motion shutdown
- ESP8266 WiFi gateway serving a live auto-refreshing dashboard and a JSON endpoint
- 1 Hz control loop paced by a hardware timer, not a counted delay
- Retired-but-exonerated nRF24 driver kept in-tree after a two-week falsification postmortem

---

## Problem

Attic bedroom accumulates heat year-round. Two adjustable ceiling vents are the only physical control. This system automates them: a sensor node at the vents reports duct temperature to a central control node, which drives servos to open and close the vents based on whether the duct air is actually cooler than the room.

---

## Architecture

Two STM32F103C8T6 (Blue Pill) nodes connected over a wired UART link. The control node relays telemetry to an ESP8266 gateway for WiFi access.

| Node | Location | Controls | Sensors |
|---|---|---|---|
| Ceiling | Bedroom ceiling vents | 2x SG90 servo | BMP280 (duct temp) |
| Control | Near door | Hub, SD logging, OLED, dashboard bridge | BMP280 (room ambient), PIR |

**Data path:** Ceiling STM32 → UART → Control STM32 → UART → ESP8266 → WiFi → browser

A third node (bathroom vent) is deferred — it replicates the Ceiling node with one servo and slots into the existing hub topology whenever it's built.

---

## Hardware

**Control node — STM32F103C8T6**

| Component | Interface | Pins |
|---|---|---|
| BMP280 (room temp) | I2C1 | PB6 (SCL), PB7 (SDA) |
| SSD1306 OLED | I2C1 | PB6 (SCL), PB7 (SDA) |
| MicroSD (ADA254) | SPI1 | PA5 (SCK), PA6 (MISO), PA7 (MOSI), PA4 (CS) |
| HC-SR501 PIR | GPIO + EXTI0 | PB0 |
| Ceiling node link | USART2 | PA2 (TX), PA3 (RX) |
| ESP8266 bridge | USART3 | PB10 (TX), PB11 (RX) |
| CP2102 debug UART | USART1 | PA9 (TX), PA10 (RX) |
| ST-Link V2 | SWD | SWDIO, SWCLK, GND, 3.3V |

**Ceiling node — STM32F103C8T6**

| Component | Interface | Pins |
|---|---|---|
| BMP280 (duct temp) | I2C1 | PB6 (SCL), PB7 (SDA) |
| SG90 servos ×2 | TIM2 CH1/CH2 PWM | PA0, PA1 |
| Control node link | USART2 | PA2 (TX), PA3 (RX) |
| CP2102 debug UART | USART1 | PA9 (TX), PA10 (RX) |

**Gateway — ESP8266 NodeMCU (LoLin, FT232R USB bridge)**

| Function | Interface | Pins |
|---|---|---|
| Control node link | SoftwareSerial, 9600 8-N-1 | D5 (RX), D6 (TX) |

Full parts list: 2x STM32F103C8T6, 3x SG90 servo, 2x BMP280, 1x ESP8266 NodeMCU, 1x SSD1306 OLED, 1x Adafruit MicroSD breakout, 1x HC-SR501 PIR, ST-Link V2, CP2102 USB-TTL. ~~nRF24L01+ modules~~ — retired, see postmortem.

---

## Wire Protocol

Self-framing bytes over UART: `[0xAA][type][payload][checksum]`. Three types — telemetry, servo command, ACK. Telemetry is fire-and-forget, commands are acknowledged. Proven end-to-end 2026-07-27.

Every receive is bounded by a byte-timeout rather than blocking forever, so a node whose peer has gone quiet keeps running its own loop instead of hanging. That was added deliberately before the WiFi bridge landed, and it is the reason three USARTs coexist on the control node without deadlocking each other.

---

## Reactive Control

The control node compares its own room temperature against the duct temperature streamed from the ceiling, driving the vents through a four-state machine: `CLOSED`, `COOLING`, `HEATING`, `AWAY`. Opening requires **two** conditions — the room outside a deadband around the 78 °F setpoint, *and* the duct air actually helping:

| Room vs setpoint | Duct vs room | Occupancy | Action |
|---|---|---|---|
| Above deadband | colder by margin | occupied | **open** — cooling |
| Above deadband | not colder | occupied | closed — *vents won't help* |
| Below deadband | warmer by margin | occupied | **open** — heating |
| Below deadband | not warmer | occupied | closed — *vents won't help* |
| Within deadband | — | occupied | closed — satisfied |
| — | — | no motion for 24 h | closed — `AWAY` |

A hot room with hot duct air means the furnace is running, so the vent stays shut rather than blowing it in. Setpoint is 25.56 °C (78.0 °F) with a 0.85 °C deadband and a 1.10 °C helping margin, all as compile-time constants.

The occupancy rule is a **24-hour** shutdown, not moment-to-moment gating, and that choice made the logic smaller rather than larger. At a ten-minute horizon there's a real design tension — closing a vent fights a room you're about to walk back into — but nobody re-enters seconds after a full day's absence, so closing is unambiguously correct and the whole question dissolves into a single override.

---

## How It Works

Both nodes run a 1 Hz loop paced by TIM2. The ceiling node samples its BMP280 and sends a telemetry frame; the control node samples its own BMP280, receives that frame with a bounded timeout, and runs the decision table above. When the state changes it sends a servo command frame and waits for the ACK. The OLED redraws every cycle, and a CSV record goes to the SD card through FatFS.

The PIR sits on PB0 with an EXTI interrupt that stamps `last_motion` from the timer tick. The main loop compares that against a 24-hour timeout; crossing it forces the vents closed and the state to `AWAY`, and any motion afterward releases it.

`log.csv` holds one record per control cycle:

```
timestamp, temp, tempVent, state, wontHelp, last_motion
```

Temperatures are hundredths of a degree Celsius throughout the firmware, converted to Fahrenheit only where they're displayed.

Every cycle the control node also writes one line to USART3:

```
T <temp>, <tempVent>, <state>, <setpoint>\n
```

The ESP8266 assembles that stream into complete lines, parses each into a struct, and holds the most recent good reading. It serves two routes: `/` returns a self-contained HTML page, and `/data` returns the current reading as JSON.

```json
{"room":2891,"duct":2962,"state":0,"set":2556}
```

The page fetches `/data` every two seconds and redraws itself, so the browser does the polling and the ESP stays a passive responder. Unit conversion happens in the browser — raw values on the wire, formatting where the presentation is. Before the first complete line arrives, `/data` answers **503** rather than serving zeros that would render as a plausible 32.0 °F.

---

## Quick Start

**Requirements:** STM32CubeIDE, ST-Link V2, Arduino IDE with the ESP8266 board package, a FAT32-formatted microSD card (2 GB or smaller — see below), and a WiFi network.

1. Clone the repo. Open `hvac_ceiling_node/` and `hvac_control_node/` as separate STM32CubeIDE projects.
2. Wire both nodes per the tables above. The inter-node link is TX→RX crossed on USART2 plus a shared ground.
3. Flash each node over SWD with ST-Link.
4. Format the SD card as FAT32 and insert it before powering the control node — the file is opened once at startup.
5. In `hvac_esp_gateway/`, copy `secrets_example.h` to `secrets.h` and fill in your WiFi SSID and password. `secrets.h` is gitignored and never leaves your machine.
6. Flash `hvac_esp_gateway.ino` to the NodeMCU. Open the serial monitor at 115200 and note the IP it prints after joining the network. Close the serial monitor before any re-flash — it holds the COM port.
7. Open that IP in a browser on the same network. Room, duct, setpoint and vent state update on their own every two seconds.

⚠️ **The SD driver is SDSC-only.** `SD_ReadBlock` and `SD_WriteBlock` send byte addresses, and `SD_Init` never reads the OCR to learn otherwise, so cards larger than 2 GB will not address correctly. The fix is small — send CMD58 after ACMD41, read the CCS bit, and make the block multiply conditional — and it was deferred on schedule, not difficulty.

---

## Why Wired? — The Wireless Postmortem

The wireless link worked once — 2026-06-29, live packets with auto-ack, Phase 2 complete — then died and never returned. Two weeks of debugging ended with the firmware proven correct and the transport cut anyway:

| Hypothesis | Test | Result |
|---|---|---|
| Driver/firmware bug | Datasheet audit; diff vs reference driver; firmware-swap bisect | **Exonerated** |
| Config not landing | Spaced single-register readbacks, both nodes | All values landed |
| TX power margin | Swept -18dBm → 0dBm | No change |
| Data rate / crystal tolerance | 2Mbps, 1Mbps, 250kbps | No change |
| Dead module batch | Two module types, two orders, incl. a never-powered pair | Identical failure |
| Anything radiating? | CONT_WAVE carrier, 0dBm at 1m, receiver polling RPD | **RPD:0 — no RF, ever** |

SI24R1-class counterfeits whose SPI reads lie while writes land — a flawless protocol engine driving dead RF front-ends. The radio was transport, not the point, so it was cut for a 3-wire UART link. The driver stays in the repo, exonerated, ready if provenance modules ever replace the clones.

---

## Design Notes

The postmortem above is the project's headline lesson, but it isn't the only one, and the follow-up is less flattering. Months later the ESP8266 wouldn't enumerate, and the same falsification method got pointed at it: a ledger of candidates, each tested and eliminated. It concluded the board was dead. The board was fine — the silkscreen had been read as a CH340G, every test searched for a CH340G, and the FT232R sitting on the correct COM port was recorded in the ledger's own notes and dismissed as a phantom. The method was rigorous about the candidates it chose and never questioned the assumption that chose them. Being disciplined downstream of a bad premise just makes the wrong answer arrive with better evidence.

The other recurring theme is that a measurement almost always exists and is almost always cheaper than the reasoning it replaces. The 1 Hz loop was once a counted busy-wait labelled *one-second delay*; the first timestamp against a real clock measured it at 5.84 seconds. The vent logic looked correct on paper until the log's own timestamps caught a 210 ms outlier that terminal sampling had missed. And a serial port that refused to open twice in one evening was diagnosed both times by asking the operating system which process held the handle, rather than by theorising about drivers.

One known defect ships with this build, deliberately. The ESP's link runs on `SoftwareSerial`, which is bit-banged, and WiFi interrupts occasionally disturb its timing — roughly one line in a hundred arrives with a digit missing. The line assembler guards against truncation and against overlong lines, but a dropped digit inside a number is still a valid number, so `sscanf` accepts it and the guard never fires. Structural checks catch structural damage; this is semantic damage, and it needs a range check or a hardware UART. It's logged rather than hidden because a dashboard that flickers a wrong value once a minute is worth understanding before it's worth fixing.

---

## Status

- Phase 0 — Mechanical validation ✅
- Phase 1 — Single-node POC (BMP280 + servo PWM on Ceiling node) ✅
- Phase 2 — nRF24 wireless 2-node link (retired — see postmortem) ✅
- Phase 3 — Wired UART inter-node link (framed protocol: telemetry + command + ACK) ✅
- Phase 4 — Reactive control logic (heat/cool state machine, OLED indicator, timer-paced) ✅
- Phase 5 — ESP8266 gateway (WiFi dashboard + JSON endpoint) ✅ (2026-08-20)
- Phase 6 — Occupancy & logging (PIR on EXTI, 24 h shutdown, FatFS CSV records) ✅ (2026-08-19)
- Installation — mount in the room and let it run ⬜

Every component in the hardware inventory is operational. What remains is mechanical: lever arms, mounting, and a long unattended run in the room it was built for.

---

## Later Plans

- Physical installation — lever-arm fabrication and mounting at the ceiling vents
- Setpoint control from the dashboard — currently a compile-time constant, so this needs a mutable setpoint and a receive path on USART3
- Hardware UART or a range check on the ESP link to close the ~1% corruption noted above
- NTP-set RTC over WiFi so log timestamps are wall-clock rather than tick-relative
- Weather forecast integration — open or close vents ahead of outdoor conditions
- Learned thermostat model predicting optimal vent positions and AC setpoints
- Mobile app for manual override and live monitoring
- Bathroom vent node (Ceiling node replica, 1 servo)
- Wireless revival with provenance nRF24 modules — driver is ready and waiting

---

## Repo Structure

```
stm32-hvac-ecosystem/
├── hvac_ceiling_node/     # Ceiling node firmware (STM32F103C8T6)
├── hvac_control_node/     # Control node firmware (STM32F103C8T6)
└── hvac_esp_gateway/      # ESP8266 WiFi gateway (Arduino framework)
```

The two STM32 nodes are independent build targets with no shared source path, so the framing functions live in both — deliberately, not as debt. A shared `frame.c` inside one project does nothing for the other, and one in each is two copies under a different name; a real `common/` directory only earns its place if the shared surface grows well beyond a pair of functions. The obligation is simply to keep the two copies identical.

The I2C, USART, clock, SPI, SSD1306, SD and FatFS drivers are adapted from [stm32-imu-logger](https://github.com/rohaanbrar-stack/stm32-imu-logger). The SPI driver was **not** copied back wholesale — the control node's version drives CS high before configuring the pin and adds a trailing busy wait, making it a strict superset. Each node's `nRF24.c` is the retired-but-exonerated wireless driver (see postmortem).

---

## Author

Rohaan Brar — embedded systems learning project, Purdue CompE.
