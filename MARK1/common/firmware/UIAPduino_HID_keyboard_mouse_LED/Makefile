TARGET:=demo_hid_keyboard_mouse_led
TARGET_EXT:=cpp

# common/firmware 内の共通USB/HIDスタックとビルド環境を利用する。
FIRMWARE_ROOT:=..
VENDOR:=$(FIRMWARE_ROOT)/vendor
HID_SHARED:=$(FIRMWARE_ROOT)/shared/hid
CH32FUN:=$(VENDOR)/ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=$(VENDOR)/rv003usb/rv003usb.S $(VENDOR)/rv003usb/rv003usb.c $(HID_SHARED)/TinyUSB_Mouse_Keyboard.cpp
EXTRA_CFLAGS:=-I$(VENDOR)/lib -I$(VENDOR)/rv003usb -I$(HID_SHARED)

include $(CH32FUN)/ch32fun.mk

CC:=$(PREFIX)-g++

flash : cv_flash
clean : cv_clean
