#ifndef _KD_CAMERA_HW_H_
#define _KD_CAMERA_HW_H_
 

#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#include "pmic_drv.h"


#define CAMERA_POWER_VCAM_A         PMIC_APP_MAIN_CAMERA_POWER_A
#define CAMERA_POWER_VCAM_D         PMIC_APP_MAIN_CAMERA_POWER_D
#define CAMERA_POWER_VCAM_A2        PMIC_APP_MAIN_CAMERA_POWER_AF
#define CAMERA_POWER_VCAM_D2        PMIC_APP_MAIN_CAMERA_POWER_IO
#define SUB_CAMERA_POWER_VCAM_D     PMIC_APP_SUB_CAMERA_POWER_D


#define CAMERA_CMRST_PIN            GPIO_CAMERA_INVALID
#define CAMERA_CMRST_PIN_M_GPIO     GPIO_CAMERA_INVALID

#define CAMERA_CMPDN_PIN            GPIO_CAMERA_INVALID
#define CAMERA_CMPDN_PIN_M_GPIO     GPIO_CAMERA_INVALID
 
#define CAMERA_CMRST1_PIN           GPIO_CAMERA_INVALID
#define CAMERA_CMRST1_PIN_M_GPIO    GPIO_CAMERA_INVALID

#define CAMERA_CMPDN1_PIN           GPIO_CAMERA_INVALID
#define CAMERA_CMPDN1_PIN_M_GPIO    GPIO_CAMERA_INVALID


#define CAMERA_CMRST2_PIN           GPIO_CAMERA_INVALID
#define CAMERA_CMRST2_PIN_M_GPIO    GPIO_CAMERA_INVALID

#define CAMERA_CMPDN2_PIN           GPIO_CAMERA_INVALID
#define CAMERA_CMPDN2_PIN_M_GPIO    GPIO_CAMERA_INVALID

#define SUPPORT_I2C_BUS_NUM1        0
#define SUPPORT_I2C_BUS_NUM2        2

#define MAIN_CAM_USE_I2C_NUM    SUPPORT_I2C_BUS_NUM1
#define SUB_CAM_USE_I2C_NUM      SUPPORT_I2C_BUS_NUM2
#define MAIN2_CAM_USE_I2C_NUM    SUPPORT_I2C_BUS_NUM2
#endif 
