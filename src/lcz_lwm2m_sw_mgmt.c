/**
 * @file lcz_lwm2m_sw_mgmt.c
 *
 * Copyright (c) 2022 Laird Connectivity
 *
 * SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
 */

/**************************************************************************************************/
/* Includes                                                                                       */
/**************************************************************************************************/
#include <logging/log.h>
LOG_MODULE_REGISTER(lcz_lwm2m_sw_mgmt, CONFIG_LCZ_LWM2M_SW_MANAGEMENT_LOG_LEVEL);

#include <zephyr.h>
#include <init.h>
#include "lwm2m_engine.h"
#include "lcz_lwm2m.h"
#include "lcz_lwm2m_sw_mgmt.h"

/**************************************************************************************************/
/* Local Data Definitions                                                                         */
/**************************************************************************************************/
static sys_slist_t sw_mgmt_event_callback_list =
	SYS_SLIST_STATIC_INIT(&sw_mgmt_event_callback_list);

static K_MUTEX_DEFINE(cb_lock);

/**************************************************************************************************/
/* Local Function Prototypes                                                                      */
/**************************************************************************************************/
static int sw_mgmt_activate_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len);
static int sw_mgmt_deactivate_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len);
static int sw_mgmt_install_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len);
static int sw_mgmt_uninstall_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len);
static void *read_ver_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			 size_t *data_len);
static int write_data_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			 uint16_t data_len, bool last_block, size_t total_size);

/**************************************************************************************************/
/* Local Function Definitions                                                                     */
/**************************************************************************************************/
static int sw_mgmt_activate_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	int ret;
	sys_snode_t *node;
	struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent;

	ARG_UNUSED(args);
	ARG_UNUSED(args_len);
	ret = 0;

	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&sw_mgmt_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_lwm2m_sw_mgmt_event_callback_agent, node);
		if (agent->event_callback != NULL && agent->obj_inst == obj_inst_id) {
			ret |= agent->event_callback(LCZ_LWM2M_SW_MGMT_EVENT_ACTIVATE);
		}
	}
	k_mutex_unlock(&cb_lock);

	return ret;
}

static int sw_mgmt_deactivate_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	int ret;
	sys_snode_t *node;
	struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent;

	ARG_UNUSED(args);
	ARG_UNUSED(args_len);
	ret = 0;

	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&sw_mgmt_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_lwm2m_sw_mgmt_event_callback_agent, node);
		if (agent->event_callback != NULL && agent->obj_inst == obj_inst_id) {
			ret |= agent->event_callback(LCZ_LWM2M_SW_MGMT_EVENT_DEACTIVATE);
		}
	}
	k_mutex_unlock(&cb_lock);

	return ret;
}

static int sw_mgmt_install_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	int ret;
	sys_snode_t *node;
	struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent;

	ARG_UNUSED(args);
	ARG_UNUSED(args_len);
	ret = 0;

	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&sw_mgmt_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_lwm2m_sw_mgmt_event_callback_agent, node);
		if (agent->event_callback != NULL && agent->obj_inst == obj_inst_id) {
			ret |= agent->event_callback(LCZ_LWM2M_SW_MGMT_EVENT_INSTALL);
		}
	}
	k_mutex_unlock(&cb_lock);

	return ret;
}

static int sw_mgmt_uninstall_exe_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	int ret;
	sys_snode_t *node;
	struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent;

	ARG_UNUSED(args);
	ARG_UNUSED(args_len);
	ret = 0;

	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&sw_mgmt_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_lwm2m_sw_mgmt_event_callback_agent, node);
		if (agent->event_callback != NULL && agent->obj_inst == obj_inst_id) {
			ret |= agent->event_callback(LCZ_LWM2M_SW_MGMT_EVENT_UNINSTALL);
		}
	}
	k_mutex_unlock(&cb_lock);

	return ret;
}

static void *read_ver_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			 size_t *data_len)
{
	sys_snode_t *node;
	struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent;
	lcz_lwm2m_sw_mgmt_read_ver_cb_t cb;

	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);
	ARG_UNUSED(data_len);

	cb = NULL;
	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&sw_mgmt_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_lwm2m_sw_mgmt_event_callback_agent, node);
		if (agent->event_callback != NULL && agent->obj_inst == obj_inst_id) {
			/* Only allow one registered callback for this object instance.
			 * Use the first registered instance (the callback used when creating the obj).
			 */
			cb = agent->read_ver_callback;
			goto unlock;
		}
	}
unlock:
	k_mutex_unlock(&cb_lock);
	if (cb) {
		return cb();
	} else {
		return NULL;
	}
}

