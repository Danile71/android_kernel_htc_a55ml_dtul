#ifndef BUILD_LK
#include <linux/string.h>
#include <mach/upmu_common_sw.h>
#include <linux/htc_devices_dtb.h>
#endif
#include "lcm_drv.h"
#include "tps65132_iic.h"

#ifdef BUILD_LK
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <htc_board_info_and_setting.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#endif

#include <cust_gpio_usage.h>
#include <cust_i2c.h>
#define LCM_DSI_CMD_MODE 0
#define FRAME_WIDTH (1440)
#define FRAME_HEIGHT (2560)
#define GPIO_65132_EN GPIO_LCD_BIAS_ENP_PIN

#define REGFLAG_PORT_SWAP 0xFFFA
#define REGFLAG_DELAY 0xFFFC
#define REGFLAG_END_OF_TABLE 0xFFFD 

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define PWM_MIN 6
#define PWM_DEFAULT 57
#define PWM_MAX 213

static const unsigned int BL_MIN_LEVEL = 20;
static LCM_UTIL_FUNCS lcm_util;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))
#define MDELAY(n) (lcm_util.mdelay(n))

#define dsi_set_cmd_by_cmdq_dual(handle,cmd,count,ppara,force_update)    lcm_util.dsi_set_cmdq_V23(handle,cmd,count,ppara,force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define dsi_swap_port(swap) lcm_util.dsi_swap_port(swap)

