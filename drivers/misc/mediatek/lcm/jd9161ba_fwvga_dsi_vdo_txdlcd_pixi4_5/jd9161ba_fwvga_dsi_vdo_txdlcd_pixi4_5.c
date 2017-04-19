#define LOG_TAG "LCM"

#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#else
#include <stdio.h>
#include <string.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#define LCM_LOGI(fmt)  dprintf(CRITICAL,fmt)
#else
#define LCM_LOGI(fmt, args...)  printk("[KERNEL/"LOG_TAG"]"fmt, ##args)
#endif



// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(854)

#define REGFLAG_DELAY             							0xFFFE
#define REGFLAG_END_OF_TABLE      							0xFFFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE									0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)									lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)				lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg											lcm_util.dsi_read_reg()
       
struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xBF,3,{0x91,0x61,0xF2}},

{0xB3,2,{0x00,0x63}},

{0xB4,2,{0x00,0x63}},

{0xB8,6,{0x00,0xB6,0x01,0x00,0xB6,0x01}},

{0xBA,3,{0x34,0x23,0x00}},

{0xC3,1,{0x02}},

{0xC4,2,{0x30,0x6A}},

{0xC7,9,{0x00,0x01,0x31,0x05,0x65,0x2C,0x13,0xA5,0xA5}},

{0xC8,38,{0x79,0x58,0x4F,0x3A,0x35,0x24,0x24,0x0F,0x2B,0x2D,0x31,0x55,0x4B,0x5C,0x56,0x5D,0x57,0x4F,0x41,0x79,0x58,0x4F,0x3A,0x35,0x24,0x24,0x0F,0x2B,0x2D,0x31,0x55,0x4B,0x5C,0x56,0x5D,0x57,0x4F,0x41}},

{0xD4,16,{0x1F,0x1E,0x1F,0x00,0x10,0x1F,0x1F,0x04,0x08,0x06,0x0A,0x1F,0x1F,0x1F,0x1F,0x1F}},

{0xD5,16,{0x1F,0x1E,0x1F,0x01,0x11,0x1F,0x1F,0x05,0x09,0x07,0x0B,0x1F,0x1F,0x1F,0x1F,0x1F}},

{0xD6,16,{0x1F,0x1F,0x1E,0x11,0x01,0x1F,0x1F,0x09,0x05,0x07,0x0B,0x1F,0x1F,0x1F,0x1F,0x1F}},

{0xD7,16,{0x1F,0x1F,0x1E,0x10,0x00,0x1F,0x1F,0x08,0x04,0x06,0x0A,0x1F,0x1F,0x1F,0x1F,0x1F}},

{0xD8,20,{0x20,0x02,0x0A,0x10,0x05,0x30,0x01,0x02,0x30,0x01,0x02,0x06,0x70,0x53,0x61,0x73,0x09,0x06,0x70,0x08}},

{0xD9,19,{0x00,0x0A,0x0A,0x88,0x00,0x00,0x06,0x7B,0x00,0x00,0x00,0x3B,0x2F,0x1F,0x00,0x00,0x00,0x03,0x7B}},

{0xBE,1,{0x01}},

{0xC1,1,{0x10}},

{0xCC,10,{0x34,0x20,0x38,0x60,0x11,0x91,0x00,0x40,0x00,0x00}},

{0xBE,1,{0x00}},

{0X11,1,{0X00}},
{REGFLAG_DELAY,120, {0x00}},
{0X29,1,{0X00}},
{REGFLAG_DELAY, 5, {0x00}},				// duanjinhui.wt
{REGFLAG_END_OF_TABLE, 0x00, {}} 		// duanjinhui.wt
};
static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Sleep Mode On
	{0x28, 1, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	
	// Sleep Mode On
	{0x10, 1, {0x00}},
	{REGFLAG_DELAY, 120, {}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};



static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;
    
    	// enable tearing-free
	params->dbi.te_mode = LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
    params->dsi.switch_mode = SYNC_PULSE_VDO_MODE;
#else
    params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

    // DSI
    /* Command mode setting */
    params->dsi.LANE_NUM				= LCM_TWO_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
    params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
    params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Highly depends on LCD driver capability.
    // Not support in MT6573
    params->dsi.packet_size=256;
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
    
    params->dsi.vertical_sync_active = 4;//6;
	params->dsi.vertical_backporch = 8;//6;
	params->dsi.vertical_frontporch = 10;//8;
	params->dsi.vertical_active_line = FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch = 40;
	params->dsi.horizontal_frontporch = 40;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

/*-begin-2016-03-07-modify by jiayu.ding for LCM ssc closed && MIPI clock 180->156-*/
	params->dsi.ssc_disable = 1;

	params->dsi.PLL_CLOCK = 156; //270; //this value must be in MTK suggested table
/*-end-2016-03-07-modify by jiayu.ding for LCM ssc closed && MIPI clock 180->156-*/

    //ESD
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 0;
	params->dsi.customization_esd_check_enable = 0;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
}


static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(2);
    SET_RESET_PIN(0);
    MDELAY(15);
    SET_RESET_PIN(1);
    //MDELAY(20);
    MDELAY(120);
    

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	
	LCM_LOGI("liuyang:lcm_init!!!\n");
}


static void lcm_suspend(void)
{
	SET_RESET_PIN(1);			// duanjinhui.wt
    MDELAY(2);
    SET_RESET_PIN(0);
    MDELAY(15);
    SET_RESET_PIN(1);
	MDELAY(120);
	push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);

}


static void lcm_resume(void)
{
	lcm_init();
}

static unsigned int lcm_compare_id(void)
{
    return 0;
}

LCM_DRIVER jd9161ba_fwvga_dsi_vdo_txdlcd_pixi4_5_lcm_drv = 
{
    .name			= "jd9161ba_fwvga_dsi_vdo_txdlcd_pixi4_5",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id = lcm_compare_id,
};