static int write_data_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id, uint8_t *data,
			 uint16_t data_len, bool last_block, size_t total_size)
{
	sys_snode_t *node;
	struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent;
	lcz_lwm2m_sw_mgmt_download_data_cb_t cb;

	ARG_UNUSED(res_id);
	ARG_UNUSED(res_inst_id);

	cb = NULL;
	k_mutex_lock(&cb_lock, K_FOREVER);
	SYS_SLIST_FOR_EACH_NODE(&sw_mgmt_event_callback_list, node) {
		agent = CONTAINER_OF(node, struct lcz_lwm2m_sw_mgmt_event_callback_agent, node);
		if (agent->event_callback != NULL && agent->obj_inst == obj_inst_id) {
			/* Only allow one registered callback for this object instance.
			 * Use the first registered instance (the callback used when creating the obj).
			 */
			cb = agent->download_data_callback;
			goto unlock;
		}
	}
unlock:
	k_mutex_unlock(&cb_lock);
	if (cb) {
		return cb(data, data_len, last_block, total_size);
	} else {
		return -ENOEXEC;
	}
}

/**************************************************************************************************/
/* Global Function Definitions                                                                    */
/**************************************************************************************************/
int lcz_lwm2m_sw_mgmt_create_inst(uint16_t obj_inst,
				  struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent)
{
	int ret;
	char obj_path[LWM2M_MAX_PATH_STR_LEN];
	struct lwm2m_engine_obj_inst *inst;

	if (!agent->event_callback || !agent->read_ver_callback || !agent->download_data_callback) {
		ret = -EINVAL;
		goto exit;
	}

	ret = lwm2m_create_obj_inst(LWM2M_OBJECT_SOFTWARE_MANAGEMENT_ID, obj_inst, &inst);
	if (ret < 0) {
		goto exit;
	}

	snprintk(obj_path, sizeof(obj_path), "9/%d/1/0", obj_inst);
	ret = lwm2m_engine_create_res_inst(obj_path);
	if (ret < 0) {
		goto exit;
	}

	ret = lwm2m_swmgmt_set_activate_cb(obj_inst, sw_mgmt_activate_exe_cb);
	if (ret < 0) {
		goto exit;
	}

	ret = lwm2m_swmgmt_set_deactivate_cb(obj_inst, sw_mgmt_deactivate_exe_cb);
	if (ret < 0) {
		goto exit;
	}

	ret = lwm2m_swmgmt_set_install_package_cb(obj_inst, sw_mgmt_install_exe_cb);
	if (ret < 0) {
		goto exit;
	}

	ret = lwm2m_swmgmt_set_delete_package_cb(obj_inst, sw_mgmt_uninstall_exe_cb);
	if (ret < 0) {
		goto exit;
	}

	ret = lwm2m_swmgmt_set_read_package_version_cb(obj_inst, read_ver_cb);
	if (ret < 0) {
		goto exit;
	}

	ret = lwm2m_swmgmt_set_write_package_cb(obj_inst, write_data_cb);
	if (ret < 0) {
		goto exit;
	}

	agent->obj_inst = obj_inst;
	sys_slist_append(&sw_mgmt_event_callback_list, &agent->node);

exit:
	return ret;
}

int lcz_lwm2m_sw_mgmt_register_event_callback(uint16_t obj_inst,
					      struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent)
{
	k_mutex_lock(&cb_lock, K_FOREVER);
	agent->obj_inst = obj_inst;
	sys_slist_append(&sw_mgmt_event_callback_list, &agent->node);
	k_mutex_unlock(&cb_lock);
	return 0;
}

int lcz_lwm2m_sw_mgmt_unregister_event_callback(
	uint16_t obj_inst, struct lcz_lwm2m_sw_mgmt_event_callback_agent *agent)
{
	k_mutex_lock(&cb_lock, K_FOREVER);
	(void)sys_slist_find_and_remove(&sw_mgmt_event_callback_list, &agent->node);
	k_mutex_unlock(&cb_lock);
	return 0;
}

int lcz_lwm2m_sw_mgmt_set_pkg_name(uint16_t obj_inst, char *value)
{
	char obj_path[LWM2M_MAX_PATH_STR_LEN];

	snprintk(obj_path, sizeof(obj_path), "9/%d/0", obj_inst);
	return lwm2m_engine_set_string(obj_path, value);
}

int lcz_lwm2m_sw_mgmt_set_pkg_version(uint16_t obj_inst, char *value)
{
	char obj_path[LWM2M_MAX_PATH_STR_LEN];

	snprintk(obj_path, sizeof(obj_path), "9/%d/1", obj_inst);
	return lwm2m_engine_set_string(obj_path, value);
}

int lcz_lwm2m_sw_mgmt_set_activate_state(uint16_t obj_inst, bool activate)
{
	char obj_path[LWM2M_MAX_PATH_STR_LEN];

	snprintk(obj_path, sizeof(obj_path), "9/%d/12", obj_inst);
	return lwm2m_engine_set_bool(obj_path, activate);
}
