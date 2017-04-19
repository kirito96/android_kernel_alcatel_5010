
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "kd_flashlight.h"
/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */
#define TAG_NAME "[sub_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_WARN(fmt, arg...)        pr_warn(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __func__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __func__ , ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a, ...)
#define PK_VER(a, ...)
#define PK_ERR(a, ...)
#endif


//add by allenyao
#if 0
 enum mt6580_led_pmic {
	MT65XX_LED_PMIC_LCD_ISINK = 0,
	MT65XX_LED_PMIC_NLED_ISINK0,
	MT65XX_LED_PMIC_NLED_ISINK1,
	MT65XX_LED_PMIC_NLED_ISINK2,
	MT65XX_LED_PMIC_NLED_ISINK3
};
extern int mt_brightness_set_pmic(enum mt6580_led_pmic pmic_type, u32 level, u32 div);
#endif


static DEFINE_SPINLOCK(g_strobeSMPLock);	/* cotta-- SMP proection */
static u32 strobe_Res;
static struct work_struct workTimeOut;
static int g_timeOutTimeMs;
static int g_duty = -1;
static BOOL g_strobe_On;



extern int mtkcam_flash_en(int val);
extern int mtkcam_flashstrobe_swich(int val);
extern int  flash_isink_init(void);
extern int flash_isink_enable(void);
extern int  flash_isink_disable(void);



static  int FL_Init(void){
	flash_isink_init();
	return 0;
}




static int FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" FL_dim_duty line=%d\n", __LINE__);
	g_duty = duty;
	return 0;
}

static int FL_Enable(void)
{
	//int ret=0;
	flash_isink_enable();
	//ret = mtkcam_flash_en(1);
	if(g_duty<=1){
		//mtkcam_flashstrobe_swich(0);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK1, 1, 1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK2, 1, 1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK3, 1, 1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK1, 255, 1);
	}else{
		//mtkcam_flashstrobe_swich(1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK1, 1, 1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK2, 1, 1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK3, 1, 1);
		//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK1, 255, 1);
	}

	return 0;
}

static int FL_Disable(void)
{
	//int ret=0;
	flash_isink_disable();
	//ret = mtkcam_flash_en(0);
	//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK1, 0, 1);
	//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK2, 0, 1);
	//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK3, 1, 1);
	//mt_brightness_set_pmic(MT65XX_LED_PMIC_NLED_ISINK1, 255, 1);
	return 0;
}

static int FL_Uninit(void)
{
	FL_Disable();
	return 0;
}



static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

static enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

static struct hrtimer g_timeOutTimer;
static void timerInit(void)
{
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	g_timeOutTimeMs = 1000;
	hrtimer_init(&g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_timeOutTimer.function = ledTimeOutCallback;
}

//end

static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	//PK_DBG("sub dummy ioctl");
	//add by allenyao
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	printk("sub dummy ioctl,allenyao,%d\n",__LINE__);

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC, 0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC, 0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC, 0, int));
/*	PK_DBG
	    ("LM3642 constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);
*/

	printk
	    ("LM3642 constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",
	     __LINE__, ior_shift, iow_shift, iowr_shift, (int)arg);

	switch (cmd) {

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		printk("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n", (int)arg);
		g_timeOutTimeMs = arg;
		break;


	case FLASH_IOC_SET_DUTY:
		printk("FLASHLIGHT_DUTY: %d\n", (int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n", (int)arg);

		break;

	case FLASH_IOC_SET_ONOFF:
		printk("FLASHLIGHT_ONOFF: %d\n", (int)arg);
		if (arg == 1) {

			int s;
			int ms;

			if (g_timeOutTimeMs > 1000) {
				s = g_timeOutTimeMs / 1000;
				ms = g_timeOutTimeMs - s * 1000;
			} else {
				s = 0;
				ms = g_timeOutTimeMs;
			}

			if (g_timeOutTimeMs != 0) {
				ktime_t ktime;

				ktime = ktime_set(s, ms * 1000000);
				hrtimer_start(&g_timeOutTimer, ktime, HRTIMER_MODE_REL);
			}
			FL_Enable();
		} else {
			FL_Disable();
			hrtimer_cancel(&g_timeOutTimer);
		}
		break;
	default:
		PK_DBG(" No such command\n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
	//end
	return 0;
}

static int sub_strobe_open(void *pArg)
{
	//PK_DBG("sub dummy open");
	//add by allenyao
	int i4RetValue = 0;

	printk("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res) {
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if (strobe_Res) {
		PK_DBG(" busy!\n");
		i4RetValue = -EBUSY;
	} else {
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;
	//end
	return 0;

}

static int sub_strobe_release(void *pArg)
{
	//PK_DBG("sub dummy release");
	printk("sub dummy open,allenyao,%d\n",__LINE__);
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res) {
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		//strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;
	return 0;

}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}
