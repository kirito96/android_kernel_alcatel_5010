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
// djnlcd parameters v2
{0xB9,3,{0xF1,0x08,0x01}},

{0xB1,4,{0x22,0x1E,0x1E,0x87}},

{0xB2,1,{0x22}},

{0xB3,8,{0x01,0x00,0x06,0x06,0x18,0x13,0x39,0x35}},

{0xBA,17,{
0x31,0x00,0x44,0x25,0x91,0x0A,0x00,0x00,0xE1,0x00,
0x00,0x00,0x0F,0x02,0x4F,0xB9,0xEE}},

{0xE3,5,{
0x03,0x03,0x03,0x03,0x00}},

{0xB4,1,{0x02}},

{0xB5,2,{0x02,0x02}},

{0xB6,2,{0x68,0x58}},

{0xB8,2,{0x64,0x22}},

{0xCC,1,{0x00}},

{0xBC,1,{0x47}},

{0xE9,51,{
0x00,0x00,0x06,0x00,0x00,0x0A,0x7C,0x12,0x30,0x00,
0x23,0x0A,0x0A,0x7C,0x27,0x00,0x03,0x00,0x00,0x00,
0x08,0x08,0x98,0xDD,0x20,0x64,0x02,0x88,0x88,0x88,
0x88,0x98,0xDD,0x31,0x75,0x13,0x88,0x88,0x88,0x88,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00}},

{0xEA,22,{
0x06,0x00,0x00,0x00,0x89,0xDD,0x35,0x71,0x31,0x88,
0x88,0x88,0x88,0x89,0xDD,0x24,0x60,0x20,0x88,0x88,
0x88,0x88}},

{0xE0,34,{
0x00,0x00,0x08,0x0D,0x0F,0x3F,0x21,0x31,0x08,0x10,
0x10,0x15,0x17,0x15,0x16,0x13,0x19,0x00,0x00,0x08,
0x0D,0x0F,0x3F,0x21,0x31,0x08,0x10,0x10,0x15,0x17,
0x15,0x16,0x13,0x19}},

{0X11,1,{0X00}},
{REGFLAG_DELAY,120, {0x00}},
{0X29,1,{0X00}},
{REGFLAG_DELAY, 5, {0x00}},
{REGFLAG_END_OF_TABLE, 0x00, {}} 	
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
	params->dsi.vertical_backporch = 12;//9;//8;//6;
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
   // MDELAY(20);
    MDELAY(120);
   

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	
	LCM_LOGI("liuyang:lcm_init!!!\n");
}


static void lcm_suspend(void)
{
		SET_RESET_PIN(1);
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

LCM_DRIVER fl10802_fwvga_dsi_vdo_djnlcd_pixi4_5_lcm_drv = 
{
    .name			= "fl10802_fwvga_dsi_vdo_djnlcd_pixi4_5",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id = lcm_compare_id,
};
