/**
 * @file lcz_lwm2m_sw_mgmt.h
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

#ifndef __LCZ_LWM2M_SW_MGMT_H__
#define __LCZ_LWM2M_SW_MGMT_H__

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <zephyr.h>
#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************/
/* Global Constants, Macros and Type Definitions                                                  */
/**************************************************************************************************/
typedef enum lcz_lwm2m_sw_mgmt_event {
	LCZ_LWM2M_SW_MGMT_EVENT_ACTIVATE = 0,
	LCZ_LWM2M_SW_MGMT_EVENT_DEACTIVATE,
	LCZ_LWM2M_SW_MGMT_EVENT_INSTALL,
	LCZ_LWM2M_SW_MGMT_EVENT_UNINSTALL,
} lcz_lwm2m_sw_mgmt_event_t;

typedef int (*lcz_lwm2m_sw_mgmt_event_cb_t)(lcz_lwm2m_sw_mgmt_event_t event);
typedef void *(*lcz_lwm2m_sw_mgmt_read_ver_cb_t)(void);
typedef int (*lcz_lwm2m_sw_mgmt_download_data_cb_t)(uint8_t *data, uint16_t data_len,
						    bool last_block, size_t total_size);

struct lcz_lwm2m_sw_mgmt_event_callback_agent {
	sys_snode_t node;
	uint16_t obj_inst;
	lcz_lwm2m_sw_mgmt_event_cb_t event_callback;
	lcz_lwm2m_sw_mgmt_read_ver_cb_t read_ver_callback;
	lcz_lwm2m_sw_mgmt_download_data_cb_t download_data_callback;
};

/**************************************************************************************************/
/* Global Function Prototypes                                                                     */
/**************************************************************************************************/

/**
 * @brief Create an instance of object 9
 *
 * @param obj_inst instance of object 9
 * @param agent agent to register required callbacks. All callbacks are required when creating the
 * object. read_ver_callback and download_data_callback callbacks only allow one registered user.
 * Those two callbacks will only be registered to the user who calls this function.
 *
 * @return int 0 on success, < 0 on error
 */
int lcz_lwm2m_sw_mgmt_create_inst(uint16_t obj_inst,
				  struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent);

/**
 * @brief Register event callbacks. event_callback is the only callback that supports multiple
 * users. read_ver_callback and download_data_callback callbacks only allow one registered user.
 * Only the user who created the object can register those two callbacks.
 *
 * @param obj_inst instance of object 9
 * @param agent agent to register callbacks
 * @return int 0 on success, < 0 on error
 */
int lcz_lwm2m_sw_mgmt_register_event_callback(uint16_t obj_inst,
					      struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent);

/**
 * @brief Unregister event callbacks
 *
 * @param obj_inst instance of object 9
 * @param agent agent with registered callbacks
 * @return int 0 on success, < 0 on error
 */
int lcz_lwm2m_sw_mgmt_register_event_callback(uint16_t obj_inst,
					      struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent);

/**
 * @brief Set the software package name
 *
 * @param obj_inst instance of object 9
 * @param value package name
 * @return int 0 on success, < 0 on error
 */
int lcz_lwm2m_sw_mgmt_set_pkg_name(uint16_t obj_inst, char *value);

/**
 * @brief Set the software package version
 *
 * @param obj_inst instance of object 9
 * @param value package version
 * @return int 0 on success, < 0 on error
 */
int lcz_lwm2m_sw_mgmt_set_pkg_version(uint16_t obj_inst, char *value);

/**
 * @brief Set the activation state of a software package
 *
 * @param obj_inst instance of object 9
 * @param activate true for enabled false for disabled
 * @return int
 */
int lcz_lwm2m_sw_mgmt_set_activate_state(uint16_t obj_inst, bool activate);

#ifdef __cplusplus
}
#endif

#endif /* __LCZ_LWM2M_SW_MGMT_H__ */
