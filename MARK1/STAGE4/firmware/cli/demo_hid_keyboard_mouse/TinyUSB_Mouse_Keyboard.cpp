#include "TinyUSB_Mouse_Keyboard.h"
#include <string.h>
extern "C" {
#include "funconfig.h"
#include "ch32fun.h"
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

// ── Keyboard ─────────────────────────────────────────────────

void TinyKeyboard_::begin() { memset(&kbd_internal, 0, sizeof(kbd_internal)); }

// リングバッファへ1レポート積む。
// 満杯時は最大~5msだけ待ち、それでも満杯なら最新エントリへ上書き合流する
// （USBサスペンド中・ケーブル半抜け時に無限ループで固まるのを防止。
//   中間遷移は失われるが最終的なキー状態は必ず一致する）。
void TinyKeyboard_::_enqueue(const kbd_report_t* r, uint8_t step) {
    int next = (kbd_internal.head + 1) % 16;
    for (int t = 0; t < 500 && next == kbd_internal.tail; t++) Delay_Us(10);
    if (next == kbd_internal.tail) {
        int last = (kbd_internal.head + 15) % 16;
        if (step == 0 && kbd_internal.buffer[last].step == 0) {
            kbd_internal.buffer[last] = *r;
            kbd_internal.buffer[last].step = 0;
        }
        return; // step=1(write系)はドロップ
    }
    kbd_internal.buffer[kbd_internal.head] = *r;
    kbd_internal.buffer[kbd_internal.head].step = step;
    kbd_internal.head = next;
}

// 現在の押下状態から modifiers を合成して送信キューへ積む
void TinyKeyboard_::_push_state() {
    uint8_t mods = 0;
    for (int i = 0; i < 8; i++)
        if (kbd_internal.mod_ref[i]) mods |= (1 << i);
    if (kbd_internal.auto_shift) mods |= 0x02; // 左Shiftを自動付与
    kbd_internal.current.modifiers = mods;
    _enqueue(&kbd_internal.current, 0);
}

size_t TinyKeyboard_::press(uint8_t k) {
    uint8_t shift = 0;
    if (k >= 136) {              // 非印字キー（修飾キー以外）
        k -= 136;
    } else if (k >= 0x80) {      // 修飾キー: 参照カウントで管理（多重割当対応）
        uint8_t i = k - 0x80;
        if (kbd_internal.mod_ref[i] < 255) kbd_internal.mod_ref[i]++;
        _push_state();
        return 1;
    } else {                     // ASCII文字
        hid_key_t h = ascii_to_hid[k];
        shift = h.mod;           // このキーはShiftが必要か
        k = h.key;
    }
    if (k == 0) return 0;        // 変換できない文字は無視

    // 既に押下中なら参照カウントのみ増やす（同一キーを複数セルに割当てても安全）
    int slot = -1, empty = -1;
    for (int i = 0; i < 6; i++) {
        if (kbd_internal.current.key_codes[i] == k) { slot = i; break; }
        if (empty < 0 && kbd_internal.current.key_codes[i] == 0) empty = i;
    }
    if (slot >= 0) {
        if (kbd_internal.key_ref[slot] < 255) kbd_internal.key_ref[slot]++;
    } else if (empty >= 0) {
        kbd_internal.current.key_codes[empty] = k;
        kbd_internal.key_ref[empty] = 1;
    } else {
        return 0;                // 6キー超過: このキーは破棄（shiftも数えない）
    }
    if (shift && kbd_internal.auto_shift < 255) kbd_internal.auto_shift++;
    _push_state();
    return 1;
}

size_t TinyKeyboard_::release(uint8_t k) {
    uint8_t shift = 0;
    if (k >= 136) {
        k -= 136;
    } else if (k >= 0x80) {
        uint8_t i = k - 0x80;
        if (kbd_internal.mod_ref[i]) kbd_internal.mod_ref[i]--;
        _push_state();
        return 1;
    } else {
        hid_key_t h = ascii_to_hid[k];
        shift = h.mod;
        k = h.key;
    }
    if (k == 0) return 0;

    for (int i = 0; i < 6; i++) {
        if (kbd_internal.current.key_codes[i] == k) {
            if (kbd_internal.key_ref[i]) kbd_internal.key_ref[i]--;
            if (kbd_internal.key_ref[i] == 0) kbd_internal.current.key_codes[i] = 0;
            if (shift && kbd_internal.auto_shift) kbd_internal.auto_shift--;
            _push_state();
            return 1;
        }
    }
    return 0;                    // 押されていないキーのreleaseは無視
}

void TinyKeyboard_::releaseAll() {
    memset(&kbd_internal.current, 0, sizeof(kbd_report_t));
    memset(kbd_internal.key_ref, 0, sizeof(kbd_internal.key_ref));
    memset(kbd_internal.mod_ref, 0, sizeof(kbd_internal.mod_ref));
    kbd_internal.auto_shift = 0;
    _push_state();
}

size_t TinyKeyboard_::write(uint8_t k) {
    if (k >= 128) return 0;
    hid_key_t h = ascii_to_hid[k];
    if (h.key == 0) return 0;
    kbd_report_t rpt;
    memset(&rpt, 0, sizeof(rpt));
    rpt.modifiers = h.mod ? 0x02 : 0; // Shiftが必要な文字なら左Shift
    rpt.key_codes[0] = h.key;
    _enqueue(&rpt, 1); // 送信後に自動で離す
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

// ── Mouse ────────────────────────────────────────────────────

void TinyMouse_::begin() { memset(&mouse_internal, 0, sizeof(mouse_internal)); }

// 8bit飽和加算
static int8_t sat_add8(int8_t a, int8_t b) {
    int v = (int)a + (int)b;
    if (v >  127) v =  127;
    if (v < -127) v = -127;
    return (int8_t)v;
}

// 送信待ちの移動量へ加算合成する。
// 旧実装の busy-wait (while(changed);) を廃止 → USBサスペンド中でも固まらない。
void TinyMouse_::move(int8_t x, int8_t y, int8_t wheel, int8_t pan) {
    __disable_irq(); // USB割り込みとの競合防止（数命令なのでUSBタイミングに影響なし）
    mouse_internal.x     = sat_add8(mouse_internal.x, x);
    mouse_internal.y     = sat_add8(mouse_internal.y, y);
    mouse_internal.wheel = sat_add8(mouse_internal.wheel, wheel);
    mouse_internal.pan   = sat_add8(mouse_internal.pan, pan);
    mouse_internal.changed = true;
    __enable_irq();
}

void TinyMouse_::press(uint8_t b) {
    __disable_irq();
    mouse_internal.buttons |= b;
    mouse_internal.changed = true;
    __enable_irq();
}

void TinyMouse_::release(uint8_t b) {
    __disable_irq();
    mouse_internal.buttons &= ~b;
    mouse_internal.changed = true;
    __enable_irq();
}

void TinyMouse_::click(uint8_t b) {
    press(b);
    // pressが1レポートとして送信されるのを待つ（最大~20msで諦める）
    for (int t = 0; t < 2000 && mouse_internal.changed; t++) Delay_Us(10);
    release(b);
}

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
