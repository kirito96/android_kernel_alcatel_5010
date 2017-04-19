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


//start:modified by fae for ETT test lcd blink 20151228

static struct LCM_setting_table lcm_initialization_setting[] = {
{0xB9,3,{0xFF,0x83,0x79}},
{0xB1,16,{0x44,0x16,0x16,0x31,0x31,0x50,0xD0,0xEE,0x54,0x80,0x38,0x38,0xF8,0x22,0x22,0x22}},
{0xB2,9,{0x80,0xFE,0x0D,0x0A,0x20,0x50,0x11,0x42,0x1D}}, //0x82
{0xB4,10,{0x02,0x7c,0x02,0x7C,0x02,0x7C,0x22,0x86,0x23,0x86}},

{0xCC, 1, {0x02}}, 

{0xD3,29,{0x00,0x07,0x00,0x3C,0x1C,0x08,0x08,0x32,0x10,0x02,0x00,0x02,0x03,0x70,0x03,0x70,0x00,0x08,0x00,0x08,0x37,0x33,0x06,0x06,0x37,0x06,0x06,0x37,0x0B}},

{0xD5,34,{0x19,0x19,0x18,0x18,0x1A,0x1A,0x1B,0x1B,0x02,0x03,0x00,0x01,0x06,0x07,0x04,0x05,0x20,0x21,0x22,0x23,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00}},

{0xD6,32,{0x18,0x18,0x19,0x19,0x1A,0x1A,0x1B,0x1B,0x03,0x02,0x05,0x04,0x07,0x06,0x01,0x00,0x23,0x22,0x21,0x20,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18}},

{0xE0,42,{0x00,0x04,0x07,0x0F,0x0F,0x1C,0x22,0x33,0x08,0x0B,0x0C,0x17,0x12,0x16,0x19,0x16,0x17,0x07,0x14,0x15,0x14,0x00,0x04,0x07,0x0F,0x0F,0x1C,0x22,0x33,0x08,0x0B,0x0C,0x17,0x12,0x16,0x19,0x16,0x17,0x07,0x14,0x15,0x14}},

{0xC7,4,{0x00,0x00,0x00,0xC0}},

{0xB6,2,{0x4F,0x4F}}, //8B 8B   43
{0x35, 1, {0x00}}, 
{0x11, 1, {0x00}},                 // Sleep-Out
{REGFLAG_DELAY, 150, {0x00}},
{0x29, 1, {0x00 }},                // Display On
{REGFLAG_DELAY, 10, {0x00}},

};

//end:modified by fae for ETT test lcd blink 20151228
/*
static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 0, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};
*/

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
    
  params->dsi.vertical_sync_active = 5;//6;
	params->dsi.vertical_backporch = 8;//8;//6;
	params->dsi.vertical_frontporch = 12;//8;
	params->dsi.vertical_active_line = FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active = 27;
	params->dsi.horizontal_backporch =  35;//40
	params->dsi.horizontal_frontporch = 35;//40
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;
	

/*-begin-2016-03-07-modify by jiayu.ding for LCM ssc closed && MIPI clock 182->156-*/
	params->dsi.ssc_disable = 1;

	params->dsi.PLL_CLOCK = 182; //270; //this value must be in MTK suggested table
/*-end-2016-03-07-modify by jiayu.ding for LCM ssc closed && MIPI clock 182->156-*/

    //ESD
	params->dsi.clk_lp_per_line_enable = 1;
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[0].count        = 1;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
}


static void lcm_init(void)
{
    SET_RESET_PIN(1);
    MDELAY(10);
    SET_RESET_PIN(0);
    MDELAY(10);
    SET_RESET_PIN(1);
   // MDELAY(20);
   MDELAY(120);
   

	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	LCM_LOGI("liuyang:lcm_init!!!\n");
}


static void lcm_suspend(void)
{
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

LCM_DRIVER hx8379c_fwvga_dsi_vdo_holitech_pixi4_5_lcm_drv = 
{
    .name			= "hx8379c_fwvga_dsi_vdo_holitech_pixi4_5",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id = lcm_compare_id,
};
