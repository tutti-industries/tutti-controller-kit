// ============================================================
//  CH552T コントローラー v4 マイクラコントローラー リリース版
// ============================================================
//  ボード設定（ツールメニューで必ず選択すること）
//    Board        : CH552
//    USB Settings : USER CODE w/ 148B USB ram
//  ※ USB Settings を「148B USB ram」以外にすると、コンパイル時に
//     USBhandler.c で
//       #error "This example needs more USB ram. Increase this setting in menu."
//     が出てビルドできない。HIDに 138B 必要なため 148B を選ぶ。
// ============================================================

#include "src/userUsbHidKeyboardMouse/USBHIDKeyboardMouse.h"

// ─── ジョイスティック設定 ──────────────────────────────────
#define JOY_CENTER    128   // 8bit ADC 中央値
#define JOY_DEADZONE   20   // 中央付近ノイズ除去
#define JOY_SCALE      8   // 感度(高いほど遅い)

// ADC入力ピン
#define JOY_X_PIN 11
#define JOY_Y_PIN 14


// 列ピン (OUTPUT)  col1=P3.5, col2=P1.3, col3=P1.2
#define COL_COUNT 3
__code uint8_t COL_PINS[COL_COUNT] = {35, 13, 12};

// 行ピン (INPUT_PULLUP)  row1=P1.0, row2=P1.7, row3=P1.6, row4=P1.5
#define ROW_COUNT 4
__code uint8_t ROW_PINS[ROW_COUNT] = {10, 17, 16, 15};

#define LED_PWM1 30
#define LED_PWM2 31
#define LED_IO   32

// ─── キーマップ [row][col] ─────────────────────────────────
//          col1(P3.5)            col2(P1.3)           col3(P1.2)
//  row1    SW1 Y  = Wheel Down   SW5  ← = A          SW9  Start  = Esc
//  row2    SW2 A  = Space        SW6  → = D          SW10 Select = E
//  row3    SW3 X  = Wheel Up     SW7  ↑ = W          SW11 RB     = Mouse Right
//  row4    SW4 B  = Shift        SW8  ↓ = S          SW12 LB     = Mouse Left
__code uint8_t KEY_MAP[ROW_COUNT][COL_COUNT] = {
    {0,                 'a',     KEY_ESC},
    {0,                 'd',     0    },
    {'E',               'w',     0    },
    {KEY_LEFT_SHIFT,    's',     ' '  },
};


// マウス左(0x01), マウス右(0x02)
__code uint8_t MOUSE_BTN_MAP[ROW_COUNT][COL_COUNT] = {
    {0,    0, 0},
    {0x02, 0, 0},
    {0,    0, 0x01},
    {0,    0, 0},
};

// 下スクロール(-1), 上スクロール(+1)
__code int8_t SCROLL_MAP[ROW_COUNT][COL_COUNT] = {
    {-1, 0,  0},
    {0, 0,  0},
    {0, 0, 0},
    {0, 0,  0},
};


uint8_t matrixState[ROW_COUNT][COL_COUNT];

void sendMouseBtn(uint8_t *prev, uint8_t current, uint8_t btn) {
    if (current == *prev) return;
    digitalWrite(LED_PWM2, HIGH);
    *prev = current;
    if (current) Mouse_press(btn);
    else         Mouse_release(btn);
    digitalWrite(LED_PWM2, LOW);
}

void sendKey(uint8_t *prev, uint8_t current, uint8_t key) { //PCへの情報送信
    if (current == *prev) return; //状態変化なし
    digitalWrite(LED_PWM1, HIGH); //変化イベント発生
    *prev = current;
    if (current) Keyboard_press(key);
    else         Keyboard_release(key);
    digitalWrite(LED_PWM1, LOW); //イベント終了
}

void setup() {
    uint8_t r, c;

    for (c = 0; c < COL_COUNT; c++) {
        pinMode(COL_PINS[c], OUTPUT);
        digitalWrite(COL_PINS[c], HIGH);
    }
    for (r = 0; r < ROW_COUNT; r++)
        pinMode(ROW_PINS[r], INPUT_PULLUP);

    for (r = 0; r < ROW_COUNT; r++)
        for (c = 0; c < COL_COUNT; c++)
            matrixState[r][c] = 0;
    pinMode(JOY_X_PIN, INPUT);
    pinMode(JOY_Y_PIN, INPUT);

    pinMode(LED_IO, OUTPUT);
    pinMode(LED_PWM1, OUTPUT); digitalWrite(LED_PWM1, LOW);
    pinMode(LED_PWM2, OUTPUT); digitalWrite(LED_PWM2, LOW);

    for (uint8_t i = 0; i < 3; i++) {
        digitalWrite(LED_IO, HIGH); delay(150);
        digitalWrite(LED_IO, LOW);  delay(150);
        digitalWrite(LED_PWM1, HIGH); delay(150);
        digitalWrite(LED_PWM1, LOW);  delay(150);
        digitalWrite(LED_PWM2, HIGH); delay(150);
        digitalWrite(LED_PWM2, LOW);  delay(150);
    }

    USBInit();
}

void loop() {
    uint8_t r, c;
    uint8_t anyPressed = 0;
    
    // ADC読み取り（0〜255 8bit）
    int joyX = analogRead(JOY_X_PIN);
    int joyY = analogRead(JOY_Y_PIN);
    // 中央との差分
    int dx = joyX - JOY_CENTER;
    int dy = joyY - JOY_CENTER;
    // デッドゾーン処理
    if (dx > -JOY_DEADZONE && dx < JOY_DEADZONE)
        dx = 0;
    if (dy > -JOY_DEADZONE && dy < JOY_DEADZONE)
        dy = 0;
    // 感度調整
    dx /= JOY_SCALE;
    dy /= JOY_SCALE;
    // マウス移動
    Mouse_move(dx, dy);

    //スイッチ読み取り
    for (c = 0; c < COL_COUNT; c++) {
        digitalWrite(COL_PINS[c], LOW);
        delayMicroseconds(20);
        for (r = 0; r < ROW_COUNT; r++) {
            uint8_t pressed = (digitalRead(ROW_PINS[r]) == LOW) ? 1 : 0; //SWが押されたらLOWにする
            if (pressed) {
                anyPressed = 1;
                digitalWrite(LED_IO, HIGH); //押された瞬間LED点灯
            }
            if (SCROLL_MAP[r][c] != 0) {
                if (pressed && !matrixState[r][c]) {
                    Mouse_scroll(SCROLL_MAP[r][c]);
                    Mouse_scroll(0);
                }
                matrixState[r][c] = pressed; //
            } else if (MOUSE_BTN_MAP[r][c]) {
                sendMouseBtn(&matrixState[r][c], pressed, MOUSE_BTN_MAP[r][c]);
            } else {
                sendKey(&matrixState[r][c], pressed, KEY_MAP[r][c]); //送信
            }
        }
        digitalWrite(COL_PINS[c], HIGH);
        delayMicroseconds(5);
    }
    if (!anyPressed) {
        digitalWrite(LED_IO, LOW);
    }

    delay(10);
}
