// ============================================================
//  UIAPduino Pro Micro CH32V003 キーボード+マウス v2
//  ch32fun + rv003usb (nak435/UIAPduino-demo_hid_keyboard_mouse ベース)
// ============================================================
//  ビルド: make clean && make build
//  書込み: minichlink -C b803boot -w demo_hid_keyboard_mouse.bin flash -b
// ============================================================

#include <stdio.h>
#include <string.h>
extern "C" {
    #include "ch32fun.h"
    #include "rv003usb.h"
    #include "ch32v003_GPIO_branchless.h"
    #include "TinyUSB_Mouse_Keyboard.h"
}

// ── ADC (ジョイスティック) ────────────────────────────────────
// PA2 = GPIO_Ain0_A2 = X軸、PA1 = GPIO_Ain1_A1 = Y軸
#define JOY_DEADZONE  25
#define JOY_SCALE     16

static int8_t clamp8(int v) {
    if (v >  127) return  127;
    if (v < -128) return -128;
    return (int8_t)v;
}

// 列ピン (OUTPUT): PD0 / PD5 / PD6
#define COL_COUNT 3
const uint8_t COL_PINS[COL_COUNT] = {
    GPIOv_from_PORT_PIN(GPIO_port_D, 0),
    GPIOv_from_PORT_PIN(GPIO_port_D, 5),
    GPIOv_from_PORT_PIN(GPIO_port_D, 6),
};

// 行ピン (INPUT_PULLUP): PC4 / PC5 / PC6 / PC7
#define ROW_COUNT 4
const uint8_t ROW_PINS[ROW_COUNT] = {
    GPIOv_from_PORT_PIN(GPIO_port_C, 4),
    GPIOv_from_PORT_PIN(GPIO_port_C, 5),
    GPIOv_from_PORT_PIN(GPIO_port_C, 6),
    GPIOv_from_PORT_PIN(GPIO_port_C, 7),
};

// LED
#define LED1 GPIOv_from_PORT_PIN(GPIO_port_D, 2)  // PD2 - スイッチ押下時点灯
#define LED2 GPIOv_from_PORT_PIN(GPIO_port_C, 3)  // PC3
#define LED3 GPIOv_from_PORT_PIN(GPIO_port_C, 0)  // PC0

// マウス特殊コード
#define KEY_MOUSE_BTN_LEFT  0xF0
#define KEY_MOUSE_BTN_RIGHT 0xF1
#define KEY_MOUSE_WHEEL_UP  0xF2
#define KEY_MOUSE_WHEEL_DN  0xF3

// ─── キーマップ [row][col] ─────────────────────────────────
//             col1(PD0)             col2(PD5/TX)        col3(PD6/RX)
//  row1(6)    SW1 Y  = Wheel Down   SW5  ← = A          SW9  Start  = Esc
//  row2(7)    SW2 A  = Space        SW6  → = D          SW10 Select = E
//  row3(8)    SW3 X  = Wheel Up     SW7  ↑ = W          SW11 RB     = Mouse Right
//  row4(9)    SW4 B  = Shift        SW8  ↓ = S          SW12 LB     = Mouse Left
const uint8_t KEY_MAP[ROW_COUNT][COL_COUNT] = {
    {KEY_MOUSE_WHEEL_DN, 'a', KEY_ESC            },
    {' ',                'd', 'e'                },
    {KEY_MOUSE_WHEEL_UP, 'w', KEY_MOUSE_BTN_RIGHT},
    {KEY_LEFT_SHIFT,     's', KEY_MOUSE_BTN_LEFT },
};

// LED分類: 1=マウス(LED1) 2=文字キー(LED2) 3=特殊キー(LED3)
const uint8_t LED_MAP[ROW_COUNT][COL_COUNT] = {
    {1, 2, 3},  // wheel_dn, a, esc
    {3, 2, 2},  // space, d, e
    {1, 2, 1},  // wheel_up, w, mouse_right
    {3, 2, 1},  // shift, s, mouse_left
};

uint8_t matrixState[ROW_COUNT][COL_COUNT];

void sendKey(uint8_t *prev, uint8_t current, uint8_t key) {
    if (current == *prev) return;
    *prev = current;
    if (current) Keyboard.press(key);
    else         Keyboard.release(key);
}

