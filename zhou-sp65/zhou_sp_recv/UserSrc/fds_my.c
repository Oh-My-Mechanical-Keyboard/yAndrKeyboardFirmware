#include "common_cfg.h"

#include "fds_my.h"
#include "fds.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

// global
esb_fds_t esb_fds __attribute__((aligned(4))) = {.addr_0=USER_ESB_BASE_ADDR0, .addr_1=USER_ESB_BASE_ADDR1, .addr_perfix=USER_ESB_ADDR_PREFIX, .channel=USER_ESB_CHANNEL};

/* A record containing dummy configuration data. */
static fds_record_t const m_dummy_record =
    {
        .file_id = CONFIG_FILE,
        .key = CONFIG_REC_KEY,
        .data.p_data = &esb_fds,
        /* The length of a record is always expressed in 4-byte units (words). */
        .data.length_words = (sizeof(esb_fds) + 3) / sizeof(uint32_t),
};

/* Flag to check fds initialization. */
static bool volatile m_fds_initialized;

#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[1;36m"

#define NRF_LOG_CYAN(...) NRF_LOG_INFO(COLOR_CYAN __VA_ARGS__)
#define NRF_LOG_YELLOW(...) NRF_LOG_INFO(COLOR_YELLOW __VA_ARGS__)
#define NRF_LOG_GREEN(...) NRF_LOG_INFO(COLOR_GREEN __VA_ARGS__)

/* Array to map FDS events to strings. */
static char const *fds_evt_str[] =
    {
        "FDS_EVT_INIT",
        "FDS_EVT_WRITE",
        "FDS_EVT_UPDATE",
        "FDS_EVT_DEL_RECORD",
        "FDS_EVT_DEL_FILE",
        "FDS_EVT_GC",
};

const char *fds_err_str(ret_code_t ret) {
    /* Array to map FDS return values to strings. */
    static char const *err_str[] =
        {
            "FDS_ERR_OPERATION_TIMEOUT",
            "FDS_ERR_NOT_INITIALIZED",
            "FDS_ERR_UNALIGNED_ADDR",
            "FDS_ERR_INVALID_ARG",
            "FDS_ERR_NULL_ARG",
            "FDS_ERR_NO_OPEN_RECORDS",
            "FDS_ERR_NO_SPACE_IN_FLASH",
            "FDS_ERR_NO_SPACE_IN_QUEUES",
            "FDS_ERR_RECORD_TOO_LARGE",
            "FDS_ERR_NOT_FOUND",
            "FDS_ERR_NO_PAGES",
            "FDS_ERR_USER_LIMIT_REACHED",
            "FDS_ERR_CRC_CHECK_FAILED",
            "FDS_ERR_BUSY",
            "FDS_ERR_INTERNAL",
        };

    return err_str[ret - NRF_ERROR_FDS_ERR_BASE];
}

static void fds_evt_handler(fds_evt_t const *p_evt) {
    if (p_evt->result == NRF_SUCCESS) {
        NRF_LOG_GREEN("Event: %s received (NRF_SUCCESS)",
            fds_evt_str[p_evt->id]);
    } else {
        NRF_LOG_GREEN("Event: %s received (%s)",
            fds_evt_str[p_evt->id],
            fds_err_str(p_evt->result));
    }

    switch (p_evt->id) {
    case FDS_EVT_INIT:
        if (p_evt->result == NRF_SUCCESS) {
            m_fds_initialized = true;
        }
        break;

    case FDS_EVT_WRITE: {
        if (p_evt->result == NRF_SUCCESS) {
            NRF_LOG_INFO("Record ID:\t0x%04x", p_evt->write.record_id);
            NRF_LOG_INFO("File ID:\t0x%04x", p_evt->write.file_id);
            NRF_LOG_INFO("Record key:\t0x%04x", p_evt->write.record_key);
        }
    } break;

    case FDS_EVT_DEL_RECORD: {
        if (p_evt->result == NRF_SUCCESS) {
            NRF_LOG_INFO("Record ID:\t0x%04x", p_evt->del.record_id);
            NRF_LOG_INFO("File ID:\t0x%04x", p_evt->del.file_id);
            NRF_LOG_INFO("Record key:\t0x%04x", p_evt->del.record_key);
        }
    } break;

    default:
        break;
    }
}

