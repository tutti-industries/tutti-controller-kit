#include "ch32fun.h"

#define LED1_PIN PD2 // IO
#define LED2_PIN PC3 // PWM (TIM1_CH3)
#define LED3_PIN PC0 // PWM (TIM2_CH3)

static void allOff(void) {
	funDigitalWrite(LED1_PIN, FUN_LOW);
	funDigitalWrite(LED2_PIN, FUN_LOW);
	funDigitalWrite(LED3_PIN, FUN_LOW);
}

// パターン0: 3つ同時点滅で点灯確認（他LEDへの影響が出る唯一の区間）
static void pattern0_lampTest(void) {
	for (int i = 0; i < 4; i++) {
		funDigitalWrite(LED1_PIN, FUN_HIGH);
		funDigitalWrite(LED2_PIN, FUN_HIGH);
		funDigitalWrite(LED3_PIN, FUN_HIGH);
		Delay_Ms(250);
		allOff();
		Delay_Ms(250);
	}
}

// 指定ピンを、指定したON時間/OFF時間(ms)のリストで順に点滅させる
static void blinkStepsAsym(uint8_t pin, const uint32_t onTimes[], const uint32_t offTimes[], int count, uint32_t stepDurationMs) {
	for (int s = 0; s < count; s++) {
		uint32_t repeats = stepDurationMs / (onTimes[s] + offTimes[s]);
		for (uint32_t r = 0; r < repeats; r++) {
			funDigitalWrite(pin, FUN_HIGH);
			Delay_Ms(onTimes[s]);
			funDigitalWrite(pin, FUN_LOW);
			Delay_Ms(offTimes[s]);
		}
	}
	funDigitalWrite(pin, FUN_LOW);
}

// パターン1: LED1のみ、2Hz/5Hz/10Hzで各2秒間点滅（点灯時間は消灯時間の半分＝duty約33%）
static void pattern1_led1Blink(void) {
	const uint32_t onTimes[]  = {10,  10, 10}; // 2Hz, 5Hz, 10Hz の点灯時間(ms)
	const uint32_t offTimes[] = {490, 190, 90}; // 同、消灯時間(ms)
	blinkStepsAsym(LED1_PIN, onTimes, offTimes, 3, 2000);
}

// ---- TIM1 (LED2 / PC3, T1CH3) PWM ----

static uint32_t pwmPscForFreq(uint32_t freqHz) {
	uint32_t psc = FUNCONF_SYSTEM_CORE_CLOCK / (freqHz * 256UL);
	return (psc > 0) ? (psc - 1) : 0;
}

static void tim1pwm_init(void) {
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1;

	// PC3 is T1CH3, 10MHz Output alt func, push-pull
	GPIOC->CFGLR &= ~(0xf << (4 * 3));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 3);

	RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

	TIM1->PSC = pwmPscForFreq(1000); // 初期値。実際の周波数はパターン内で都度設定
	TIM1->ATRLR = 255;
	TIM1->SWEVGR |= TIM_UG;

	TIM1->CCER |= TIM_CC3E | TIM_CC3P;
	TIM1->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1;
	TIM1->CH3CVR = 0;

	TIM1->BDTR |= TIM_MOE;
	TIM1->CTLR1 |= TIM_CEN;
}

static void tim1pwm_setFreq(uint32_t freqHz) {
	TIM1->PSC = pwmPscForFreq(freqHz);
	TIM1->SWEVGR |= TIM_UG;
}

// パターン2: LED2のみ、20〜5120Hzで各2秒間PWM出力(duty50%)
static void pattern2_led2PwmSweep(void) {
	const uint32_t freqs[] = {10, 20, 40, 80, 160, 320, 640, 5000};
	const int numSteps = sizeof(freqs) / sizeof(freqs[0]);

	tim1pwm_init();
	for (int s = 0; s < numSteps; s++) {
		tim1pwm_setFreq(freqs[s]);
		TIM1->CH3CVR = 128; // duty 50%
		Delay_Ms(2000);
	}
	TIM1->CH3CVR = 0;
}

// ---- TIM2 (LED3 / PC0, T2CH3) PWM ----

static void tim2pwm_init(void) {
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	// PC0 is T2CH3, 10MHz Output alt func, push-pull
	GPIOC->CFGLR &= ~(0xf << (4 * 0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 0);

	RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

	TIM2->PSC = pwmPscForFreq(200); // PWM周期を約5ms(200Hz)に設定
	TIM2->ATRLR = 255;

	TIM2->CHCTLR2 |= TIM_OC3M_2 | TIM_OC3M_1 | TIM_OC3PE;
	TIM2->CTLR1 |= TIM_ARPE;
	TIM2->CCER |= TIM_CC3E | TIM_CC3P;
	TIM2->SWEVGR |= TIM_UG;

	TIM2->CTLR1 |= TIM_CEN;
}

// 0〜πを33点サンプリングした正弦(呼吸)テーブル
static const uint8_t SINE_TABLE[33] = {
	    0,   1,   2,   5,  10,  15,  21,  29,  37,  47,  57,  67,  79,  90, 103, 115, 128,
	  140, 152, 165, 176, 188, 198, 208, 218, 226, 234, 240, 245, 249, 253, 254, 255
};
#define SINE_TABLE_SIZE (sizeof(SINE_TABLE) / sizeof(SINE_TABLE[0]))

// パターン3: LED3のみ、台形波（duty0〜100%まで立ち上げ→頂点ホールド→立ち下げ→底ホールド）を10秒間
static void pattern3_led3Trapezoid(void) {
	const uint32_t STEP_MS = 31; // ランプ中の1ステップの表示時間
	const uint32_t HOLD_TOP_MS = 500;    // 頂点でのホールド時間
	const uint32_t HOLD_BOTTOM_MS = 500; // 底でのホールド時間
	const uint32_t TOTAL_MS = 10000;

	tim2pwm_init();

	uint32_t elapsed = 0;
	while (elapsed < TOTAL_MS) {
		for (unsigned idx = 0; idx < SINE_TABLE_SIZE; idx++) { // 立ち上げ(duty0%→100%)
			TIM2->CH3CVR = SINE_TABLE[idx];
			Delay_Ms(STEP_MS);
		}
		elapsed += SINE_TABLE_SIZE * STEP_MS;
		Delay_Ms(HOLD_TOP_MS); // 頂点ホールド
		elapsed += HOLD_TOP_MS;
		for (int idx = SINE_TABLE_SIZE - 1; idx >= 0; idx--) { // 立ち下げ(duty100%→0%)
			TIM2->CH3CVR = SINE_TABLE[idx];
			Delay_Ms(STEP_MS);
		}
		elapsed += SINE_TABLE_SIZE * STEP_MS;
		Delay_Ms(HOLD_BOTTOM_MS); // 底ホールド
		elapsed += HOLD_BOTTOM_MS;
	}
	TIM2->CH3CVR = 0;
}

int main(void) {
	SystemInit();
	funGpioInitAll();

	funPinMode(LED1_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funPinMode(LED2_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funPinMode(LED3_PIN, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	allOff();
	Delay_Ms(50);

	pattern0_lampTest();
	Delay_Ms(500);

	while (1) {
		pattern1_led1Blink();
		Delay_Ms(500);
		pattern2_led2PwmSweep();
		Delay_Ms(500);
		pattern3_led3Trapezoid();
		Delay_Ms(500);
	}
}
