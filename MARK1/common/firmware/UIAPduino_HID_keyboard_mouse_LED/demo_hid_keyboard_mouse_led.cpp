// UIAPduino Pro Micro CH32V003: HID keyboard/mouse + idle LED animation.
// The USB/HID stack is provided by this common firmware package.

#include <stdint.h>

extern "C" {
#include "ch32fun.h"
#include "rv003usb.h"
#include "ch32v003_GPIO_branchless.h"
}
#include "TinyUSB_Mouse_Keyboard.h"

// ---- Hardware assignment -------------------------------------------------

// Joystick: PA2 = X, PA1 = Y.
#define JOY_DEADZONE 25
#define JOY_SCALE    16

// 3 columns x 4 rows matrix.
#define COL_COUNT 3
static const uint8_t COL_PINS[COL_COUNT] = {
    GPIOv_from_PORT_PIN(GPIO_port_D, 0),
    GPIOv_from_PORT_PIN(GPIO_port_D, 5),
    GPIOv_from_PORT_PIN(GPIO_port_D, 6),
};

#define ROW_COUNT 4
static const uint8_t ROW_PINS[ROW_COUNT] = {
    GPIOv_from_PORT_PIN(GPIO_port_C, 4),
    GPIOv_from_PORT_PIN(GPIO_port_C, 5),
    GPIOv_from_PORT_PIN(GPIO_port_C, 6),
    GPIOv_from_PORT_PIN(GPIO_port_C, 7),
};

// LED1: PD2, LED2: PC3 (TIM1_CH3), LED3: PC0 (TIM2_CH3 / onboard LED).
#define LED1 GPIOv_from_PORT_PIN(GPIO_port_D, 2)
#define LED2 GPIOv_from_PORT_PIN(GPIO_port_C, 3)
#define LED3 GPIOv_from_PORT_PIN(GPIO_port_C, 0)

// Mouse actions use values outside the keyboard-code range.
#define KEY_MOUSE_BTN_LEFT   0x100
#define KEY_MOUSE_BTN_RIGHT  0x101
#define KEY_MOUSE_BTN_MIDDLE 0x102
#define KEY_MOUSE_WHEEL_UP   0x103
#define KEY_MOUSE_WHEEL_DN   0x104

#define WHEEL_TRIGGER  4
#define WHEEL_COOLDOWN 12

// Change only this table to change the controller mapping.
// 0 means the switch is intentionally unused.
static const uint16_t KEY_MAP[ROW_COUNT][COL_COUNT] = {
    {KEY_MOUSE_WHEEL_DN, 'a', KEY_ESC            },
    {' ',                'd', 'e'                },
    {KEY_MOUSE_WHEEL_UP, 'w', KEY_MOUSE_BTN_RIGHT},
    {KEY_LEFT_SHIFT,     's', KEY_MOUSE_BTN_LEFT },
};

// ---- General helpers -----------------------------------------------------

static int8_t clamp8(int value) {
    if (value > 127) return 127;
    if (value < -128) return -128;
    return (int8_t)value;
}

static bool joystickMoved(int dx, int dy) {
    return dx <= -JOY_DEADZONE || dx >= JOY_DEADZONE ||
           dy <= -JOY_DEADZONE || dy >= JOY_DEADZONE;
}

static uint8_t ledClassOf(uint16_t key) {
    if (key >= 0x100) return 1;              // mouse action -> LED1
    if (key >= 0x20 && key < 0x80) return 2; // printable ASCII -> LED2
    return 3;                                // modifier / special -> LED3
}

// If matrix diodes are absent, reject the fourth corner of a 3-key rectangle.
static bool ghostRisk(uint8_t state[ROW_COUNT][COL_COUNT], int r, int c) {
    for (int r2 = 0; r2 < ROW_COUNT; r2++) {
        if (r2 == r || !state[r2][c]) continue;
        for (int c2 = 0; c2 < COL_COUNT; c2++) {
            if (c2 != c && state[r][c2] && state[r2][c2]) return true;
        }
    }
    return false;
}

