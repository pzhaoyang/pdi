#ifndef __DEBUG_TOOL_H__
#define __DEBUG_TOOL_H__

#include <stdint.h>

#define PDI_PAD_PORT_NUM ((unsigned int)(0x1C20))
#define PDI_SERVMON_PORT_NUM (25427)
#define PDI_DLMD_PORT_NUM    (50001)
#define PDI_VSMD_PORT_NUM    (50002)
#define PDI_ESSIFCD_PORT_NUM (50003)
#define PDI_ALD_SSC_PORT_NUM (50004)
#define PDI_SERVLOG_PORT_NUM (50005)
#define PDI_IP4RTE_PORT_NUM  (50006)
#define PDI_PAP_RES_PORT_NUM (50007)

#define PDI_APPMON_PORT_NUM  (50009)

extern unsigned int pdi_debug_port_num;

#endif
