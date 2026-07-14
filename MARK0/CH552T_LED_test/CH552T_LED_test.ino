const int LED1_PIN = 32; // LED_1 P3.2 IO
const int LED2_PIN = 31; // LED_2 P3.1 PWM
const int LED3_PIN = 30; // LED_3 P3.0 PWM

void allOff() {
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
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

// パターン2: LED2のみ、20〜5120Hzで各2秒間PWM出力(duty50%)
void pattern2_led2PwmSweep() {
  const uint32_t freqs[] = {10, 20, 40, 80, 1000};
  const int numSteps = sizeof(freqs) / sizeof(freqs[0]);
  const unsigned long STEP_DURATION_MS = 2000;

  for (int s = 0; s < numSteps; s++) {
    analogWriteFrequency(freqs[s]);
    analogWrite(LED2_PIN, 128); // duty 50%
    delay(STEP_DURATION_MS);
  }
  analogWrite(LED2_PIN, 0);
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

  analogWriteFrequency(200); // PWM周期を約5ms(200Hz)に変更

  unsigned long start = millis();
  while (millis() - start < TOTAL_MS) {
    for (int idx = 0; idx < SINE_TABLE_SIZE; idx++) { // 立ち上げ(duty0%→100%)
      analogWrite(LED3_PIN, SINE_TABLE[idx]);
      delay(STEP_MS);
    }
    delay(HOLD_TOP_MS); // 頂点ホールド
    for (int idx = SINE_TABLE_SIZE - 1; idx >= 0; idx--) { // 立ち下げ(duty100%→0%)
      analogWrite(LED3_PIN, SINE_TABLE[idx]);
      delay(STEP_MS);
    }
    delay(HOLD_BOTTOM_MS); // 底ホールド
  }
  analogWrite(LED3_PIN, 0);
}

void setup() {
  analogWriteResolution(8); // このコアのanalogWriteは既定12bit(0-4095)のため、8bit(0-255)に設定
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
