// ============================================================
//  UIAPduino Pro Micro CH32V003 キーボード+マウス v3
//  ch32fun + rv003usb (nak435/UIAPduino-demo_hid_keyboard_mouse ベース)
// ============================================================
//  ビルド: make clean && make build
//  書込み: minichlink -C b803boot -w demo_hid_keyboard_mouse.bin flash -b
// ------------------------------------------------------------
//  v3: KEY_MAP を自由に書き換えても壊れないよう堅牢化
//   - KEY_MAP を uint16_t 化。マウス操作コードは 0x100 以降にして
//     キーボードコード(KEY_F13=0xF0 等)との衝突を解消
//   - LED は KEY_MAP から自動分類（LED_MAP の手動更新は不要）
//   - ホイール連射の状態をセルごとに保持（複数セル割当OK）
//   - ダイオードレスマトリクスのゴースト(ロの字4隅目)をブロック
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
#define LED1 GPIOv_from_PORT_PIN(GPIO_port_D, 2)  // PD2 - マウス系キー押下で点灯
#define LED2 GPIOv_from_PORT_PIN(GPIO_port_C, 3)  // PC3 - 印字キー押下で点灯
#define LED3 GPIOv_from_PORT_PIN(GPIO_port_C, 0)  // PC0 - 修飾・特殊キー押下で点灯

// マウス操作コード
// キーボードコードは 0x00-0xFF を使い切っている(例: KEY_F13=0xF0)ため、
// 0x100 以降に配置して衝突を防ぐ。KEY_MAP は uint16_t であること。
#define KEY_MOUSE_BTN_LEFT   0x100
#define KEY_MOUSE_BTN_RIGHT  0x101
#define KEY_MOUSE_BTN_MIDDLE 0x102
#define KEY_MOUSE_WHEEL_UP   0x103
#define KEY_MOUSE_WHEEL_DN   0x104

// ホイール連射パラメータ（スキャン周期 ~13ms 単位）
#define WHEEL_TRIGGER   4   // 押下がこの回数続いたら発火
#define WHEEL_COOLDOWN 12   // 発火後の待ち ~150ms

// ─── キーマップ [row][col] ─────────────────────────────────
//  ここを自由に書き換えてよい:
//   - ASCII文字 ('a', 'A', '!', ' ' など。大文字・記号はShift自動付与)
//   - TinyUSB_Mouse_Keyboard.h の KEY_* (KEY_ESC, KEY_LEFT_SHIFT, KEY_F13 など)
//   - 上記の KEY_MOUSE_* (マウスボタン/ホイール)
//   - 0 = 未割当
//  同じキーを複数セルに割り当てても正しく動作する。
//
//             col1(PD0)             col2(PD5/TX)        col3(PD6/RX)
//  row1(6)    SW1 Y  = Wheel Down   SW5  ← = A          SW9  Start  = Esc
//  row2(7)    SW2 A  = Space        SW6  → = D          SW10 Select = E
//  row3(8)    SW3 X  = Wheel Up     SW7  ↑ = W          SW11 RB     = Mouse Right
//  row4(9)    SW4 B  = Shift        SW8  ↓ = S          SW12 LB     = Mouse Left
const uint16_t KEY_MAP[ROW_COUNT][COL_COUNT] = {
    {KEY_MOUSE_WHEEL_DN, 'a', KEY_ESC            },
    {' ',                'd', 'e'                },
    {KEY_MOUSE_WHEEL_UP, 'w', KEY_MOUSE_BTN_RIGHT},
    {KEY_LEFT_SHIFT,     's', KEY_MOUSE_BTN_LEFT },
};

// LED分類をキーコードから自動判定: 1=マウス系(LED1) 2=印字キー(LED2) 3=修飾・特殊キー(LED3)
static uint8_t ledClassOf(uint16_t key) {
    if (key >= 0x100) return 1;               // KEY_MOUSE_*
    if (key >= 0x20 && key < 0x80) return 2;  // 印字可能ASCII
    return 3;                                 // 修飾キー・KEY_*特殊キー
}

uint8_t matrixState[ROW_COUNT][COL_COUNT];        // 受理済みの押下状態
static uint8_t wheelCnt[ROW_COUNT][COL_COUNT];    // ホイール: 押下継続カウント(セルごと)
static uint8_t wheelCd[ROW_COUNT][COL_COUNT];     // ホイール: クールダウン(セルごと)