static void scanMatrix(uint8_t raw[ROW_COUNT][COL_COUNT]) {
    for (int c = 0; c < COL_COUNT; c++) {
        GPIO_digitalWrite_lo(COL_PINS[c]);
        Delay_Ms(1);
        for (int r = 0; r < ROW_COUNT; r++) {
            raw[r][c] = (GPIO_digitalRead(ROW_PINS[r]) == low) ? 1 : 0;
        }
        GPIO_digitalWrite_hi(COL_PINS[c]);
    }
}

static bool anyMatrixKey(const uint8_t raw[ROW_COUNT][COL_COUNT]) {
    for (int r = 0; r < ROW_COUNT; r++) {
        for (int c = 0; c < COL_COUNT; c++) {
            if (KEY_MAP[r][c] != 0 && raw[r][c]) return true;
        }
    }
    return false;
}

// ---- Idle LED animation --------------------------------------------------
// These functions use only PC0/PC3 and TIM1/TIM2.  USB and matrix pins are
// deliberately untouched; avoid broad GPIO reinitialization here.

static void ledsOff(void) {
    GPIO_digitalWrite_lo(LED1);
    GPIO_digitalWrite_lo(LED2);
    GPIO_digitalWrite_lo(LED3);
}

static void writeLed(uint8_t pin, uint8_t on) {
    if (on) GPIO_digitalWrite_hi(pin);
    else GPIO_digitalWrite_lo(pin);
}

static uint32_t pwmPscForFreq(uint32_t freqHz) {
    uint32_t psc = FUNCONF_SYSTEM_CORE_CLOCK / (freqHz * 256UL);
    return psc ? psc - 1 : 0;
}

static void tim1PwmStart(uint32_t freqHz) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1;
    GPIOC->CFGLR &= ~(0xf << (4 * 3));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 3);

    RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
    RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;
    TIM1->PSC = pwmPscForFreq(freqHz);
    TIM1->ATRLR = 255;
    TIM1->SWEVGR |= TIM_UG;
    TIM1->CCER |= TIM_CC3E | TIM_CC3P;
    TIM1->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1;
    TIM1->CH3CVR = 128;
    TIM1->BDTR |= TIM_MOE;
    TIM1->CTLR1 |= TIM_CEN;
}

static void tim1PwmStop(void) {
    TIM1->CH3CVR = 0;
    TIM1->CTLR1 &= ~TIM_CEN;
    TIM1->BDTR &= ~TIM_MOE;
    GPIO_pinMode(LED2, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_digitalWrite_lo(LED2);
}

static void tim2PwmStart(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    RCC->APB1PCENR |= RCC_APB1Periph_TIM2;
    GPIOC->CFGLR &= ~(0xf << (4 * 0));
    GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 0);

    RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
    RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;
    TIM2->PSC = pwmPscForFreq(200);
    TIM2->ATRLR = 255;
    TIM2->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1 | TIM_OC3PE;
    TIM2->CTLR1 |= TIM_ARPE;
    TIM2->CCER |= TIM_CC3E | TIM_CC3P;
    TIM2->SWEVGR |= TIM_UG;
    TIM2->CTLR1 |= TIM_CEN;
}