int main() {
    SystemInit();

    GPIO_port_enable(GPIO_port_C);
    GPIO_port_enable(GPIO_port_D);

    // LED
    GPIO_pinMode(LED1, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_pinMode(LED2, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_pinMode(LED3, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_digitalWrite_lo(LED1);
    GPIO_digitalWrite_lo(LED2);
    GPIO_digitalWrite_lo(LED3);

    // 列: OUTPUT HIGH
    for (int c = 0; c < COL_COUNT; c++) {
        GPIO_pinMode(COL_PINS[c], GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
        GPIO_digitalWrite_hi(COL_PINS[c]);
    }

    // 行: INPUT PULLUP
    for (int r = 0; r < ROW_COUNT; r++) {
        GPIO_pinMode(ROW_PINS[r], GPIO_pinMode_I_pullUp, GPIO_Speed_In);
        for (int c = 0; c < COL_COUNT; c++)
            matrixState[r][c] = 0;
    }

    // 起動演出: ナイトライダー風 (1→2→3→2→1) × 3往復
    const uint8_t LEDS[3] = {LED1, LED2, LED3};
    for (int rep = 0; rep < 3; rep++) {
        for (int i = 0; i < 3; i++) {
            GPIO_digitalWrite_hi(LEDS[i]); Delay_Ms(80);
            GPIO_digitalWrite_lo(LEDS[i]); Delay_Ms(20);
        }
        for (int i = 1; i >= 0; i--) {
            GPIO_digitalWrite_hi(LEDS[i]); Delay_Ms(80);
            GPIO_digitalWrite_lo(LEDS[i]); Delay_Ms(20);
        }
    }
    // 最後に全点灯→消灯
    for (int i = 0; i < 3; i++) GPIO_digitalWrite_hi(LEDS[i]);
    Delay_Ms(200);
    for (int i = 0; i < 3; i++) GPIO_digitalWrite_lo(LEDS[i]);

    // ADC初期化（GPIO_branchlessライブラリ使用）
    GPIO_port_enable(GPIO_port_A);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 2), GPIO_pinMode_I_analog, GPIO_Speed_In);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 1), GPIO_pinMode_I_analog, GPIO_Speed_In);
    GPIO_ADCinit();

    // 起動時センターキャリブレーション（中央で電源ON）
    int sum_x = 0, sum_y = 0;
    for (int i = 0; i < 50; i++) {
        sum_x += GPIO_analogRead(GPIO_Ain0_A2);
        sum_y += GPIO_analogRead(GPIO_Ain1_A1);
        Delay_Ms(1);
    }
    int center_x = sum_x / 50;
    int center_y = sum_y / 50;

    Delay_Ms(1);
    usb_setup();

    while (1) {
        uint8_t led1 = 0, led2 = 0, led3 = 0;

        for (int c = 0; c < COL_COUNT; c++) {
            GPIO_digitalWrite_lo(COL_PINS[c]);
            Delay_Ms(1);
            for (int r = 0; r < ROW_COUNT; r++) {
                uint8_t pressed = (GPIO_digitalRead(ROW_PINS[r]) == low) ? 1 : 0;
                uint8_t key = KEY_MAP[r][c];

                if (pressed) {
                    uint8_t cls = LED_MAP[r][c];
                    if (cls == 1) led1 = 1;
                    else if (cls == 2) led2 = 1;
                    else if (cls == 3) led3 = 1;
                }

                if (key == KEY_MOUSE_BTN_LEFT) {
                    if (pressed != matrixState[r][c]) {
                        matrixState[r][c] = pressed;
                        if (pressed) Mouse.press(MOUSE_LEFT);
                        else         Mouse.release(MOUSE_LEFT);
                    }
                } else if (key == KEY_MOUSE_BTN_RIGHT) {
                    if (pressed != matrixState[r][c]) {
                        matrixState[r][c] = pressed;
                        if (pressed) Mouse.press(MOUSE_RIGHT);
                        else         Mouse.release(MOUSE_RIGHT);
                    }
                } else if (key == KEY_MOUSE_WHEEL_UP) {
                    static uint8_t cnt = 0;
                    static uint8_t cooldown = 0;
                    if (cooldown > 0) cooldown--;
                    if (pressed) {
                        if (++cnt >= 4 && cooldown == 0) {
                            Mouse.move(0, 0, 1);
                            cooldown = 12;  // ~150ms
                            cnt = 4;
                        }
                    } else { cnt = 0; }
                    matrixState[r][c] = pressed;
                } else if (key == KEY_MOUSE_WHEEL_DN) {
                    static uint8_t cnt = 0;
                    static uint8_t cooldown = 0;
                    if (cooldown > 0) cooldown--;
                    if (pressed) {
                        if (++cnt >= 4 && cooldown == 0) {
                            Mouse.move(0, 0, -1);
                            cooldown = 12;  // ~150ms
                            cnt = 4;
                        }
                    } else { cnt = 0; }
                    matrixState[r][c] = pressed;
                } else if (key) {
                    sendKey(&matrixState[r][c], pressed, key);
                }
            }
            GPIO_digitalWrite_hi(COL_PINS[c]);
        }

        if (led1) GPIO_digitalWrite_hi(LED1); else GPIO_digitalWrite_lo(LED1);
        if (led2) GPIO_digitalWrite_hi(LED2); else GPIO_digitalWrite_lo(LED2);
        if (led3) GPIO_digitalWrite_hi(LED3); else GPIO_digitalWrite_lo(LED3);

        // ジョイスティック → マウス移動
        int dx = GPIO_analogRead(GPIO_Ain0_A2) - center_x;
        int dy = GPIO_analogRead(GPIO_Ain1_A1) - center_y;
        if (dx > -JOY_DEADZONE && dx < JOY_DEADZONE) dx = 0;
        if (dy > -JOY_DEADZONE && dy < JOY_DEADZONE) dy = 0;
        int8_t mx = clamp8(dx / JOY_SCALE);
        int8_t my = clamp8(dy / JOY_SCALE);
        if (mx || my) Mouse.move(mx, my);

        Delay_Ms(10);
    }
}
