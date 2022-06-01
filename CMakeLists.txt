#
# Copyright (c) 2022 Laird Connectivity LLC
#
# SPDX-License-Identifier: LicenseRef-LairdConnectivity-Clause
#

if(CONFIG_LCZ_LWM2M_SW_MANAGEMENT)

zephyr_include_directories(include)
zephyr_sources(src/lcz_lwm2m_sw_mgmt.c)
zephyr_sources_ifdef(CONFIG_LCZ_LWM2M_SW_MGMT_HL7800
    src/lcz_lwm2m_sw_mgmt_hl7800.c)

endif()