static void tim2PwmStop(void) {
    TIM2->CH3CVR = 0;
    TIM2->CTLR1 &= ~TIM_CEN;
    GPIO_pinMode(LED3, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_digitalWrite_lo(LED3);
}

enum IdlePhase : uint8_t { IDLE_LAMP_TEST, IDLE_LED1_BLINK, IDLE_LED2_SWEEP, IDLE_LED3_FADE };
static IdlePhase idlePhase = IDLE_LAMP_TEST;
static uint32_t idleElapsed = 0;
static uint8_t idleStep = 0;
static bool idlePwmStarted = false;

static void idleEnter(IdlePhase phase) {
    tim1PwmStop();
    tim2PwmStop();
    ledsOff();
    idlePhase = phase;
    idleElapsed = 0;
    idleStep = 0;
    idlePwmStarted = false;
}

static void idleAdvance(void) {
    switch (idlePhase) {
    case IDLE_LAMP_TEST:
        // Four 250ms on/off flashes.
        if (((idleElapsed / 250) & 1) == 0) {
            GPIO_digitalWrite_hi(LED1);
            GPIO_digitalWrite_hi(LED2);
            GPIO_digitalWrite_hi(LED3);
        } else {
            ledsOff();
        }
        if (idleElapsed >= 2000) idleEnter(IDLE_LED1_BLINK);
        break;

    case IDLE_LED1_BLINK: {
        // 2Hz, 5Hz, 10Hz: each for two seconds.
        static const uint16_t halfPeriodMs[] = {250, 100, 50};
        uint8_t step = idleElapsed / 2000;
        if (step >= 3) {
            idleEnter(IDLE_LED2_SWEEP);
            break;
        }
        writeLed(LED1, ((idleElapsed / halfPeriodMs[step]) & 1) == 0);
        break;
    }

    case IDLE_LED2_SWEEP: {
        static const uint16_t freqs[] = {10, 20, 40, 80, 1000};
        uint8_t step = idleElapsed / 2000;
        if (step >= 5) {
            idleEnter(IDLE_LED3_FADE);
            break;
        }
        if (!idlePwmStarted || step != idleStep) {
            if (!idlePwmStarted) tim1PwmStart(freqs[step]);
            else {
                TIM1->PSC = pwmPscForFreq(freqs[step]);
                TIM1->SWEVGR |= TIM_UG;
            }
            idlePwmStarted = true;
            idleStep = step;
        }
        break;
    }

    case IDLE_LED3_FADE: {
        // 10-second triangle fade, refreshed every 10ms.
        if (!idlePwmStarted) {
            tim2PwmStart();
            idlePwmStarted = true;
        }
        uint32_t phase = idleElapsed % 10000;
        uint32_t level = (phase < 5000) ? (phase * 255UL / 5000UL)
                                        : ((10000UL - phase) * 255UL / 5000UL);
        TIM2->CH3CVR = level;
        if (idleElapsed >= 10000) idleEnter(IDLE_LAMP_TEST);
        break;
    }
    }
}

static void idleTick(uint32_t elapsedMs) {
    idleElapsed += elapsedMs;
    idleAdvance();
}

static void idleStop(void) {
    tim1PwmStop();
    tim2PwmStop();
    ledsOff();
}

// ---- HID mode ------------------------------------------------------------

static uint8_t matrixState[ROW_COUNT][COL_COUNT];
static uint8_t wheelCnt[ROW_COUNT][COL_COUNT];
static uint8_t wheelCd[ROW_COUNT][COL_COUNT];

static void sendKey(uint8_t *previous, uint8_t current, uint8_t key) {
    if (current == *previous) return;
    *previous = current;
    if (current) Keyboard.press(key);
    else Keyboard.release(key);
}

static void processHid(const uint8_t raw[ROW_COUNT][COL_COUNT], int dx, int dy) {
    for (int r = 0; r < ROW_COUNT; r++) {
        for (int c = 0; c < COL_COUNT; c++) {
            uint16_t key = KEY_MAP[r][c];
            if (key == 0) continue;
            uint8_t pressed = raw[r][c];
            if (pressed && !matrixState[r][c] && ghostRisk(matrixState, r, c)) pressed = 0;

            switch (key) {
            case KEY_MOUSE_BTN_LEFT:
            case KEY_MOUSE_BTN_RIGHT:
            case KEY_MOUSE_BTN_MIDDLE: {
                uint8_t button = (key == KEY_MOUSE_BTN_LEFT) ? MOUSE_LEFT :
                                 (key == KEY_MOUSE_BTN_RIGHT) ? MOUSE_RIGHT : MOUSE_MIDDLE;
                if (pressed != matrixState[r][c]) {
                    matrixState[r][c] = pressed;
                    if (pressed) Mouse.press(button); else Mouse.release(button);
                }
                break;
            }
            case KEY_MOUSE_WHEEL_UP:
            case KEY_MOUSE_WHEEL_DN:
                if (wheelCd[r][c]) wheelCd[r][c]--;
                if (pressed) {
                    if (wheelCnt[r][c] < WHEEL_TRIGGER) wheelCnt[r][c]++;
                    if (wheelCnt[r][c] >= WHEEL_TRIGGER && wheelCd[r][c] == 0) {
                        Mouse.move(0, 0, key == KEY_MOUSE_WHEEL_UP ? 1 : -1);
                        wheelCd[r][c] = WHEEL_COOLDOWN;
                    }
                } else {
                    wheelCnt[r][c] = 0;
                }
                matrixState[r][c] = pressed;
                break;
            default:
                sendKey(&matrixState[r][c], pressed, (uint8_t)key);
                break;
            }
        }
    }

    uint8_t led1 = 0, led2 = 0, led3 = 0;
    for (int r = 0; r < ROW_COUNT; r++) {
        for (int c = 0; c < COL_COUNT; c++) {
            if (!matrixState[r][c]) continue;
            uint8_t ledClass = ledClassOf(KEY_MAP[r][c]);
            if (ledClass == 1) led1 = 1;
            else if (ledClass == 2) led2 = 1;
            else led3 = 1;
        }
    }
    writeLed(LED1, led1);
    writeLed(LED2, led2);
    writeLed(LED3, led3);

    if (joystickMoved(dx, dy)) {
        int8_t mx = clamp8(dx / JOY_SCALE);
        int8_t my = clamp8(dy / JOY_SCALE);
        if (mx || my) Mouse.move(mx, my);
    }
}

int main(void) {
    SystemInit();
    GPIO_port_enable(GPIO_port_A);
    GPIO_port_enable(GPIO_port_C);
    GPIO_port_enable(GPIO_port_D);

    GPIO_pinMode(LED1, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_pinMode(LED2, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    GPIO_pinMode(LED3, GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
    ledsOff();

    for (int c = 0; c < COL_COUNT; c++) {
        GPIO_pinMode(COL_PINS[c], GPIO_pinMode_O_pushPull, GPIO_Speed_50MHz);
        GPIO_digitalWrite_hi(COL_PINS[c]);
    }
    for (int r = 0; r < ROW_COUNT; r++) {
        GPIO_pinMode(ROW_PINS[r], GPIO_pinMode_I_pullUp, GPIO_Speed_In);
        for (int c = 0; c < COL_COUNT; c++) {
            matrixState[r][c] = 0;
            wheelCnt[r][c] = 0;
            wheelCd[r][c] = 0;
        }
    }

    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 2), GPIO_pinMode_I_analog, GPIO_Speed_In);
    GPIO_pinMode(GPIOv_from_PORT_PIN(GPIO_port_A, 1), GPIO_pinMode_I_analog, GPIO_Speed_In);
    GPIO_ADCinit();

    int centerX = 0, centerY = 0;
    for (int i = 0; i < 50; i++) {
        centerX += GPIO_analogRead(GPIO_Ain0_A2);
        centerY += GPIO_analogRead(GPIO_Ain1_A1);
        Delay_Ms(1);
    }
    centerX /= 50;
    centerY /= 50;

    // USB must start before the idle loop, so a PC recognizes HID immediately.
    Keyboard.begin();
    Mouse.begin();
    Delay_Ms(1);
    usb_setup();

    bool hidActive = false;
    idleEnter(IDLE_LAMP_TEST);

    while (1) {
        uint8_t raw[ROW_COUNT][COL_COUNT];
        scanMatrix(raw);

        if (!hidActive) {
            // Only a matrix-key press exits the idle animation.  Joystick ADC
            // pins may float when the joystick circuit is not populated, so
            // their values must never be used as a wake trigger.
            if (anyMatrixKey(raw)) {
                // The current raw state is deliberately processed below, so the
                // action that wakes the controller is also delivered to the PC.
                idleStop();
                hidActive = true;
            } else {
                idleTick(10);
                Delay_Ms(10);
                continue;
            }
        }

        // ADC is read only after a matrix key has activated HID mode.
        int dx = GPIO_analogRead(GPIO_Ain0_A2) - centerX;
        int dy = GPIO_analogRead(GPIO_Ain1_A1) - centerY;
        processHid(raw, dx, dy);

        Delay_Ms(10);
    }
}
