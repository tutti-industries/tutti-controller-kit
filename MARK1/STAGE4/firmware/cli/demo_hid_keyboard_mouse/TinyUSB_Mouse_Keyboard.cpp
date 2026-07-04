#include "TinyUSB_Mouse_Keyboard.h"
#include <string.h>
extern "C" {
#include "funconfig.h"
#include "rv003usb.h"
}

#if FUNCONF_USE_US_KEYBOARD == 1
    #pragma message("US keyboard was specified.")
#elif FUNCONF_USE_JIS_KEYBOARD == 1
    #pragma message("JIS keyboard was specified.")
#else
    #error("No keyboard type was specified.")
#endif

mouse_internal_t mouse_internal;
kbd_internal_t kbd_internal;

// ASCII -> HID変換テーブル
typedef struct { uint8_t mod; uint8_t key; } hid_key_t;
#if FUNCONF_USE_US_KEYBOARD == 1
    #include "US_Keyboard.h"
#elif FUNCONF_USE_JIS_KEYBOARD == 1
    #include "JIS_Keyboard.h"
#else
    static const hid_key_t ascii_to_hid[128] = { {0, 0} };
#endif

void TinyKeyboard_::begin() { memset(&kbd_internal, 0, sizeof(kbd_internal)); }

void TinyKeyboard_::_push_report(uint8_t step) {
    int next = (kbd_internal.head + 1) % 16;
    // 【待ち合わせ】バッファが空くまでループ
    while (next == kbd_internal.tail);

    kbd_internal.current.step = step;
    kbd_internal.buffer[kbd_internal.head] = kbd_internal.current;
    kbd_internal.head = next;
}

size_t TinyKeyboard_::press(uint8_t k) {
    if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    } else if (k >= 0x80) { // it's a modifier key
        kbd_internal.current.modifiers |= (1 << (k - 0x80));
        k = 0;
    } else {
        hid_key_t h = ascii_to_hid[k];
        if (h.key & h.mod) {   // it's a capital letter or other character reached with shift
            kbd_internal.current.modifiers |= 0x02; // the left shift modifier
        }
        k = h.key;
    }
    for (int i=0; i<6; i++) {
        if (kbd_internal.current.key_codes[i] == 0) {
            kbd_internal.current.key_codes[i] = k;
            break;
        }
    }

    _push_report(0);
    return 1;
}

size_t TinyKeyboard_::release(uint8_t k) {
    if (k >= 136) { // it's a non-printing key (not a modifier)
        k = k - 136;
    } else if (k >= 0x80) { // it's a modifier key
        kbd_internal.current.modifiers &= ~(1 << (k - 0x80));
        k = 0;
    } else {
        hid_key_t h = ascii_to_hid[k];
        if (h.key & h.mod) {   // it's a capital letter or other character reached with shift
            kbd_internal.current.modifiers |= 0x02; // the left shift modifier
        }
        k = h.key;
    }

    for (int i=0; i<6; i++) {
        if (kbd_internal.current.key_codes[i] == k) kbd_internal.current.key_codes[i] = 0;
    }

    _push_report(0);
    return 1;
}

void TinyKeyboard_::releaseAll() {
    memset(&kbd_internal.current, 0, sizeof(kbd_report_t));
    _push_report(0);
}

size_t TinyKeyboard_::write(uint8_t k) {
    if (k >= 128) return 0;
    hid_key_t h = ascii_to_hid[k];
    kbd_report_t prev = kbd_internal.current;
    kbd_internal.current.modifiers = h.mod << 1; // if 1, left shift
    memset(kbd_internal.current.key_codes, 0, 6);
    kbd_internal.current.key_codes[0] = h.key;
    _push_report(1); // 送信後に自動で離す
    kbd_internal.current = prev;
    return 1;
}

size_t TinyKeyboard_::print(const char* s) {
    size_t n = 0;
    while (*s) n += write(*s++);
    return n;
}

size_t TinyKeyboard_::println(const char* s) {
    size_t n = print(s);
    return n + write('\n');
}

void TinyMouse_::begin() { memset(&mouse_internal, 0, sizeof(mouse_internal)); }

void TinyMouse_::move(int8_t x, int8_t y, int8_t wheel, int8_t pan) {
    while (mouse_internal.changed); // 【待ち合わせ】前回の送信完了を待つ
    mouse_internal.x = x;
    mouse_internal.y = y;
    mouse_internal.wheel = wheel;
    mouse_internal.pan = pan;
    mouse_internal.changed = true;
}

void TinyMouse_::press(uint8_t b) {
    while (mouse_internal.changed);
    mouse_internal.buttons |= b;
    mouse_internal.changed = true;
}

void TinyMouse_::release(uint8_t b) {
    while (mouse_internal.changed);
    mouse_internal.buttons &= ~b;
    mouse_internal.changed = true;
}

void TinyMouse_::click(uint8_t b) { press(b); release(b); }

TinyMouse_ Mouse;
TinyKeyboard_ Keyboard;


void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
	if (len > 0) {
		LogUEvent(1139, data[0], 0, current_endpoint);
	}
}


void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist ) {
    if( endp == 1 ) { // Mouse
        if( mouse_internal.changed ) {
            uint8_t report[5] = { 
                mouse_internal.buttons, 
                (uint8_t)mouse_internal.x, 
                (uint8_t)mouse_internal.y, 
                (uint8_t)mouse_internal.wheel,
                (uint8_t)mouse_internal.pan 
            };
            // 5バイト送信
            usb_send_data( report, 5, 0, sendtok );
            mouse_internal.x = 0;
            mouse_internal.y = 0;
            mouse_internal.wheel = 0;
            mouse_internal.pan = 0;
            mouse_internal.changed = false; // 送信完了フラグを落とす
        } else {
            usb_send_empty( sendtok );
        }
    }
    else if( endp == 2 ) { // Keyboard
        static bool release_step = false;
        if( kbd_internal.head != kbd_internal.tail ) {
            kbd_report_t *m = &kbd_internal.buffer[kbd_internal.tail];
            if( release_step ) {
                uint8_t empty[8] = {0};
                usb_send_data( empty, 8, 0, sendtok );
                release_step = false;
                kbd_internal.tail = (kbd_internal.tail + 1) % 16;
            } else {
                uint8_t report[8] = { m->modifiers, 0, m->key_codes[0], m->key_codes[1], m->key_codes[2], m->key_codes[3], m->key_codes[4], m->key_codes[5] };
                usb_send_data( report, 8, 0, sendtok );
                if( m->step == 1 ) release_step = true;
                else kbd_internal.tail = (kbd_internal.tail + 1) % 16;
            }
        } else {
            usb_send_empty( sendtok );
        }
    }
}
