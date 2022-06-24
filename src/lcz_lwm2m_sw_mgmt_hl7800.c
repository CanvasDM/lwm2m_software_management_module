/**
 * @file lcz_lwm2m_sw_mgmt_hl7800.c
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_lwm2m_sw_mgmt_hl7800, CONFIG_LCZ_LWM2M_SW_MANAGEMENT_LOG_LEVEL);

#include <zephyr.h>
#include <init.h>
#include <drivers/modem/hl7800.h>
#include "lwm2m_engine.h"
#include "lcz_lwm2m.h"
#include "lcz_lwm2m_sw_mgmt.h"
#include "file_system_utilities.h"

/**************************************************************************************************/
/* Local Constant, Macro and Type Definitions                                                     */
/**************************************************************************************************/
#define UPDATE_FILE_PATH CONFIG_FSU_MOUNT_POINT "/" CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_FILE_NAME

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static int sw_mgmt_event(lcz_lwm2m_sw_mgmt_event_t event);
static void *sw_mgmt_read_ver_cb(void);
static int sw_mgmt_download_data_cb(uint8_t *data, uint16_t data_len, bool last_block,
				    size_t total_size);
static void hl7800_event_cb(enum mdm_hl7800_event event, void *event_data);
static int lcz_lwm2m_sw_mgmt_hl780_init(const struct device *device);
static void start_fw_update_work_cb(struct k_work *work);
static int delete_update_file(void);

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static struct lcz_lwm2m_sw_mgmt_event_callback_agent event_agent;
static int bytes_downloaded;
static int update_file_size;
static struct mdm_hl7800_callback_agent hl7800_evt_agent;
static K_WORK_DELAYABLE_DEFINE(start_fw_update_work, start_fw_update_work_cb);

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static int delete_update_file(void)
{
	int ret;

	ret = fsu_get_file_size_abs(UPDATE_FILE_PATH);
	if (ret > 0) {
		ret = fsu_delete_abs(UPDATE_FILE_PATH);
	} else {
		ret = 0;
	}
	return ret;
}

static int sw_mgmt_event(lcz_lwm2m_sw_mgmt_event_t event)
{
	int ret;
	LOG_DBG("event %d", event);
	switch (event) {
	case LCZ_LWM2M_SW_MGMT_EVENT_INSTALL:
		k_work_reschedule(&start_fw_update_work,
				  K_SECONDS(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_INSTALL_DELAY_SECONDS));
		ret = 0;
		break;
	case LCZ_LWM2M_SW_MGMT_EVENT_UNINSTALL:
		/* Uninstall event used to reset state machine to allow for another install/update.
		 * Return 0 for the callback to allow the uninstall execution to continue without error
		 * and let the software management object state machine to reset its state properly.
		 */
		ret = 0;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}

static void start_fw_update_work_cb(struct k_work *work)
{
	int ret;

	ARG_UNUSED(work);

	ret = mdm_hl7800_update_fw(UPDATE_FILE_PATH);
	if (ret < 0) {
		lwm2m_swmgmt_install_completed(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_OBJ_INST, ret);
	}
}

static void *sw_mgmt_read_ver_cb(void)
{
	return (void *)mdm_hl7800_get_fw_version();
}

static int sw_mgmt_download_data_cb(uint8_t *data, uint16_t data_len, bool last_block,
				    size_t total_size)
{
	int ret;

	bytes_downloaded += data_len;
	if (bytes_downloaded > total_size) {
		/* Starting a new download */
		bytes_downloaded = data_len;
		ret = delete_update_file();
		if (ret < 0) {
			LOG_ERR("Could not delete file [%d]", ret);
			goto exit;
		}
	}

	ret = fsu_append(CONFIG_FSU_MOUNT_POINT, CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_FILE_NAME,
			 (void *)data, data_len);
	if (ret < 0) {
		LOG_ERR("Could not write file [%d]", ret);
		goto exit;
	}

	LOG_INF("Download %d/%d (%d%%)", bytes_downloaded, total_size,
		bytes_downloaded * 100 / total_size);

	if (last_block) {
		update_file_size = bytes_downloaded;
		bytes_downloaded = 0;
	}

exit:
	return ret;
}

static void hl7800_event_cb(enum mdm_hl7800_event event, void *event_data)
{
	uint8_t fota_state;

	switch (event) {
	case HL7800_EVENT_FOTA_STATE:
		fota_state = *(uint8_t *)event_data;
		if (fota_state == HL7800_FOTA_COMPLETE) {
			lwm2m_swmgmt_install_completed(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_OBJ_INST, 0);
			(void)delete_update_file();
			bytes_downloaded = 0;
			LOG_INF("HL7800 firmware update complete");
		} else if (fota_state == HL7800_FOTA_FILE_ERROR) {
			lwm2m_swmgmt_install_completed(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_OBJ_INST,
						       -EIO);
		} else if (fota_state == HL7800_FOTA_INSTALL) {
			LOG_INF("Installing HL7800 firmware");
		}
		break;
	case HL7800_EVENT_FOTA_COUNT:
		LOG_INF("Firmware write %d/%d (%d%%)", *(uint32_t *)event_data, update_file_size,
			*(uint32_t *)event_data * 100 / update_file_size);
		break;
	default:
		break;
	}
}

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
SYS_INIT(lcz_lwm2m_sw_mgmt_hl780_init, APPLICATION, CONFIG_LCZ_LWM2M_SW_MGMT_INIT_PRIORITY);

/**************************************************************************************************/
/* SYS INIT                                                                                       */
/**************************************************************************************************/
static int lcz_lwm2m_sw_mgmt_hl780_init(const struct device *device)
{
	int ret;

	ARG_UNUSED(device);

	hl7800_evt_agent.event_callback = hl7800_event_cb;
	mdm_hl7800_register_event_callback(&hl7800_evt_agent);

	bytes_downloaded = 0;
	event_agent.event_callback = sw_mgmt_event;
	event_agent.read_ver_callback = sw_mgmt_read_ver_cb;
	event_agent.download_data_callback = sw_mgmt_download_data_cb;
	ret = lcz_lwm2m_sw_mgmt_create_inst(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_OBJ_INST, &event_agent);
	if (ret < 0) {
		LOG_ERR("Create obj [%d]", ret);
		goto exit;
	}

	ret = lcz_lwm2m_sw_mgmt_set_pkg_name(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_OBJ_INST,
					     CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_PKG_NAME);
	if (ret < 0) {
		LOG_ERR("Set HL7800 sw mgmt pkg name [%d]", ret);
		goto exit;
	}

	/* HL7800 firmware is always active. Activate, Deactivate, and Uninstall are not allowed */
	ret = lcz_lwm2m_sw_mgmt_set_activate_state(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800_OBJ_INST, true);
	if (ret < 0) {
		LOG_ERR("Set HL7800 sw mgmt active [%d]", ret);
		goto exit;
	}

	/* Delete the download file if it exists */
	(void)delete_update_file();

	LOG_DBG("LwM2M software management HL7800 initialized");
exit:
	return ret;
}