/**@brief   Wait for fds to initialize. */
static void wait_for_fds_ready(void) {
    while (!m_fds_initialized) {
        __WFE();
    }
}

void fds_update_esb_data(void) {
    ret_code_t rc;
    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};
    NRF_LOG_INFO("ESB data Updating....");
    /* Write the updated record to flash. */
    rc = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);
    if (rc == NRF_SUCCESS) {
        rc = fds_record_update(&desc, &m_dummy_record);
        if ((rc != NRF_SUCCESS) && (rc == FDS_ERR_NO_SPACE_IN_FLASH)) {
            NRF_LOG_INFO("No space in flash, delete some records to update the config file.");
            rc = fds_gc();   // 垃圾回收，完成后自动清除标记
            NRF_LOG_INFO("fds_gc: %d", rc);
        } else {
            APP_ERROR_CHECK(rc);
        }    
    }
}

void fds_init_my(void) {
    ret_code_t rc;
    /* Register first to receive an event when initialization is complete. */
    (void)fds_register(fds_evt_handler);
    NRF_LOG_INFO("Initializing fds...");
    m_fds_initialized = false;
    rc = fds_init();
    APP_ERROR_CHECK(rc);
    /* Wait for fds to initialize. */
    wait_for_fds_ready();

    NRF_LOG_INFO("Reading flash usage statistics...");

    fds_stat_t stat = {0};

    rc = fds_stat(&stat);
    APP_ERROR_CHECK(rc);

    NRF_LOG_INFO("Found %d valid records.", stat.valid_records);
    NRF_LOG_INFO("Found %d dirty records (ready to be garbage collected).", stat.dirty_records);

    fds_record_desc_t desc = {0};
    fds_find_token_t tok = {0};

    rc = fds_record_find(CONFIG_FILE, CONFIG_REC_KEY, &desc, &tok);

    if (rc == NRF_SUCCESS) { // 从flash恢复
        /* A config file is in flash. Let's update it. */
        fds_flash_record_t config = {0};

        /* Open the record and read its contents. */
        rc = fds_record_open(&desc, &config);
        APP_ERROR_CHECK(rc);

        /* Copy the configuration from flash into m_dummy_cfg. */
        memcpy(&esb_fds, config.p_data, sizeof(esb_fds_t));

        NRF_LOG_INFO("ESB data found>>>>>>>>");
        NRF_LOG_INFO("channel %d.", esb_fds.channel);
        NRF_LOG_INFO("add0 [%02X %02X %02X %02X]", esb_fds.addr_0[0], esb_fds.addr_0[1], esb_fds.addr_0[2], esb_fds.addr_0[3]);
        NRF_LOG_INFO("add1 [%02X %02X %02X %02X]", esb_fds.addr_1[0], esb_fds.addr_1[1], esb_fds.addr_1[2], esb_fds.addr_1[3]);
        NRF_LOG_INFO("perfix_03 [%02X %02X %02X %02X].", esb_fds.addr_perfix[0], esb_fds.addr_perfix[1], esb_fds.addr_perfix[2], esb_fds.addr_perfix[3]);
        NRF_LOG_INFO("perfix_47 [%02X %02X %02X %02X].", esb_fds.addr_perfix[4], esb_fds.addr_perfix[5], esb_fds.addr_perfix[6], esb_fds.addr_perfix[7]);

        /* Close the record when done reading. */
        rc = fds_record_close(&desc);
        APP_ERROR_CHECK(rc);
    } else { // 新写入
        /* System config not found; write a new one. */
        NRF_LOG_INFO("Writing new esb data...");

        rc = fds_record_write(&desc, &m_dummy_record);
        NRF_LOG_INFO("Writing new esb data...%d", rc);
        if ((rc != NRF_SUCCESS) && (rc == FDS_ERR_NO_SPACE_IN_FLASH)) {
            NRF_LOG_INFO("No space in flash, run gc");
            rc = fds_gc();   // 垃圾回收，完成后自动清除标记
            NRF_LOG_INFO("fds_gc: %d", rc);
        } else {
            APP_ERROR_CHECK(rc);
        }
    }
}
