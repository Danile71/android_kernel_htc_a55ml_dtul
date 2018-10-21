include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y += ov13850mipiraw_Sensor.o

ifeq ($(CONFIG_ARCH_MTK_PROJECT),"a55ml")
ccflags-y += -DA55ML_CAMERA=1
endif