struct LCM_setting_table {
	unsigned int cmd;
	unsigned char count;
	unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] =
{
	{0xB9,  3, {0xFF, 0x83, 0x96}},

	
	{0xB1,  8, {0x10, 0x31, 0x71, 0x30, 0x00, 0x21, 0x21, 0x74}},
	{0xBD,  1, {0x01}},

	
	{0xB1,  3, {0x40, 0x44, 0x00}},

	{0xBD,  1, {0x00}},

	
	{0xB2,  7, {0x40, 0x08, 0xDC, 0x06, 0x04, 0x2D, 0x21}},

	
	{0xB4, 20, {0x00, 0x42, 0x04, 0x83,
				0x00, 0x00, 0x00, 0x00,
				0x07, 0x08, 0x00, 0x21,
				0x02, 0x08, 0x0A, 0x00,
				0x00, 0x83, 0x43, 0x07}},
	{0xE9,  1, {0x87}},

	
	{0xBF,  1, {0x07}},
	{0xE9,  1, {0x3F}},
	{0xB6,  3, {0x85, 0x85, 0x03}},
	{0xC0,  2, {0x01, 0x41}},
	{0xCC,  1, {0x02}},

	
	{0xD2,  1, {0x66}},

	{0xD3, 28, {0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x0A, 0x00,
				0x32, 0x10, 0x04, 0x00,
				0x04, 0x32, 0x1A, 0x05,
				0x0A, 0x05, 0x32, 0x10,
				0x08, 0x00, 0x00, 0x20,
				0x00, 0x04, 0x04, 0x03}},
	{0xD5, 32, {0x18, 0x18, 0x19, 0x19,
				0x03, 0x02, 0x01, 0x00,
				0x1E, 0x1E, 0x1F, 0x1F,
				0x18, 0x18, 0x20, 0x20,
				0x18, 0x18, 0x34, 0x34,
				0x33, 0x33, 0x32, 0x32,
				0x31, 0x31, 0x30, 0x30,
				0x2F, 0x2F, 0x18, 0x18}},
	{0xD6, 32, {0x18, 0x18, 0x19, 0x19,
				0x00, 0x01, 0x02, 0x03,
				0x1E, 0x1E, 0x1F, 0x1F,
				0x18, 0x18, 0x20, 0x20,
				0x18, 0x18, 0x34, 0x34,
				0x33, 0x33, 0x32, 0x32,
				0x31, 0x31, 0x30, 0x30,
				0x2F, 0x2F, 0x18, 0x18}},
	{0xD8,  5, {0x20, 0x00, 0x00, 0x00, 0x20}},
	{0xBD,  1, {0x01}},
	{0xD8, 16, {0x0F, 0x03, 0x2A, 0xFC,
				0x0F, 0x03, 0x2A, 0xFC,
				0x0A, 0x02, 0x3F, 0xA8,
				0x0A, 0x02, 0x3F, 0xA8}},
	{0xBD,  1, {0x02}},
	{0xD8,  8, {0x0F, 0x03, 0x2A, 0xFC, 0x0F, 0x03, 0x2A, 0xFC}},
	{0xBD,  1, {0x00}},

	
	{0xE0, 42, {0x10, 0x16, 0x22, 0x1D,
				0x45, 0x4F, 0x5C, 0x57,
				0x66, 0x6D, 0x75, 0x7F,
				0x8E, 0xA8, 0xA1, 0xB4,
				0xBA, 0x60, 0x5B, 0x65,
				0x6F, 0x00, 0x16, 0x22,
				0x1D, 0x45, 0x4F, 0x5C,
				0x57, 0x66, 0x6D, 0x75,
				0x7F, 0x8E, 0xA8, 0xA1,
				0xB4, 0xBA, 0x60, 0x5B,
				0x65, 0x72}},

	{0xB8,  1, {0xA5}},
	{0xEF,  1, {0x77}},
	{0xB1,  3, {0x2F, 0x21, 0x03}},
	{0xB2,  4, {0x56, 0x00, 0x23, 0x01}},
	{0xB3,  5, {0x20, 0x20, 0xFF, 0x00, 0x30}},
	{0xB4, 12, {0x09, 0x2E, 0x09, 0x00,
				0x20, 0x20, 0x40, 0x00,
				0x00, 0x40, 0x00, 0x00}},
	{0xB5, 12, {0x09, 0x37, 0x00, 0x00,
				0x40, 0x00, 0x0D, 0x33,
				0x00, 0x10, 0x30, 0x00}},
	{0xB6, 12, {0x00, 0x37, 0x09, 0x00,
				0x20, 0x20, 0x00, 0x33,
				0x0D, 0x00, 0x20, 0x20}},
	{0xB7, 12, {0x09, 0x2E, 0x09, 0x00,
				0x20, 0x20, 0x40, 0x00,
				0x00, 0x40, 0x00, 0x00}},
	{0xB8, 12, {0x09, 0x37, 0x00, 0x00,
				0x40, 0x00, 0x0D, 0x33,
				0x00, 0x10, 0x30, 0x00}},
	{0xB9, 12, {0x00, 0x37, 0x09, 0x00,
				0x20, 0x20, 0x00, 0x33,
				0x0D, 0x00, 0x20, 0x20}},
	{0xBA, 16, {0x00, 0x00, 0x40, 0x00,
				0x80, 0x00, 0xC0, 0x00,
				0x00, 0x01, 0x40, 0x01,
				0x80, 0x01, 0xC0, 0x01}},
	{0xBB, 16, {0x00, 0x02, 0x40, 0x02,
				0x80, 0x02, 0xC0, 0x02,
				0x00, 0x03, 0x40, 0x03,
				0x80, 0x03, 0xC0, 0x03}},
	{0xBC,  4, {0x00, 0x04, 0x00, 0x00}},
	{0xBD, 16, {0x00, 0x00, 0x40, 0x00,
				0x80, 0x00, 0xC0, 0x00,
				0x00, 0x01, 0x40, 0x01,
				0x80, 0x01, 0xC0, 0x01}},
	{0xBE, 16, {0x00, 0x02, 0x40, 0x02,
				0x80, 0x02, 0xC0, 0x02,
				0x00, 0x03, 0x40, 0x03,
				0x80, 0x03, 0xC0, 0x03}},
	{0xBF,  4, {0x00, 0x04, 0x00, 0x00}},
	{0xC0, 16, {0x00, 0x00, 0x40, 0x00,
				0x80, 0x00, 0xC0, 0x00,
				0x00, 0x01, 0x40, 0x01,
				0x80, 0x01, 0xC0, 0x01}},
	{0xC1, 16, {0x00, 0x02, 0x40, 0x02,
			0x80, 0x02, 0xC0, 0x02,
			0x00, 0x03, 0x40, 0x03,
			0x80, 0x03, 0xC0, 0x03}},
	{0xC2,  4, {0x00, 0x04, 0x00, 0x00}},
	{0xCF,  4, {0xA8, 0x29, 0x40, 0x08}},
	{0xD0,  5, {0x00, 0x00, 0x00, 0x00, 0x00}},
	{0xD1,  6, {0x42, 0xF3, 0xB1, 0x0F, 0x1C, 0x80}},
	{0xD2,  1, {0x34}},
	{0xDA,  2, {0x0C, 0x40}},
	{0xDB,  9, {0x00, 0xEE, 0xDE, 0xDD, 0xED, 0xEE, 0xEE, 0x0E, 0x00}},
	{0xDC,  5, {0x54, 0x86, 0xB9, 0xEC, 0x0F}},
	{0xEF,  1, {0x00}},

	{0x35,  0, {}},

	
	{0x11,  1, {0x00}},

	
	{REGFLAG_DELAY, 120, {}},

	
	{0xB8,  1, {0xA5}},

	
	{0xD9,  4, {0x80, 0x00, 0x02, 0x03}},

	
	{0xC9,  4, {0x04, 0x1E, 0xB1, 0x01}},

	
	{0xE9,  1, {0x87}},
	{REGFLAG_DELAY, 1, {}},
	
	{0xBF,  1, {0x07}},
	{REGFLAG_DELAY, 1, {}},
	
	{0xE9,  1, {0x3F}},

	{0x53,  1, {0x24}},
	{0x55,  1, {0x00}},

	
	{REGFLAG_DELAY, 10, {}},

	{0xE9,  1, {0x83}},
	{REGFLAG_DELAY, 1, {}},
	{0xCB,  1, {0x87}},
	{REGFLAG_DELAY, 1, {}},
	{0xE9,  1, {0x3F}},

	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x00}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xC1, 43, {0x01, 0x00, 0x08, 0x11,
				0x19, 0x21, 0x29, 0x31,
				0x38, 0x3F, 0x45, 0x4B,
				0x53, 0x5B, 0x63, 0x6B,
				0x74, 0x7D, 0x86, 0x8F,
				0x99, 0xA2, 0xAB, 0xB4,
				0xBB, 0xC3, 0xCB, 0xD2,
				0xD9, 0xE1, 0xE8, 0xEF,
				0xF7, 0xFF, 0x06, 0xFB,
				0x3D, 0x4D, 0x0E, 0x51,
				0x6F, 0x7D, 0xC1}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x01}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xC1, 42, {0x00, 0x08, 0x11, 0x19,
				0x21, 0x29, 0x31, 0x38,
				0x3F, 0x45, 0x4B, 0x53,
				0x5B, 0x63, 0x6B, 0x74,
				0x7D, 0x86, 0x8F, 0x99,
				0xA2, 0xAB, 0xB4, 0xBB,
				0xC3, 0xCB, 0xD2, 0xD9,
				0xE1, 0xE8, 0xEF, 0xF7,
				0xFF, 0x06, 0xFB, 0x39,
				0x4D, 0x0E, 0x51, 0x6F,
				0x7D, 0xC1}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x02}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xC1, 42, {0x00, 0x08, 0x11, 0x19,
				0x21, 0x29, 0x31, 0x38,
				0x3F, 0x45, 0x4C, 0x53,
				0x5B, 0x63, 0x6B, 0x74,
				0x7D, 0x86, 0x8F, 0x99,
				0xA2, 0xAB, 0xB4, 0xBB,
				0xC3, 0xCB, 0xD2, 0xD9,
				0xE1, 0xE8, 0xEF, 0xF7,
				0xFF, 0x06, 0xFB, 0x35,
				0x4D, 0x0E, 0x51, 0x6F,
				0x7D, 0xC1}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x00}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xE5, 45, {0x06, 0x64, 0x20, 0x2E,
				0xEC, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x44, 0x44,
				0x04, 0x00, 0x02, 0x08,
				0x40, 0x80, 0x00, 0x02,
				0x08, 0x20, 0x80, 0x00,
				0x02, 0x08, 0x20, 0x80,
				0x00, 0x02, 0x08, 0x20,
				0x80, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x02}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xE6, 60, {0x47, 0x3A, 0x1A, 0xE7,
				0x94, 0x47, 0xEE, 0xC8,
				0x22, 0x87, 0x4E, 0x6E,
				0xFA, 0xA7, 0x98, 0x45,
				0x06, 0x09, 0x22, 0x80,
				0x54, 0xA2, 0xDA, 0x68,
				0x9C, 0x42, 0x4E, 0x98,
				0x9D, 0x67, 0x4D, 0x6A,
				0xFA, 0x67, 0x99, 0x3C,
				0x52, 0x18, 0xDE, 0x6A,
				0x46, 0x2E, 0x2A, 0x27,
				0x96, 0x3A, 0x7A, 0x48,
				0x9F, 0x70, 0x4A, 0x4E,
				0x0A, 0x6A, 0xAA, 0x83,
				0x46, 0x89, 0xA2, 0x83}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x03}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xE6, 60, {0x4E, 0x6E, 0xFA, 0xAC,
				0xBB, 0x93, 0x2E, 0x99,
				0x62, 0x87, 0x2C, 0x62,
				0xC9, 0x26, 0x9F, 0x5C,
				0xEA, 0x18, 0xA2, 0x84,
				0x0A, 0x52, 0xA8, 0xA0,
				0x80, 0x13, 0x5A, 0x48,
				0x20, 0x7C, 0x00, 0x02,
				0x08, 0x20, 0x80, 0xF3,
				0xBD, 0x07, 0x5E, 0x71,
				0xF6, 0xB1, 0x67, 0x1F,
				0x80, 0xE3, 0x2D, 0x57,
				0x5B, 0x65, 0x1E, 0xF6,
				0x38, 0xA3, 0x8A, 0x14,
				0xFE, 0x97, 0xDE, 0x74}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x00}},
	{REGFLAG_DELAY, 1, {}},

	{0xCE,  2, {0x04, 0x06}},

	
	{REGFLAG_DELAY, 5, {}},

	
	
	{0x55, 1, {0x02}},

	
	{REGFLAG_DELAY, 5, {}},

	{0xBD,  1, {0x00}},
	{REGFLAG_DELAY, 1, {}},

	{0xCE,  2, {0x24, 0x06}},

	
	{0xE4, 63, {0x6C, 0x01, 0x2C, 0x00,
				0x08, 0x00, 0x14, 0x10,
				0x00, 0x04, 0xCD, 0xF3,
				0x1A, 0x40, 0x66, 0x8C,
				0xB2, 0xDE, 0xFF, 0xFA,
				0xFF, 0x03, 0x00, 0x10,
				0x20, 0x20, 0x01, 0x00,
				0x10, 0x80, 0x00, 0x00,
				0x90, 0x10, 0x10, 0x10,
				0x10, 0x00, 0x02, 0x00,
				0x00, 0x20, 0x10, 0x10,
				0x80, 0x00, 0x00, 0xD0,
				0x20, 0x20, 0x20, 0x20,
				0x00, 0x00, 0x04, 0x1B,
				0x00, 0x00, 0x00, 0x80,
				0x0C, 0x40, 0xFF}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD,  1, {0x01}},
	{REGFLAG_DELAY, 1, {}},

	
	{0xE4, 41, {0x00, 0x77, 0xFF, 0x87,
				0xEF, 0x93, 0x53, 0x24,
				0xFF, 0xE1, 0xC8, 0xB2,
				0xA3, 0x94, 0x85, 0x76,
				0x68, 0x5B, 0x4E, 0x42,
				0x37, 0x2C, 0x23, 0x1A,
				0x13, 0x0D, 0x08, 0x04,
				0x02, 0x01, 0x00, 0x00,
				0x00, 0xBC, 0x6A, 0x55,
				0x55, 0x55, 0x55, 0x55,
				0x55}},
	{REGFLAG_DELAY, 1, {}},
	{0xBD, 1, {0x00}},
	{REGFLAG_DELAY, 1, {}},
	

	
	{0x29,  1, {0x00}},
	{REGFLAG_DELAY, 10, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static struct LCM_setting_table lcm_suspend_setting[] = {
	{0x28,0,{}},
	{REGFLAG_DELAY, 24, {}},
	{0x10,0,{}},
	{REGFLAG_DELAY, 80, {}},

	
	{0x53, 1, {0x00}},
	{0x55, 1, {0x00}},

	
	{REGFLAG_DELAY, 5, {}},

	{0xBD,  1, {0x00}},
	{REGFLAG_DELAY, 1, {}},

	{0xCE,  2, {0x04, 0x06}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++)
	{
		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {
		case REGFLAG_DELAY :
			if(table[i].count <= 10)
				MDELAY(table[i].count);
			else
				MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE :
			break;

		case REGFLAG_PORT_SWAP:
			dsi_swap_port(1);
			break;
		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}
}

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;
	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;
#if defined(CONFIG_CUSTOM_KERNEL_LCM_PHY_WIDTH) && defined(CONFIG_CUSTOM_KERNEL_LCM_PHY_HEIGHT)
	params->physical_width = CONFIG_CUSTOM_KERNEL_LCM_PHY_WIDTH;
	params->physical_height = CONFIG_CUSTOM_KERNEL_LCM_PHY_HEIGHT;
#endif
	params->lcm_if = LCM_INTERFACE_DSI_DUAL;
	params->lcm_cmd_if = LCM_INTERFACE_DSI0;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode = CMD_MODE;
#else
	params->dsi.mode = SYNC_PULSE_VDO_MODE;
#endif
	params->dsi.dual_dsi_type = DUAL_DSI_VDO;

	
	
	params->dsi.LANE_NUM = LCM_FOUR_LANE;
	
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	
	params->dsi.packet_size = 256;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range = 4;

	
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.vertical_sync_active = 2;
	params->dsi.vertical_backporch = 6;
	params->dsi.vertical_frontporch	= 123;
	params->dsi.vertical_active_line = FRAME_HEIGHT;
	params->dsi.horizontal_sync_active = 32;
	params->dsi.horizontal_backporch = 32;
	params->dsi.horizontal_frontporch = 39;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	params->dsi.PLL_CLOCK = 412;
	params->dsi.ufoe_enable = 1;
	params->dsi.ufoe_params.lr_mode_en = 1;

	

	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 0;

	params->dsi.clk_lp_per_line_enable = 0;


#ifdef BUILD_LK
	if (htc_setting_pcbid() <= XC) {
#else
	if (of_machine_hwid() <= 2) {
#endif
		LCD_LOG("[DISP]:%s: enable lane swap function.\n", __FUNCTION__);
		
		params->dsi.lane_swap_en = 1;
		params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_1;
		params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_0;
		params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_3;
		params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_2;
		params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_CK;
		params->dsi.lane_swap[MIPITX_PHY_PORT_1][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_1;

		params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_0] = MIPITX_PHY_LANE_1;
		params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_1] = MIPITX_PHY_LANE_0;
		params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_2] = MIPITX_PHY_LANE_3;
		params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_3] = MIPITX_PHY_LANE_2;
		params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_CK] = MIPITX_PHY_LANE_CK;
		params->dsi.lane_swap[MIPITX_PHY_PORT_0][MIPITX_PHY_LANE_RX] = MIPITX_PHY_LANE_1;
	} else {
		LCD_LOG("[DISP]:%s: disable lane swap function.\n", __FUNCTION__);
		params->dsi.lane_swap_en = 0;
	}

	params->pwm_min = 6;
	params->pwm_default = 57;
	params->pwm_max = 213;
	params->camera_blk = 203;
	params->camera_dua_blk = 203;
	params->dim_cmd = 0x2c;
}

