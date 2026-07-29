TARGET:=demo_hid_keyboard_mouse_led
TARGET_EXT:=cpp

# STAGE4 の共通USB/HIDスタックとビルド環境を利用する。
STAGE4_CLI:=../../../STAGE4/firmware/cli
CH32FUN:=$(STAGE4_CLI)/ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=$(STAGE4_CLI)/rv003usb/rv003usb.S $(STAGE4_CLI)/rv003usb/rv003usb.c $(STAGE4_CLI)/demo_hid_keyboard_mouse/TinyUSB_Mouse_Keyboard.cpp
EXTRA_CFLAGS:=-I$(STAGE4_CLI)/lib -I$(STAGE4_CLI)/rv003usb -I$(STAGE4_CLI)/demo_hid_keyboard_mouse

include $(CH32FUN)/ch32fun.mk

CC:=$(PREFIX)-g++

flash : cv_flash
clean : cv_clean
