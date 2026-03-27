#ifndef DRV_ERRORS_H
#define DRV_ERRORS_H

/**
 * @file drv_errors.h
 * @brief Common driver error definitions for embedded drivers.
 *
 * This file defines a unified error system for all drivers:
 * - I2C, SPI, UART
 * - Sensor drivers (MPU, HMC, etc.)
 *
 * Design goals:
 * - minimal footprint
 * - structured error ranges
 * - easy debugging
 * - no dynamic memory
 * - optional string support
 */

#include <stdint.h>

#ifndef DRV_ERR_ENABLE_STRINGS
#define DRV_ERR_ENABLE_STRINGS 1
#endif
/* ========================================================================= */
/* ERROR TYPE                                                                */
/* ========================================================================= */

/**
 * @brief Unified driver error type.
 *
 * Error values are grouped by category:
 * - 0x00       : OK
 * - 0x01–0x1F  : Generic/API errors
 * - 0x20–0x3F  : Bus/Hardware errors
 * - 0x40–0x5F  : Register/Protocol errors
 * - 0x60–0x7F  : Device/Data errors
 * - 0x80–0x9F  : Configuration/State errors
 */
typedef enum {

    /* ========================= */
    /* === SUCCESS ============ */
    /* ========================= */

    DRV_OK = 0x00,                /**< Operation successful */

    /* ========================= */
    /* === GENERIC ============ */
    /* ========================= */

    DRV_ERR_NULL       = 0x01,    /**< Null pointer */
    DRV_ERR_ARG        = 0x02,    /**< Invalid argument */
    DRV_ERR_LEN        = 0x03,    /**< Invalid length */
    DRV_ERR_STATE      = 0x04,    /**< Invalid state */
    DRV_ERR_NOT_INIT   = 0x05,    /**< Not initialized */
    DRV_ERR_UNSUPPORTED= 0x06,    /**< Unsupported operation */

    /* ========================= */
    /* === BUS / HW =========== */
    /* ========================= */

    DRV_ERR_BUSY       = 0x20,    /**< Bus or peripheral busy */
    DRV_ERR_TIMEOUT    = 0x21,    /**< Timeout occurred */
    DRV_ERR_SDA_STUCK  = 0x22,    /**< SDA line stuck low */
    DRV_ERR_SCL_STUCK  = 0x23,    /**< SCL line stuck low */
    DRV_ERR_NACK       = 0x24,    /**< No acknowledge from slave */
    DRV_ERR_ARB_LOST   = 0x25,    /**< Arbitration lost */
    DRV_ERR_ABORT      = 0x26,    /**< Transfer aborted */
    DRV_ERR_HW         = 0x27,    /**< General hardware error */

    /* ========================= */
    /* === REGISTER / PROTO ==== */
    /* ========================= */

    DRV_ERR_REG        = 0x40,    /**< Invalid register */
    DRV_ERR_REG_READ   = 0x41,    /**< Register read failed */
    DRV_ERR_REG_WRITE  = 0x42,    /**< Register write failed */
    DRV_ERR_VERIFY     = 0x43,    /**< Register verify mismatch */
    DRV_ERR_PROTOCOL   = 0x44,    /**< Protocol/data format error */

    /* ========================= */
    /* === DEVICE / DATA ======= */
    /* ========================= */

    DRV_ERR_NO_DEVICE  = 0x60,    /**< Device not responding */
    DRV_ERR_ID         = 0x61,    /**< Device ID mismatch */
    DRV_ERR_DATA       = 0x62,    /**< Invalid data */
    DRV_ERR_DATA_RDY   = 0x63,    /**< Data not ready */
    DRV_ERR_OVERFLOW   = 0x64,    /**< Sensor overflow */
    DRV_ERR_LOCKED     = 0x65,    /**< Data registers locked */
    DRV_ERR_CALIB      = 0x66,    /**< Calibration failed */

    /* ========================= */
    /* === CONFIG / STATE ====== */
    /* ========================= */

    DRV_ERR_CFG        = 0x80,    /**< Configuration failed */
    DRV_ERR_MODE       = 0x81,    /**< Mode change failed */
    DRV_ERR_RESET      = 0x82     /**< Reset failed */

} drv_err_t;