static void lcm_power_on(void) {
	unsigned char cmd = 0x0;
	unsigned char data = 0xFF;

	LCD_LOG("[DISP] %s: \n", __FUNCTION__);

	
	mt_set_gpio_mode((GPIO62 | 0x80000000), GPIO_MODE_00);
	mt_set_gpio_dir((GPIO62 | 0x80000000), GPIO_DIR_OUT);
	mt_set_gpio_out((GPIO62 | 0x80000000), GPIO_OUT_ONE);
	MDELAY(10);

#ifdef BUILD_LK
	mt6331_upmu_set_rg_vgp1_en(1);
	mt6331_upmu_set_rg_vcam_io_en(1);
#endif
	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ONE);
	MDELAY(1);
	
	cmd = 0x00;
	data = 0x0A;
	tps65132_write_bytes(cmd, data);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ONE);
	MDELAY(1);
	
	cmd = 0x01;
	data = 0x0A;
	tps65132_write_bytes(cmd, data);
	
	tps65132_write_bytes(0x3, 0x43);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_01);

	SET_RESET_PIN(0);
	MDELAY(2);

	SET_RESET_PIN(1);
	MDELAY(50);
}

static void lcm_power_off(void) {

	LCD_LOG("[DISP] %s: \n", __FUNCTION__);

	SET_RESET_PIN(0);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENN_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENN_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENN_PIN, GPIO_OUT_ZERO);
	MDELAY(10);

	
	mt_set_gpio_mode(GPIO_LCD_BIAS_ENP_PIN, GPIO_MODE_00);
	mt_set_gpio_dir(GPIO_LCD_BIAS_ENP_PIN, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCD_BIAS_ENP_PIN, GPIO_OUT_ZERO);
	MDELAY(10);

	
	mt_set_gpio_mode((GPIO62 | 0x80000000), GPIO_MODE_00);
	mt_set_gpio_dir((GPIO62 | 0x80000000), GPIO_DIR_OUT);
	mt_set_gpio_out((GPIO62 | 0x80000000), GPIO_OUT_ZERO);
	MDELAY(50);
}

