# NIC Out-of-Band Management Firmware

Firmware skeleton for NIC out-of-band / side-band management — the device-side firmware that communicates with the BMC for sensor monitoring, firmware updates, and platform management.

## Architecture

Layered architecture with three RTOS threads and modular service components:

```
HW Unit
  ↕
ISR Vector ──────────────────────────────────────────
  ↕                    ↕                    ↕
BMC IF            NIC Chip IF          Sensor IF
HAL+driver        HAL+driver           HAL+driver
  ↕                    ↕                    ↕
Protocol Hdlr     SPI Flash cmd        Sensor Protocol
(MCTP + PLDM)
  ↕                    ↕                    ↕
BMC Manager ←→    NIC Flash Manager    Per. Sensor Manager
  ↕ NIC Gen Mgr
  ↕
RTOS Execution (ESP-IDF FreeRTOS)
```

### Threads (by priority)
- **BMC Manager** (high) — PLDM command dispatch, liveness, NIC event handling
- **Sensor Manager** (medium) — periodic sensor polling, cache maintenance
- **Flash Manager** (low) — firmware update operations, long-running flash writes

### Key Design Decisions
- Thread boundaries driven by priority requirements, not code modularity
- MCTP/PLDM execute as function calls within BMC Manager thread context
- NIC General Manager (FRU, SEL, reset, version) is a code module, not a thread
- Sensor cache with mutex for cross-thread sensor data access
- Event flags + separate queues for BMC Manager multi-queue prioritization
- RTOS HAL abstracts ESP-IDF FreeRTOS for portability

## Build

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) v5.x.

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

## Project Structure

```
nic-ob-fw/
├── main/                       # Entry point only
│   └── main.c
├── components/
│   ├── common/                 # Shared types and definitions
│   ├── rtos_hal/               # RTOS abstraction (ESP-IDF FreeRTOS)
│   ├── bmc_hal/                # I2C slave driver — BMC interface
│   ├── sensor_hal/             # I2C master driver — sensor reads
│   ├── nic_chip_hal/           # SPI + GPIO — NIC ASIC interface
│   ├── isr_handlers/           # ISR registration and handlers
│   ├── protocol_hdlr/         # MCTP + PLDM (internals hidden)
│   ├── sensor_protocol/        # Sensor register abstraction (PMBus stub)
│   ├── spi_flash_cmd/          # JEDEC SPI flash command layer
│   ├── nic_chip_protocol/      # NIC ASIC communication (stub)
│   ├── bmc_mgr/                # BMC Manager thread
│   ├── nic_gen_mgr/            # FRU, SEL, reset, version (code module)
│   ├── nic_flash_mgr/          # Flash Manager thread
│   └── sensor_mgr/             # Periodic Sensor Manager thread
```

Each component exposes only its public header via `include/`. Internal headers live in `src/` and are invisible to other components. Dependencies are declared explicitly in each component's `CMakeLists.txt`.