/* ========================================================================= */
/* HELPER MACROS                                                             */
/* ========================================================================= */

/**
 * @brief Check if operation was successful.
 */
#define DRV_OKAY(x)      ((x) == DRV_OK)

/**
 * @brief Check if operation failed.
 */
#define DRV_FAILED(x)    ((x) != DRV_OK)

/**
 * @brief Extract error class (high nibble).
 */
#define DRV_ERR_CLASS(x) ((x) & 0xE0)

/**
 * @brief Check if error is bus-related.
 */
#define DRV_IS_BUS_ERR(x) (((x) & 0xE0) == 0x20)

/**
 * @brief Check if error is device-related.
 */
#define DRV_IS_DEV_ERR(x) (((x) & 0xE0) == 0x60)

/* ========================================================================= */
/* OPTIONAL STRING SUPPORT                                                   */
/* ========================================================================= */

#ifdef DRV_ERR_ENABLE_STRINGS

/**
 * @brief Convert error code to string.
 *
 * Note: increases flash usage.
 */
static inline const char* drv_err_str(drv_err_t err)
{
    switch (err) {
        case DRV_OK: return "OK";

        case DRV_ERR_NULL: return "NULL";
        case DRV_ERR_ARG: return "INVALID_ARG";
        case DRV_ERR_LEN: return "INVALID_LEN";
        case DRV_ERR_STATE: return "INVALID_STATE";
        case DRV_ERR_NOT_INIT: return "NOT_INIT";
        case DRV_ERR_UNSUPPORTED: return "UNSUPPORTED";

        case DRV_ERR_BUSY: return "BUSY";
        case DRV_ERR_TIMEOUT: return "TIMEOUT";
        case DRV_ERR_SDA_STUCK: return "SDA_STUCK";
        case DRV_ERR_SCL_STUCK: return "SCL_STUCK";
        case DRV_ERR_NACK: return "NACK";
        case DRV_ERR_ARB_LOST: return "ARB_LOST";
        case DRV_ERR_ABORT: return "ABORT";
        case DRV_ERR_HW: return "HW_ERR";

        case DRV_ERR_REG: return "REG";
        case DRV_ERR_REG_READ: return "REG_READ";
        case DRV_ERR_REG_WRITE: return "REG_WRITE";
        case DRV_ERR_VERIFY: return "VERIFY";
        case DRV_ERR_PROTOCOL: return "PROTOCOL";

        case DRV_ERR_NO_DEVICE: return "NO_DEVICE";
        case DRV_ERR_ID: return "ID_MISMATCH";
        case DRV_ERR_DATA: return "DATA_ERR";
        case DRV_ERR_DATA_RDY: return "DATA_NOT_READY";
        case DRV_ERR_OVERFLOW: return "OVERFLOW";
        case DRV_ERR_LOCKED: return "LOCKED";
        case DRV_ERR_CALIB: return "CALIB_FAIL";

        case DRV_ERR_CFG: return "CFG_FAIL";
        case DRV_ERR_MODE: return "MODE_FAIL";
        case DRV_ERR_RESET: return "RESET_FAIL";

        default: return "UNKNOWN";
    }
}

#endif

#endif /* DRV_ERRORS_H */
/* invalid pointer / arg */
// LOG_E(ANSI_COLOR_FN "func()" ANSI_RESET ": dev = NULL");
// LOG_E(ANSI_COLOR_FN "func()" ANSI_RESET ": invalid len = %u", len);
//
// /* register write */
// LOG_E(ANSI_COLOR_FN "func()" ANSI_RESET
//       ": failed to write reg 0x%02X with value 0x%02X",
//       reg, value);
//
// /* register read */
// LOG_E(ANSI_COLOR_FN "func()" ANSI_RESET
//       ": failed to read reg 0x%02X (%u bytes)",
//       reg, len);
//
// /* warning for invalid measurement */
// LOG_W(ANSI_COLOR_FN "func()" ANSI_RESET
//       ": overflow detected raw=(%d,%d,%d), keeping last valid values",
//       x, y, z);
//
// /* info */
// LOG_I(ANSI_COLOR_FN "func()" ANSI_RESET
//       ": initialized SDA:%d SCL:%d addr=0x%02X",
//       sda, scl, addr);
