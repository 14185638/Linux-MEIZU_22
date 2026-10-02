# SPDX-License-Identifier: GPL-2.0
# define_trace.h needs to know how to find our header
CFLAGS_trace.o				:= -I$(src)
CFLAGS_dwc3-msm-core.o			:= -I$(srctree)/drivers/usb/host

obj-$(CONFIG_USB_DWC3)			+= dwc3.o

dwc3-y					:= core.o

ifneq ($(CONFIG_TRACING),)
	dwc3-y				+= trace.o
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_HOST) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= host.o
endif

ifneq ($(filter y,$(CONFIG_USB_DWC3_GADGET) $(CONFIG_USB_DWC3_DUAL_ROLE)),)
	dwc3-y				+= gadget.o ep0.o
endif

ifneq ($(CONFIG_USB_DWC3_DUAL_ROLE),)
	dwc3-y				+= drd.o
endif

ifneq ($(CONFIG_USB_DWC3_ULPI),)
	dwc3-y				+= ulpi.o
endif

ifneq ($(CONFIG_DEBUG_FS),)
	dwc3-y				+= debugfs.o
endif

# M2582 bring-up: the vendor dwc3-msm glue replaces mainline's dwc3-qcom here.
# It performs the clock/reset/DBM sequence that lets this core's DCTL.CSFTRST
# actually clear.
obj-$(CONFIG_USB_DWC3_MSM)	+= dwc3-msm.o
# debug_ipc.o kept for the IPC log helpers; dwc3-msm-ops.o is the Android
# kprobe USB-state reporter and depends on CONFIG_ANDROID_USB_CONFIGFS_UEVENT,
# which mainline does not have. It is not needed for CDC ACM.
dwc3-msm-y				:= dwc3-msm-core.o debug_ipc.o dwc3-msm-m2582-stubs.o