// ダイオードレスマトリクスのゴースト対策:
// (r,c) の新規押下が、受理済み押下とロの字(長方形)の4隅目を作るなら
// ゴーストの可能性があり真偽を判別できない → その押下は保留する。
// (該当する3キー同時押しの組合せを多用するならダイオード追加で根本解決)
static bool ghostRisk(int r, int c) {
    for (int r2 = 0; r2 < ROW_COUNT; r2++) {
        if (r2 == r || !matrixState[r2][c]) continue;
        for (int c2 = 0; c2 < COL_COUNT; c2++) {
            if (c2 == c) continue;
            if (matrixState[r][c2] && matrixState[r2][c2]) return true;
        }
    }
    return false;
}

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
        for (int c = 0; c < COL_COUNT; c++) {
            matrixState[r][c] = 0;
            wheelCnt[r][c] = 0;
            wheelCd[r][c] = 0;
        }
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

    Keyboard.begin();
    Mouse.begin();
    Delay_Ms(1);
    usb_setup();

    while (1) {
        // 1) マトリクス全体を読み取り
        uint8_t raw[ROW_COUNT][COL_COUNT];
        for (int c = 0; c < COL_COUNT; c++) {
            GPIO_digitalWrite_lo(COL_PINS[c]);
            Delay_Ms(1);
            for (int r = 0; r < ROW_COUNT; r++)
                raw[r][c] = (GPIO_digitalRead(ROW_PINS[r]) == low) ? 1 : 0;
            GPIO_digitalWrite_hi(COL_PINS[c]);
        }

        // 2) キーイベント適用
        for (int r = 0; r < ROW_COUNT; r++) {
            for (int c = 0; c < COL_COUNT; c++) {
                uint16_t key = KEY_MAP[r][c];
                if (key == 0) continue;
                uint8_t pressed = raw[r][c];

                // ゴーストの可能性がある新規押下は保留（解放は常に受理）
                if (pressed && !matrixState[r][c] && ghostRisk(r, c)) pressed = 0;

                switch (key) {
                case KEY_MOUSE_BTN_LEFT:
                case KEY_MOUSE_BTN_RIGHT:
                case KEY_MOUSE_BTN_MIDDLE: {
                    uint8_t btn = (key == KEY_MOUSE_BTN_LEFT)  ? MOUSE_LEFT :
                                  (key == KEY_MOUSE_BTN_RIGHT) ? MOUSE_RIGHT : MOUSE_MIDDLE;
                    if (pressed != matrixState[r][c]) {
                        matrixState[r][c] = pressed;
                        if (pressed) Mouse.press(btn);
                        else         Mouse.release(btn);
                    }
                    break;
                }
                case KEY_MOUSE_WHEEL_UP:
                case KEY_MOUSE_WHEEL_DN: {
                    if (wheelCd[r][c] > 0) wheelCd[r][c]--;
                    if (pressed) {
                        if (wheelCnt[r][c] < WHEEL_TRIGGER) wheelCnt[r][c]++;
                        if (wheelCnt[r][c] >= WHEEL_TRIGGER && wheelCd[r][c] == 0) {
                            Mouse.move(0, 0, (key == KEY_MOUSE_WHEEL_UP) ? 1 : -1);
                            wheelCd[r][c] = WHEEL_COOLDOWN;
                        }
                    } else {
                        wheelCnt[r][c] = 0;
                    }
                    matrixState[r][c] = pressed;
                    break;
                }
                default:
                    sendKey(&matrixState[r][c], pressed, (uint8_t)key);
                    break;
                }
            }
        }

        // 3) LED更新（キー種別は KEY_MAP から自動判定）
        uint8_t led1 = 0, led2 = 0, led3 = 0;
        for (int r = 0; r < ROW_COUNT; r++) {
            for (int c = 0; c < COL_COUNT; c++) {
                if (!matrixState[r][c]) continue;
                uint8_t cls = ledClassOf(KEY_MAP[r][c]);
                if (cls == 1) led1 = 1;
                else if (cls == 2) led2 = 1;
                else led3 = 1;
            }
        }
        if (led1) GPIO_digitalWrite_hi(LED1); else GPIO_digitalWrite_lo(LED1);
        if (led2) GPIO_digitalWrite_hi(LED2); else GPIO_digitalWrite_lo(LED2);
        if (led3) GPIO_digitalWrite_hi(LED3); else GPIO_digitalWrite_lo(LED3);

        // 4) ジョイスティック → マウス移動
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
