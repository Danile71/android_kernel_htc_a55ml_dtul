include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-$(CONFIG_MTK_I2C) := i2c.o i2c_common.o

ifeq ($(CONFIG_ARCH_MTK_PROJECT),"a55ml")
ccflags-y += -DA55ML_CAMERA=1
endif

