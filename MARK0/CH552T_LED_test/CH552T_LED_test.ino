const int LED1_PIN = 32; // LED_1 P3.2 IO
const int LED2_PIN = 31; // LED_2 P3.1 ソフトPWM(ハードPWM非対応ピン)
const int LED3_PIN = 30; // LED_3 P3.0 ソフトPWM(ハードPWM非対応ピン)

// CH552のハードPWMはP1.4/P1.5/P3.3/P3.4のみ。上記3ピンは非対応のため、
// GPIOのビットバンギング(ソフトPWM)で点滅・PWM・呼吸を生成する。

void allOff() {
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
}

// delayMicrosecondsの安全域を超える長い待ち(低周波用)を、delay(1)刻みで代替する
void delayUsLong(unsigned long us) {
  while (us >= 1000) {
    delay(1);
    us -= 1000;
  }
  if (us) delayMicroseconds((unsigned int)us);
}

// ソフトPWM: 指定周波数・duty50%の矩形波をdurationMsだけ出力する
void softPwmFreq50(int pin, uint32_t freqHz, unsigned long durationMs) {
  unsigned long halfPeriodUs = 500000UL / freqHz; // 50%duty時の半周期(us)
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    digitalWrite(pin, HIGH);
    delayUsLong(halfPeriodUs);
    digitalWrite(pin, LOW);
    delayUsLong(halfPeriodUs);
  }
  digitalWrite(pin, LOW);
}

// ソフトPWM: 約500Hzのキャリアで、指定duty(0-255)をdurationMsだけ出力する
void softPwmDuty(int pin, uint8_t duty, unsigned long durationMs) {
  const unsigned int PERIOD_US = 2000; // 約500Hz(ちらつき知覚しにくい)
  unsigned int onUs  = (unsigned int)(((unsigned long)PERIOD_US * duty) / 255);
  unsigned int offUs = PERIOD_US - onUs;
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    if (onUs)  { digitalWrite(pin, HIGH); delayMicroseconds(onUs); }
    if (offUs) { digitalWrite(pin, LOW);  delayMicroseconds(offUs); }
  }
  digitalWrite(pin, LOW);
}

// パターン0: 3つ同時点滅で点灯確認（他LEDへの影響が出る唯一の区間）
void pattern0_lampTest() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, HIGH);
    digitalWrite(LED3_PIN, HIGH);
    delay(250);
    allOff();
    delay(250);
  }
}

// 指定ピンを、指定したON時間/OFF時間(ms)のリストで順に点滅させる
void blinkStepsAsym(int pin, const unsigned long onTimes[], const unsigned long offTimes[], int count, unsigned long stepDurationMs) {
  for (int s = 0; s < count; s++) {
    unsigned long start = millis();
    while (millis() - start < stepDurationMs) {
      digitalWrite(pin, HIGH);
      delay(onTimes[s]);
      digitalWrite(pin, LOW);
      delay(offTimes[s]);
    }
  }
  digitalWrite(pin, LOW);
}

// パターン1: LED1のみ、2Hz/5Hz/10Hzで各2秒間点滅（点灯時間と消灯時間が等しい＝duty50%）
void pattern1_led1Blink() {
  const unsigned long onTimes[]  = {250, 100, 50}; // 2Hz, 5Hz, 10Hz の点灯時間(ms)
  const unsigned long offTimes[] = {250, 100, 50}; // 同、消灯時間(ms)
  blinkStepsAsym(LED1_PIN, onTimes, offTimes, 3, 2000);
}

// パターン2: LED2のみ、10〜1000Hzで各2秒間ソフトPWM出力(duty50%)
void pattern2_led2PwmSweep() {
  const uint32_t freqs[] = {10, 20, 40, 80, 1000};
  const int numSteps = sizeof(freqs) / sizeof(freqs[0]);
  const unsigned long STEP_DURATION_MS = 2000;

  for (int s = 0; s < numSteps; s++) {
    softPwmFreq50(LED2_PIN, freqs[s], STEP_DURATION_MS);
  }
  digitalWrite(LED2_PIN, LOW);
}

// 0〜πを33点サンプリングした正弦(呼吸)テーブル。
// float/cos()を使うとFPU非搭載の8051コアCH552TではFLASHに収まらないため整数テーブル化している
const uint8_t SINE_TABLE[33] = {
    0,   1,   2,   5,  10,  15,  21,  29,  37,  47,  57,  67,  79,  90, 103, 115, 128,
  140, 152, 165, 176, 188, 198, 208, 218, 226, 234, 240, 245, 249, 253, 254, 255
};
const int SINE_TABLE_SIZE = 33;

// パターン3: LED3のみ、台形波（duty0〜100%まで立ち上げ→頂点ホールド→立ち下げ→底ホールド）を10秒間
void pattern3_led3Trapezoid() {
  const unsigned long STEP_MS = 31; // ランプ中の1ステップの表示時間
  const unsigned long HOLD_TOP_MS = 500;    // 頂点でのホールド時間
  const unsigned long HOLD_BOTTOM_MS = 500; // 底でのホールド時間
  const unsigned long TOTAL_MS = 10000;

  unsigned long start = millis();
  while (millis() - start < TOTAL_MS) {
    for (int idx = 0; idx < SINE_TABLE_SIZE; idx++) { // 立ち上げ(duty0%→100%)
      softPwmDuty(LED3_PIN, SINE_TABLE[idx], STEP_MS);
    }
    softPwmDuty(LED3_PIN, 255, HOLD_TOP_MS); // 頂点ホールド
    for (int idx = SINE_TABLE_SIZE - 1; idx >= 0; idx--) { // 立ち下げ(duty100%→0%)
      softPwmDuty(LED3_PIN, SINE_TABLE[idx], STEP_MS);
    }
    delay(HOLD_BOTTOM_MS); // 底ホールド(消灯)
  }
  digitalWrite(LED3_PIN, LOW);
}

void setup() {
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  allOff();
  delay(50);

  pattern0_lampTest();
  delay(500);
}

void loop() {
  pattern1_led1Blink();
  delay(50);
  pattern2_led2PwmSweep();
  delay(50);
  pattern3_led3Trapezoid();
  delay(50);
}
