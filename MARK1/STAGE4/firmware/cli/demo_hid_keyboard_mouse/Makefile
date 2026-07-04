all : flash

TARGET:=demo_hid_keyboard_mouse
TARGET_EXT:=cpp  # メインファイルが .cpp
CH32FUN:=../ch32v003fun/ch32fun
TARGET_MCU:=CH32V003

ADDITIONAL_C_FILES+=../rv003usb/rv003usb.S ../rv003usb/rv003usb.c TinyUSB_Mouse_Keyboard.cpp
EXTRA_CFLAGS:=-I../lib -I../rv003usb

include $(CH32FUN)/ch32fun.mk

# g++ に差し替え
CC:=$(PREFIX)-g++

flash : cv_flash
clean : cv_clean
