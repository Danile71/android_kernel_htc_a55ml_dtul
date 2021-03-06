/*
 * drivers/leds/leds-mt65xx.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * mt65xx leds driver
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/leds-mt65xx.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <mach/mt_pwm.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>
#include <leds_hal.h>
#include "leds_drv.h"


struct cust_mt65xx_led *bl_setting = NULL;
static unsigned int bl_brightness = 102;
static unsigned int bl_duty = 21;
static unsigned int bl_div = CLK_DIV1;
static unsigned int bl_frequency = 32000;
static unsigned int div_array[PWM_DIV_NUM];
static unsigned int current_blink;
struct mt65xx_led_data *g_leds_data[MT65XX_LED_TYPE_TOTAL];


static int debug_enable_led = 1;
#define LEDS_DRV_DEBUG(format, args...) do { \
	if (debug_enable_led) \
	{\
		printk(KERN_WARNING format, ##args);\
	} \
} while (0)


#define MT_LED_INTERNAL_LEVEL_BIT_CNT 10

#ifdef CONTROL_BL_TEMPERATURE
int setMaxbrightness(int max_level, int enable);
#else
int setMaxbrightness(int max_level, int enable) {}
#endif

#ifdef LED_INCREASE_LED_LEVEL_MTKPATCH
#define LED_INTERNAL_LEVEL_BIT_CNT 10
#endif


static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level);


#ifdef CONTROL_BL_TEMPERATURE

static unsigned int limit = 255;
static unsigned int limit_flag;
static unsigned int last_level;
static unsigned int current_level;
static DEFINE_MUTEX(bl_level_limit_mutex);
extern int disp_bls_set_max_backlight(unsigned int level);


int setMaxbrightness(int max_level, int enable)
{
#if !defined(CONFIG_MTK_AAL_SUPPORT)
	struct cust_mt65xx_led *cust_led_list = mt_get_cust_led_list();
	mutex_lock(&bl_level_limit_mutex);
	if (1 == enable) {
		limit_flag = 1;
		limit = max_level;
		mutex_unlock(&bl_level_limit_mutex);
		
		
		
		if (0 != current_level) {
			if (limit < last_level) {
				LEDS_DRV_DEBUG
				    ("mt65xx_leds_set_cust in setMaxbrightness:value control start! limit=%d\n",
				     limit);
				mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], limit);
			} else {
				
				mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD],
						    last_level);
			}
		}
	} else {
		limit_flag = 0;
		limit = 255;
		mutex_unlock(&bl_level_limit_mutex);
		
		

		if (last_level != 0) {
			if (0 != current_level) {
				LEDS_DRV_DEBUG("control temperature close:limit=%d\n", limit);
				mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], last_level);

				
			}
		}
	}

	

#else
	LEDS_DRV_DEBUG("setMaxbrightness go through AAL\n");
	disp_bls_set_max_backlight( ((((1 << LED_INTERNAL_LEVEL_BIT_CNT) - 1) * max_level +
                     127) / 255) );
#endif				
	return 0;

}
#endif
static void get_div_array(void)
{
	int i = 0;
	unsigned int *temp = mt_get_div_array();
	while (i < PWM_DIV_NUM) {
		div_array[i] = *temp++;
		LEDS_DRV_DEBUG("get_div_array: div_array=%d\n", div_array[i]);
		i++;
	}
}

static int led_set_pwm(int pwm_num, struct nled_setting *led)
{

	mt_led_set_pwm(pwm_num, led);
	return 0;
}

static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div)
{
	mt_brightness_set_pmic(pmic_type, level, div);
	return -1;

}

static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
{
#ifdef CONTROL_BL_TEMPERATURE
	mutex_lock(&bl_level_limit_mutex);
	current_level = level;
	
	if (0 == limit_flag) {
		last_level = level;
		
	} else {
		if (limit < current_level) {
			level = limit;
			
		}
	}
	mutex_unlock(&bl_level_limit_mutex);
#endif
#ifdef LED_INCREASE_LED_LEVEL_MTKPATCH
	if (MT65XX_LED_MODE_CUST_BLS_PWM == cust->mode) {
		mt_mt65xx_led_set_cust(cust,
				       ((((1 << LED_INTERNAL_LEVEL_BIT_CNT) - 1) * level +
					 127) / 255));
	} else {
		mt_mt65xx_led_set_cust(cust, level);
	}
#else
	mt_mt65xx_led_set_cust(cust, level);
#endif
	return -1;
}


static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
{
	struct mt65xx_led_data *led_data = container_of(led_cdev, struct mt65xx_led_data, cdev);
	int delay_on,delay_off;
	LEDS_DRV_DEBUG("[LED]dev name = %s, level = %d, last = %d\n",
					led_data->cust.name, level, led_data->level);
	if (strcmp(led_data->cust.name, "lcd-backlight") == 0) {
#ifdef CONTROL_BL_TEMPERATURE
		mutex_lock(&bl_level_limit_mutex);
		current_level = level;
		
		if (0 == limit_flag) {
			last_level = level;
			
		} else {
			if (limit < current_level) {
				level = limit;
				LEDS_DRV_DEBUG("backlight_set_cust: control level=%d\n", level);
			}
		}
		mutex_unlock(&bl_level_limit_mutex);
#endif
	}
#ifdef CONTROL_BRIGHTNESS_LEVEL
	else if(strcmp(led_data->cust.name, "amber") == 0 ||
			strcmp(led_data->cust.name, "green") == 0){
		if(level > 0 && led_data->cust.brightness_level > 0){
			delay_on = 1;
		    delay_off = 51-(led_data->cust.brightness_level/2);
			led_blink_set(led_cdev, &delay_on, &delay_off);
			return;
		}
	}
#endif
	mt_mt65xx_led_set(led_cdev, level);
}

static int mt65xx_blink_set(struct led_classdev *led_cdev,
			    unsigned long *delay_on, unsigned long *delay_off)
{
	if (mt_mt65xx_blink_set(led_cdev, delay_on, delay_off)) {
		return -1;
	} else {
		return 0;
	}
}

int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level)
{
	struct cust_mt65xx_led *cust_led_list = mt_get_cust_led_list();

	LEDS_DRV_DEBUG("[LED]#%d:%d\n", type, level);

	if (type < 0 || type >= MT65XX_LED_TYPE_TOTAL)
		return -1;

	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

	return mt65xx_led_set_cust(&cust_led_list[type], level);

}
EXPORT_SYMBOL(mt65xx_leds_brightness_set);


int backlight_brightness_set(int level)
{
	struct cust_mt65xx_led *cust_led_list = mt_get_cust_led_list();

	if (level > ((1 << MT_LED_INTERNAL_LEVEL_BIT_CNT) - 1) )
		level = ((1 << MT_LED_INTERNAL_LEVEL_BIT_CNT) - 1);
	else if (level < 0)
		level = 0;

	if(MT65XX_LED_MODE_CUST_BLS_PWM == cust_led_list[MT65XX_LED_TYPE_LCD].mode)
	{
	#ifdef CONTROL_BL_TEMPERATURE
		mutex_lock(&bl_level_limit_mutex);
		current_level = (level >> (MT_LED_INTERNAL_LEVEL_BIT_CNT - 8)); 
		if(0 == limit_flag){
			last_level = current_level;
		}else {
			if(limit < current_level){
				
				level = (limit << (MT_LED_INTERNAL_LEVEL_BIT_CNT - 8)) | (limit >> (16 - MT_LED_INTERNAL_LEVEL_BIT_CNT));
			}
		}
		mutex_unlock(&bl_level_limit_mutex);
	#endif

		return mt_mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], level);
	}
	else
	{
		return mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD],( level >> (MT_LED_INTERNAL_LEVEL_BIT_CNT - 8)) );
	}	

}

EXPORT_SYMBOL(backlight_brightness_set);
static ssize_t show_blink(struct device *dev, struct device_attribute *attr, char *buf)
{
    LEDS_DRV_DEBUG("[LED]get blink value is:%d\n", current_blink);
    return sprintf(buf, "%u\n", current_blink);
}

static ssize_t store_blink(struct device *dev, struct device_attribute *attr, const char *buf,
                size_t size)
{
    struct led_classdev *led_cdev;
    int val;
    val = -1;
    sscanf(buf, "%u", &val);
	LEDS_DRV_DEBUG("[LED]set blink mode start, blink mode is %d\n",val);
    if (val < -1 || val > 255)
        return -EINVAL;
    current_blink = val;
    led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	struct mt65xx_led_data *led_data = container_of(led_cdev, struct mt65xx_led_data, cdev);

    switch(val) {
        case 0:
            break;
        case 1:
            led_data->delay_on = 64;
            led_data->delay_off = 1936;
			queue_work(led_data->led_wq, &led_data->blink_work);
            break;
        case 2:
            led_data->delay_on = 64;
            led_data->delay_off = 1936;
            msleep(310);
			queue_work(led_data->led_wq, &led_data->blink_work);
            break;
        case 3:
            led_data->delay_on = 64;
            led_data->delay_off = 1936;
            msleep(1000);
			queue_work(led_data->led_wq, &led_data->blink_work);
            break;
        case 4:
            led_data->delay_on = 1000;
            led_data->delay_off = 1000;
			queue_work(led_data->led_wq, &led_data->blink_work);
            break;
        default:
            return -EINVAL;
    }
    return size;
}
static DEVICE_ATTR(blink, 0664, show_blink, store_blink);

void mt_mt65xx_led_blink_work(struct work_struct *work)
{
	struct mt65xx_led_data *led_data = container_of(work, struct mt65xx_led_data, blink_work);

	LEDS_DRV_DEBUG("[LED]mt_mt65xx_led_blink_work %s:%d\n", led_data->cust.name, led_data->level);
	led_blink_set(&led_data->cdev, &led_data->delay_on, &led_data->delay_off);
}

static ssize_t show_duty(struct device *dev, struct device_attribute *attr, char *buf)
{
	LEDS_DRV_DEBUG("[LED]get backlight duty value is:%d\n", bl_duty);
	return sprintf(buf, "%u\n", bl_duty);
}

static ssize_t store_duty(struct device *dev, struct device_attribute *attr, const char *buf,
			  size_t size)
{
	char *pvalue = NULL;
	unsigned int level = 0;
	size_t count = 0;
	bl_div = mt_get_bl_div();
	LEDS_DRV_DEBUG("set backlight duty start\n");
	level = simple_strtoul(buf, &pvalue, 10);
	count = pvalue - buf;
	if (*pvalue && isspace(*pvalue))
		count++;

	if (count == size) {

		if (bl_setting->mode == MT65XX_LED_MODE_PMIC) {
			
			if ((level >= 0) && (level <= 15)) {
				mt_brightness_set_pmic_duty_store((level * 17), bl_div);
			} else {
				LEDS_DRV_DEBUG
				    ("duty value is error, please select vaule from [0-15]!\n");
			}

		}

		else if (bl_setting->mode == MT65XX_LED_MODE_PWM) {
			if (level == 0) {
				mt_led_pwm_disable(bl_setting->data);
			} else if (level <= 64) {
				mt_backlight_set_pwm_duty(bl_setting->data, level, bl_div,
							  &bl_setting->config_data);
			}
		}

		mt_set_bl_duty(level);

	}

	return size;
}
static DEVICE_ATTR(duty, 0664, show_duty, store_duty);


static ssize_t store_setwledcon3(struct device *dev, struct device_attribute *attr, const char *buf,
                          size_t size)
{
	unsigned int mode = -1;
	unsigned int value = 0x0;

	LEDS_DRV_DEBUG("%s: input with %s\n", __FUNCTION__, buf);

	if (size == 0 || size > 64) {
		LEDS_DRV_DEBUG("%s: echo <mode:0-0x7> <value:0-0xFF> to this node\n", __FUNCTION__);
		return size;
	}

	sscanf(buf, " %x %x", &mode, &value);

	if (mode > 0x7) {
		LEDS_DRV_DEBUG("%s: echo <mode:0-0x7> <value:0-0xFF> to this node\n", __FUNCTION__);
		return size;
	}

	if (value > 0x3ff) {
		LEDS_DRV_DEBUG("%s: echo <mode:0-0x7> <value:0-0xFF> to this node\n", __FUNCTION__);
		return size;
	}

	LEDS_DRV_DEBUG("%s: mode:0x%X, value:0x%X", __FUNCTION__, mode, value);

	mt6332_upmu_set_rg_iwled_testmode2(mode & 0x1);
	mt6332_upmu_set_rg_iwled_testmode1((mode>>1) & 0x1);
	mt6332_upmu_set_rg_iwled_testmode0((mode>>2) & 0x1);
	mt6332_upmu_set_rg_iwled_step_sw(value);

	return size;
}
static DEVICE_ATTR(setwledcon3, 0220, NULL, store_setwledcon3);

static ssize_t show_div(struct device *dev, struct device_attribute *attr, char *buf)
{
	bl_div = mt_get_bl_div();
	LEDS_DRV_DEBUG("get backlight div value is:%d\n", bl_div);
	return sprintf(buf, "%u\n", bl_div);
}

static ssize_t store_div(struct device *dev, struct device_attribute *attr, const char *buf,
			 size_t size)
{
	char *pvalue = NULL;
	unsigned int div = 0;
	size_t count = 0;

	bl_duty = mt_get_bl_duty();
	LEDS_DRV_DEBUG("set backlight div start\n");
	div = simple_strtoul(buf, &pvalue, 10);
	count = pvalue - buf;

	if (*pvalue && isspace(*pvalue))
		count++;

	if (count == size) {
		if (div < 0 || (div > 7)) {
			LEDS_DRV_DEBUG("set backlight div parameter error: %d[div:0~7]\n", div);
			return 0;
		}

		if (bl_setting->mode == MT65XX_LED_MODE_PWM) {
			LEDS_DRV_DEBUG("set PWM backlight div OK: div=%d, duty=%d\n", div, bl_duty);
			mt_backlight_set_pwm_div(bl_setting->data, bl_duty, div,
						 &bl_setting->config_data);
		}

		else if (bl_setting->mode == MT65XX_LED_MODE_CUST_LCM) {
			bl_brightness = mt_get_bl_brightness();
			LEDS_DRV_DEBUG("set cust backlight div OK: div=%d, brightness=%d\n", div,
				       bl_brightness);
			((cust_brightness_set) (bl_setting->data)) (bl_brightness, div);
		}
		mt_set_bl_div(div);

	}

	return size;
}

static DEVICE_ATTR(div, 0664, show_div, store_div);


static ssize_t show_frequency(struct device *dev, struct device_attribute *attr, char *buf)
{
	bl_div = mt_get_bl_div();
	bl_frequency = mt_get_bl_frequency();

	if (bl_setting->mode == MT65XX_LED_MODE_PWM) {
		mt_set_bl_frequency(32000 / div_array[bl_div]);
	} else if (bl_setting->mode == MT65XX_LED_MODE_CUST_LCM) {
		
		mt_backlight_get_pwm_fsel(bl_div, &bl_frequency);
	}

	LEDS_DRV_DEBUG("[LED]get backlight PWM frequency value is:%d\n", bl_frequency);

	return sprintf(buf, "%u\n", bl_frequency);
}

static DEVICE_ATTR(frequency, 0444, show_frequency, NULL);



static ssize_t store_pwm_register(struct device *dev, struct device_attribute *attr,
				  const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;
	if (buf != NULL && size != 0) {
		
		reg_address = simple_strtoul(buf, &pvalue, 16);

		if (*pvalue && (*pvalue == '#')) {
			reg_value = simple_strtoul((pvalue + 1), NULL, 16);
			LEDS_DRV_DEBUG("set pwm register:[0x%x]= 0x%x\n", reg_address, reg_value);
			
			mt_store_pwm_register(reg_address, reg_value);

		} else if (*pvalue && (*pvalue == '@')) {
			LEDS_DRV_DEBUG("get pwm register:[0x%x]=0x%x\n", reg_address,
				       mt_show_pwm_register(reg_address));
		}
	}

	return size;
}

static ssize_t show_pwm_register(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static DEVICE_ATTR(pwm_register, 0664, show_pwm_register, store_pwm_register);

extern unsigned char get_camera_blk();
extern unsigned char get_camera_dua_blk();
extern unsigned char get_camera_rec_blk();

static ssize_t camera_bl_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
        ssize_t ret =0;
        ret = scnprintf(buf, PAGE_SIZE, "%s%u\n%s%u\n", "BL_CAM_MIN=", get_camera_blk(), "BL_CAM_DUA_MIN=", get_camera_dua_blk());

	if(get_camera_rec_blk())
		ret = scnprintf(buf, PAGE_SIZE, "%s%s%u\n", buf, "BL_CAM_REC_MIN=", get_camera_rec_blk());
        return ret;
}

static DEVICE_ATTR(backlight_info, S_IRUGO, camera_bl_show, NULL);

#ifdef CONTROL_BRIGHTNESS_LEVEL
static ssize_t show_brightness_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	struct mt65xx_led_data *led_data = container_of(led_cdev, struct mt65xx_led_data, cdev);
	LEDS_DRV_DEBUG("[LED]get brightness level value is:%d\n", led_data->cust.brightness_level );
    return sprintf(buf, "%u\n", led_data->cust.brightness_level );
}

static ssize_t store_brightness_level(struct device *dev, struct device_attribute *attr, const char *buf,
                size_t size)
{
    struct led_classdev *led_cdev;
	struct mt65xx_led_data *led_data;
    int val;
    unsigned long delay_on;
    unsigned long delay_off;
    val = -1;
    sscanf(buf, "%u", &val);
	LEDS_DRV_DEBUG("[LED]Write brightness level as value: %d\n",val);
    if (val < 0 || val > 100){
		LEDS_DRV_DEBUG("[LED]brightness level should be in range 0~100\n");
        return -EINVAL;
    }
    led_cdev = (struct led_classdev *) dev_get_drvdata(dev);
	led_data = container_of(led_cdev, struct mt65xx_led_data, cdev);
	led_data->cust.brightness_level = val;
    mt65xx_led_set(led_cdev, 1);
    return size;
}

static DEVICE_ATTR(brightness_level, 0664, show_brightness_level, store_brightness_level);
#endif


static int __init mt65xx_leds_probe(struct platform_device *pdev)
{
	int i;
	int ret, rc;
	struct cust_mt65xx_led *cust_led_list = mt_get_cust_led_list();
	LEDS_DRV_DEBUG("[LED]%s\n", __func__);
	get_div_array();
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (cust_led_list[i].mode == MT65XX_LED_MODE_NONE) {
			g_leds_data[i] = NULL;
			continue;
		}

		g_leds_data[i] = kzalloc(sizeof(struct mt65xx_led_data), GFP_KERNEL);
		if (!g_leds_data[i]) {
			ret = -ENOMEM;
			goto err;
		}

		g_leds_data[i]->cust.mode = cust_led_list[i].mode;
		g_leds_data[i]->cust.data = cust_led_list[i].data;
		g_leds_data[i]->cust.name = cust_led_list[i].name;

		g_leds_data[i]->cdev.name = cust_led_list[i].name;
		g_leds_data[i]->cust.config_data = cust_led_list[i].config_data;	
#ifdef CONTROL_BRIGHTNESS_LEVEL
		g_leds_data[i]->cust.brightness_level= cust_led_list[i].brightness_level;
#endif
		g_leds_data[i]->cdev.brightness_set = mt65xx_led_set;
		g_leds_data[i]->cdev.blink_set = mt65xx_blink_set;

		INIT_WORK(&g_leds_data[i]->work, mt_mt65xx_led_work);

		ret = led_classdev_register(&pdev->dev, &g_leds_data[i]->cdev);

		if (strcmp(g_leds_data[i]->cdev.name, "lcd-backlight") == 0) {
			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_duty);
			if (rc) {
				LEDS_DRV_DEBUG("[LED]device_create_file duty fail!\n");
			}

			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_div);
			if (rc) {
				LEDS_DRV_DEBUG("[LED]device_create_file duty fail!\n");
			}

			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_frequency);
			if (rc) {
				LEDS_DRV_DEBUG("[LED]device_create_file duty fail!\n");
			}

			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_pwm_register);
			if (rc) {
				LEDS_DRV_DEBUG("[LED]device_create_file duty fail!\n");
			}

			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_backlight_info);
			if (rc) {
				LEDS_DRV_DEBUG("[LED]device_create_file backlight_info fail!\n");
			}

			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_setwledcon3);
			if (rc) {
				LEDS_DRV_DEBUG("[LED]device_create_file setwledcon3 fail!\n");
			}

			bl_setting = &g_leds_data[i]->cust;
		}
		else{
			g_leds_data[i]->led_wq = create_singlethread_workqueue(g_leds_data[i]->cdev.name);
			if(g_leds_data[i]->led_wq == NULL)
				ret = -1;
		}
        if (strcmp(g_leds_data[i]->cdev.name, "amber") == 0) {
			g_leds_data[i]->level = -1;
			INIT_WORK(&g_leds_data[i]->blink_work, mt_mt65xx_led_blink_work);
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_blink);
            if (rc) {
                LEDS_DRV_DEBUG("[LED] %s device_create_file blink fail!\n", g_leds_data[i]->cdev.name);
            }
#ifdef CONTROL_BRIGHTNESS_LEVEL
			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_brightness_level);
            if (rc) {
                LEDS_DRV_DEBUG("[LED] %s device_create_file brightness_level fail!\n", g_leds_data[i]->cdev.name);
            }
#endif
        }
        if (strcmp(g_leds_data[i]->cdev.name, "green") == 0) {
			g_leds_data[i]->level = -1;
			INIT_WORK(&g_leds_data[i]->blink_work, mt_mt65xx_led_blink_work);
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_blink);
            if (rc) {
                LEDS_DRV_DEBUG("[LED] %s device_create_file blink fail!\n", g_leds_data[i]->cdev.name);
            }
#ifdef CONTROL_BRIGHTNESS_LEVEL
			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_brightness_level);
            if (rc) {
                LEDS_DRV_DEBUG("[LED] %s device_create_file brightness_level fail!\n", g_leds_data[i]->cdev.name);
            }
#endif
        }
		if (ret)
			goto err;

	}
#ifdef CONTROL_BL_TEMPERATURE

	last_level = 0;
	limit = 255;
	limit_flag = 0;
	current_level = 0;
	LEDS_DRV_DEBUG
	    ("[LED]led probe last_level = %d, limit = %d, limit_flag = %d, current_level = %d\n",
	     last_level, limit, limit_flag, current_level);
#endif


	return 0;

 err:
	if (i) {
		for (i = i - 1; i >= 0; i--) {
			if (!g_leds_data[i])
				continue;
			if(strcmp(g_leds_data[i]->cdev.name, "lcd-backlight") != 0)
				destroy_workqueue(g_leds_data[i]->led_wq);
			led_classdev_unregister(&g_leds_data[i]->cdev);
			cancel_work_sync(&g_leds_data[i]->work);
			kfree(g_leds_data[i]);
			g_leds_data[i] = NULL;
		}
	}

	return ret;
}

static int mt65xx_leds_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		if(strcmp(g_leds_data[i]->cdev.name, "lcd-backlight") != 0)
			destroy_workqueue(g_leds_data[i]->led_wq);
		led_classdev_unregister(&g_leds_data[i]->cdev);
		cancel_work_sync(&g_leds_data[i]->work);
		kfree(g_leds_data[i]);
		g_leds_data[i] = NULL;
	}

	return 0;
}


static void mt65xx_leds_shutdown(struct platform_device *pdev)
{
	int i;
	struct nled_setting led_tmp_setting = { NLED_OFF, 0, 0 };

	LEDS_DRV_DEBUG("[LED]%s\n", __func__);
	LEDS_DRV_DEBUG("[LED]mt65xx_leds_shutdown: turn off backlight\n");

	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		switch (g_leds_data[i]->cust.mode) {

		case MT65XX_LED_MODE_PWM:
			if (strcmp(g_leds_data[i]->cust.name, "lcd-backlight") == 0) {
				
				
				mt_led_pwm_disable(g_leds_data[i]->cust.data);
			} else {
				led_set_pwm(g_leds_data[i]->cust.data, &led_tmp_setting);
			}
			break;

			
			
			

		case MT65XX_LED_MODE_PMIC:
			brightness_set_pmic(g_leds_data[i]->cust.data, 0, 0);
			break;
		case MT65XX_LED_MODE_CUST_LCM:
			LEDS_DRV_DEBUG("[LED]backlight control through LCM!!1\n");
			((cust_brightness_set) (g_leds_data[i]->cust.data)) (0, bl_div);
			break;
		case MT65XX_LED_MODE_CUST_BLS_PWM:
			LEDS_DRV_DEBUG("[LED]backlight control through BLS!!1\n");
			((cust_set_brightness) (g_leds_data[i]->cust.data)) (0);
			break;
		case MT65XX_LED_MODE_NONE:
		default:
			break;
		}
	}

}

static struct platform_driver mt65xx_leds_driver = {
	.driver = {
		   .name = "leds-mt65xx",
		   .owner = THIS_MODULE,
		   },
	.probe = mt65xx_leds_probe,
	.remove = mt65xx_leds_remove,
	
	.shutdown = mt65xx_leds_shutdown,
};

#ifdef CONFIG_OF
static struct platform_device mt65xx_leds_device = {
	.name = "leds-mt65xx",
	.id = -1
};

#endif

static int __init mt65xx_leds_init(void)
{
	int ret;

	LEDS_DRV_DEBUG("[LED]%s\n", __func__);

#ifdef CONFIG_OF
	ret = platform_device_register(&mt65xx_leds_device);
	if (ret)
		printk("[LED]mt65xx_leds_init:dev:E%d\n", ret);
#endif
	ret = platform_driver_register(&mt65xx_leds_driver);

	if (ret) {
		LEDS_DRV_DEBUG("[LED]mt65xx_leds_init:drv:E%d\n", ret);
		return ret;
	}

	mt_leds_wake_lock_init();

	return ret;
}

static void __exit mt65xx_leds_exit(void)
{
	platform_driver_unregister(&mt65xx_leds_driver);
}

module_param(debug_enable_led, int, 0644);

module_init(mt65xx_leds_init);
module_exit(mt65xx_leds_exit);

MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("LED driver for MediaTek MT65xx chip");
MODULE_LICENSE("GPL");
MODULE_ALIAS("leds-mt65xx");
