#ifndef TINYUSB_MOUSE_KEYBOARD_H
#define TINYUSB_MOUSE_KEYBOARD_H

#include <stdint.h>
#include <stddef.h>

// Mouse button 定義
#define MOUSE_LEFT      1
#define MOUSE_RIGHT     2
#define MOUSE_MIDDLE    4

// 1つのHIDレポート状態
typedef struct {
    uint8_t modifiers;
    uint8_t key_codes[6];
    uint8_t step; // 0:送信のみ, 1:送信後に自動でAll-0(離す)を送る
} kbd_report_t;

class TinyMouse_ {
public:
    void begin();
    void move(int8_t x, int8_t y, int8_t wheel = 0, int8_t pan = 0);
    void press(uint8_t b = MOUSE_LEFT);
    void release(uint8_t b = MOUSE_LEFT);
    void click(uint8_t b = MOUSE_LEFT);
};

class TinyKeyboard_ {
public:
    void begin();
    size_t write(uint8_t k);
    size_t print(const char* s);
    size_t println(const char* s);
    size_t press(uint8_t k);
    size_t release(uint8_t k);
    void releaseAll();
private:
    void _push_state();
    void _enqueue(const kbd_report_t* r, uint8_t step);
};

extern TinyMouse_ Mouse;
extern TinyKeyboard_ Keyboard;

// rv003usbから参照するための内部構造体宣言
extern struct mouse_internal_t {
    int8_t x, y, wheel, pan;
    uint8_t buttons;
    volatile bool changed;
} mouse_internal;

extern struct kbd_internal_t {
    kbd_report_t buffer[16]; // バッファサイズ
    volatile int head;
    volatile int tail;
    kbd_report_t current;
    uint8_t key_ref[6];   // key_codes[i] の押下参照カウント（同一キーの多重割当対応）
    uint8_t mod_ref[8];   // 修飾キー(0x80-0x87)の押下参照カウント
    uint8_t auto_shift;   // Shift必須ASCIIキーの押下数（>0なら左Shiftを合成）
} kbd_internal;

extern "C" {
    void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist );
    void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist );
}

//  Keyboard codes

#define KEY_LEFT_CTRL       0x80
#define KEY_LEFT_SHIFT      0x81
#define KEY_LEFT_ALT        0x82
#define KEY_LEFT_GUI        0x83
#define KEY_LEFT_WINDOWS    0x83    //alias for Windows
#define KEY_LEFT_COMMAND    0x83    //alias for macOS
#define KEY_RIGHT_CTRL      0x84
#define KEY_RIGHT_SHIFT     0x85
#define KEY_RIGHT_ALT       0x86
#define KEY_RIGHT_GUI       0x87
#define KEY_RIGHT_WINDOWS   0x87    //alias for Windows
#define KEY_RIGHT_COMMAND   0x87    //alias for macOS

#define KEY_RETURN          0xB0
#define KEY_ESC             0xB1
#define KEY_BACKSPACE       0xB2
#define KEY_TAB             0xB3
#define KEY_CAPS_LOCK       0xC1
#define KEY_F1              0xC2
#define KEY_F2              0xC3
#define KEY_F3              0xC4
#define KEY_F4              0xC5
#define KEY_F5              0xC6
#define KEY_F6              0xC7
#define KEY_F7              0xC8
#define KEY_F8              0xC9
#define KEY_F9              0xCA
#define KEY_F10             0xCB
#define KEY_F11             0xCC
#define KEY_F12             0xCD
#define KEY_F13             0xF0
#define KEY_F14             0xF1
#define KEY_F15             0xF2
#define KEY_F16             0xF3
#define KEY_F17             0xF4
#define KEY_F18             0xF5
#define KEY_F19             0xF6
#define KEY_F20             0xF7
#define KEY_F21             0xF8
#define KEY_F22             0xF9
#define KEY_F23             0xFA
#define KEY_F24             0xFB
#define KEY_INSERT          0xD1
#define KEY_HOME            0xD2
#define KEY_PAGE_UP         0xD3
#define KEY_DELETE          0xD4
#define KEY_END             0xD5
#define KEY_PAGE_DOWN       0xD6
#define KEY_RIGHT_ARROW     0xD7
#define KEY_LEFT_ARROW      0xD8
#define KEY_DOWN_ARROW      0xD9
#define KEY_UP_ARROW        0xDA        
#define KEY_KEYPAD_DIVIDE   0xDC
#define KEY_KEYPAD_MULTIPLY 0xDD
#define KEY_KEYPAD_SUBTRACT 0xDE
#define KEY_KEYPAD_ADD      0xDF
#define KEY_KEYPAD_ENTER    0xE0
#define KEY_KEYPAD_1        0xE1
#define KEY_KEYPAD_2        0xE2
#define KEY_KEYPAD_3        0xE3
#define KEY_KEYPAD_4        0xE4
#define KEY_KEYPAD_5        0xE5
#define KEY_KEYPAD_6        0xE6
#define KEY_KEYPAD_7        0xE7
#define KEY_KEYPAD_8        0xE8
#define KEY_KEYPAD_9        0xE9
#define KEY_KEYPAD_0        0xEA
#define KEY_KEYPAD_DECIMAL  0xEB
#define KEY_KEYPAD_EQUAL    0xEF


#endif