static void lcm_init_power(void)
{
	lcm_power_on();
}

static void lcm_resume_power(void)
{
	lcm_power_on();
}

static void lcm_suspend_power(void)
{
	lcm_power_off();
}

static void lcm_init(void)
{
	LCD_LOG("[DISP] %s: \n", __FUNCTION__);
	
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	LCD_LOG("[DISP] %s: \n", __FUNCTION__);
	push_table(lcm_suspend_setting, sizeof(lcm_suspend_setting) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_resume(void)
{
	LCD_LOG("[DISP] %s: \n", __FUNCTION__);
	lcm_init();
}

static void lcm_update(unsigned int x, unsigned int y,
			unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(data_array, 3, 1);

	data_array[0]= 0x002c3909;
	dsi_set_cmdq(data_array, 1, 0);
}

static unsigned int lcm_compare_id(void)
{
	return 1;
}

static unsigned int lcm_check_id(void)
{
#ifdef BUILD_LK
	unsigned int retval = 0;
	unsigned int pcbid = htc_setting_pcbid();

	if (pcbid == XA || pcbid == XB || pcbid == XC) {
		LCD_LOG("[DISP] %s: Tianma pcbid %d, engid %X\n", __func__, pcbid, htc_setting_engineerid());
		return (htc_setting_engineerid() == 0xA) ? 1 : 0;
	} else {
		LCD_LOG("[DISP] %s: Tianma pcbid %d, triple %d\n", __func__, pcbid, which_lcd_module_triple());
		return (which_lcd_module_triple() == 0x8) ? 1 : 0;
	}
#else
	return 0;
#endif
}

static void lcm_setbacklight_cmdq(void* handle,unsigned int level)
{
	unsigned int cmd = 0x51;
	unsigned int count = 1;
	unsigned int value = level;

	LCD_LOG("[DISP] %s, backlight: = %d\n", __func__, level);
	dsi_set_cmd_by_cmdq_dual(handle, cmd, count, &value, 1);
}

static void lcm_set_lcm_cmd(void* handle,unsigned int *lcm_cmd,unsigned int *lcm_count,unsigned int *lcm_value)
{
	unsigned int cmd = lcm_cmd[0];
	unsigned int count = lcm_count[0];
	unsigned int *ppara = lcm_value;

	LCD_LOG("[DISP] %s, lcm cmd: \n", __func__);
	dsi_set_cmd_by_cmdq_dual(handle, cmd, count, ppara, 1);
}

static void lcm_pmic_setting()
{
	mt6332_upmu_set_rg_iwled_ovp(1);
}

LCM_DRIVER hx8396a_wqhd_tianma_lcm_drv =
{
	.name = "hx8396a_tianma_vdo",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume  = lcm_resume,
	.compare_id = lcm_compare_id,
	.check_id = lcm_check_id,
	.init_power	= lcm_init_power,
	.resume_power = lcm_resume_power,
	.suspend_power = lcm_suspend_power,
	.set_backlight_cmdq  = lcm_setbacklight_cmdq,
	.set_lcm_cmd    = lcm_set_lcm_cmd,
#if (LCM_DSI_CMD_MODE)
	.update = lcm_update,
#endif
	.pmic_setting = lcm_pmic_setting,
};
