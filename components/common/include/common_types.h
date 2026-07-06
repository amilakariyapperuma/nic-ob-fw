/**
 * @file common_types.h
 * @brief Shared data types and definitions for NIC Out-of-Band FW
 */
#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*--------------------------------------------------------------------------
 * System-wide constants
 *------------------------------------------------------------------------*/
#define MAX_SENSORS             16
#define SENSOR_POLL_INTERVAL_MS 100
#define MCTP_MAX_PAYLOAD        64
#define PLDM_MAX_PAYLOAD        256
#define SMB_MAX_BLOCK_SIZE      32
#define FLASH_PAGE_SIZE         256
#define FLASH_SECTOR_SIZE       4096

#define QUEUE_DEPTH_BMC         8
#define QUEUE_DEPTH_NIC_GEN     4
#define QUEUE_DEPTH_SENSOR      4
#define QUEUE_DEPTH_FLASH       2

#define TASK_STACK_BMC          4096
#define TASK_STACK_SENSOR       2048
#define TASK_STACK_FLASH        4096

#define PRI_BMC_MGR             5
#define PRI_SENSOR_MGR          3
#define PRI_FLASH_MGR           1

/*--------------------------------------------------------------------------
 * Alignment macro — ESP32 Xtensa is 32-bit, 4-byte aligned
 *------------------------------------------------------------------------*/
#define ALIGNED4  __attribute__((aligned(4)))

/*--------------------------------------------------------------------------
 * Message type enumerations
 *------------------------------------------------------------------------*/
typedef enum {
    MSG_TYPE_SENSOR_READ = 0,
    MSG_TYPE_FW_UPDATE,
    MSG_TYPE_FRU_QUERY,
    MSG_TYPE_FW_VERSION,
    MSG_TYPE_RESET_CMD,
    MSG_TYPE_POWER_CTRL,
    MSG_TYPE_SEL_EVENT,
    MSG_TYPE_NIC_ASIC_EVENT,
    MSG_TYPE_LIVENESS,
    MSG_TYPE_MAX
} msg_type_t;

typedef enum {
    SENSOR_TYPE_TEMP = 0,
    SENSOR_TYPE_VOLTAGE,
    SENSOR_TYPE_CURRENT,
    SENSOR_TYPE_POWER,
    SENSOR_TYPE_FAN_RPM,
    SENSOR_TYPE_MAX
} sensor_type_t;

typedef enum {
    SENSOR_STATE_OK = 0,
    SENSOR_STATE_WARNING,
    SENSOR_STATE_CRITICAL,
    SENSOR_STATE_FAILED,
    SENSOR_STATE_UNAVAILABLE
} sensor_state_t;

typedef enum {
    SENSOR_UNIT_CELSIUS = 0,
    SENSOR_UNIT_VOLTS,
    SENSOR_UNIT_AMPS,
    SENSOR_UNIT_WATTS,
    SENSOR_UNIT_RPM
} sensor_unit_t;

typedef enum {
    FLASH_CMD_UPDATE_START = 0,
    FLASH_CMD_UPDATE_ABORT,
    FLASH_CMD_VERIFY,
    FLASH_CMD_ACTIVATE
} flash_cmd_type_t;

typedef enum {
    NIC_EVENT_HW_ERROR = 0,
    NIC_EVENT_LINK_CHANGE,
    NIC_EVENT_THRESHOLD_ALERT,
    NIC_EVENT_BOOT_COMPLETE
} nic_event_type_t;

/*--------------------------------------------------------------------------
 * Core data structures — all 4-byte aligned to avoid unaligned access
 *------------------------------------------------------------------------*/

/** Generic message header — prepended to all inter-module messages */
typedef struct ALIGNED4 {
    uint8_t  msg_type;
    uint8_t  flags;
    uint16_t payload_length;
    uint32_t crc;
} msg_header_t;

/** Sensor record — cached sensor data */
typedef struct ALIGNED4 {
    uint16_t      sensor_id;
    uint8_t       sensor_type;      /* sensor_type_t  */
    uint8_t       state;            /* sensor_state_t */
    int16_t       raw_value;
    int16_t       scaled_value;     /* fixed-point after calibration */
    uint8_t       unit;             /* sensor_unit_t  */
    uint8_t       reserved[3];
    uint32_t      timestamp_ms;
} sensor_record_t;

/** Sensor cache table */
typedef struct ALIGNED4 {
    sensor_record_t records[MAX_SENSORS];
    uint8_t         count;
    uint8_t         reserved[3];
} sensor_cache_t;

/** Flash manager command — posted from BMC Mgr to Flash Mgr queue */
typedef struct ALIGNED4 {
    uint8_t  cmd_type;              /* flash_cmd_type_t */
    uint8_t  target_partition;
    uint16_t reserved;
    uint32_t image_offset;
    uint32_t image_size;
} flash_cmd_t;

/** NIC General manager message — posted from NIC ASIC ISR */
typedef struct ALIGNED4 {
    uint8_t  event_type;            /* nic_event_type_t */
    uint8_t  reserved[3];
    uint32_t event_data;
    uint32_t timestamp_ms;
} nic_event_msg_t;

/** BMC receive queue message — raw bytes from I2C ISR */
typedef struct ALIGNED4 {
    uint8_t  data[SMB_MAX_BLOCK_SIZE];
    uint16_t length;
    uint16_t reserved;
} bmc_rx_msg_t;

/** MCTP packet structure */
typedef struct ALIGNED4 {
    uint8_t  hdr_version;
    uint8_t  dest_eid;
    uint8_t  src_eid;
    uint8_t  flags_seq;             /* SOM | EOM | seq_num | tag */
    uint16_t payload_len;
    uint16_t reserved;
    uint8_t  payload[MCTP_MAX_PAYLOAD];
} mctp_packet_t;

/** PLDM message structure */
typedef struct ALIGNED4 {
    uint8_t  instance_id;
    uint8_t  pldm_type;             /* 2=sensor, 5=fw_update */
    uint8_t  command;
    uint8_t  completion_code;
    uint16_t payload_len;
    uint16_t reserved;
    uint8_t  payload[PLDM_MAX_PAYLOAD];
} pldm_msg_t;

/** FRU inventory record */
typedef struct ALIGNED4 {
    uint8_t  board_serial[16];
    uint8_t  part_number[16];
    uint8_t  mac_addr[8];           /* 6 bytes + 2 padding */
    uint8_t  hw_revision;
    uint8_t  reserved[3];
} fru_record_t;

/*--------------------------------------------------------------------------
 * CRC utility — declared inline, defined once in common module
 *------------------------------------------------------------------------*/
static inline uint32_t crc32_byte(uint32_t crc, uint8_t byte)
{
    crc ^= (uint32_t)byte;
    /* Unrolled 8 iterations of CRC32 bit processing */
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    crc = (crc >> 1) ^ (0xEDB88320U & -(crc & 1U));
    return crc;
}

static inline uint32_t crc32_compute(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    size_t i;

    /* Process 4 bytes per iteration where possible */
    size_t unrolled = len & ~3U;
    for (i = 0; i < unrolled; i += 4) {
        crc = crc32_byte(crc, data[i]);
        crc = crc32_byte(crc, data[i + 1]);
        crc = crc32_byte(crc, data[i + 2]);
        crc = crc32_byte(crc, data[i + 3]);
    }
    /* Remaining bytes */
    for (; i < len; i++) {
        crc = crc32_byte(crc, data[i]);
    }
    return crc ^ 0xFFFFFFFFU;
}

#endif /* COMMON_TYPES_H */
