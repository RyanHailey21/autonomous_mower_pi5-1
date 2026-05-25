const byte LEFT_SPEED_PIN = 2;
const byte RIGHT_SPEED_PIN = 3;
const byte PWM_OUT_PIN = 9;

volatile unsigned long left_pulses = 0;
volatile unsigned long right_pulses = 0;
volatile unsigned long left_total_pulses = 0;
volatile unsigned long right_total_pulses = 0;

unsigned long last_report_ms = 0;
const unsigned long REPORT_INTERVAL_MS = 200;

int pwm_value = 20;

void leftPulseISR() {
  left_pulses++;
  left_total_pulses++;
}

void rightPulseISR() {
  right_pulses++;
  right_total_pulses++;
}

void setPwm(int value) {
  pwm_value = constrain(value, 0, 255);
  analogWrite(PWM_OUT_PIN, pwm_value);

  Serial.print("OK PWM ");
  Serial.println(pwm_value);
}

void resetCounts() {
  noInterrupts();
  left_pulses = 0;
  right_pulses = 0;
  left_total_pulses = 0;
  right_total_pulses = 0;
  interrupts();

  Serial.println("OK RESET_COUNTS");
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(LEFT_SPEED_PIN, INPUT_PULLUP);
  pinMode(RIGHT_SPEED_PIN, INPUT_PULLUP);

  pinMode(PWM_OUT_PIN, OUTPUT);
  analogWrite(PWM_OUT_PIN, pwm_value);

  attachInterrupt(digitalPinToInterrupt(LEFT_SPEED_PIN), leftPulseISR, RISING);
  attachInterrupt(digitalPinToInterrupt(RIGHT_SPEED_PIN), rightPulseISR, RISING);

  Serial.println("BOOT UNO_R4_SPEED_PWM_TOTAL_TEST");
  Serial.print("PWM_START ");
  Serial.println(pwm_value);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "PING") {
      Serial.println("PONG");
    } else if (cmd == "LED_ON") {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("OK LED_ON");
    } else if (cmd == "LED_OFF") {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("OK LED_OFF");
    } else if (cmd == "STATUS") {
      Serial.print("STATUS OK PWM ");
      Serial.println(pwm_value);
    } else if (cmd == "RESET_COUNTS") {
      resetCounts();
    } else if (cmd == "PWM_OFF") {
      setPwm(0);
    } else if (cmd == "PWM_SLOW") {
      setPwm(20);
    } else if (cmd == "PWM_UP") {
      setPwm(pwm_value + 5);
    } else if (cmd == "PWM_DOWN") {
      setPwm(pwm_value - 5);
    } else if (cmd.startsWith("PWM ")) {
      int value = cmd.substring(4).toInt();
      setPwm(value);
    } else {
      Serial.print("UNKNOWN ");
      Serial.println(cmd);
    }
  }

  unsigned long now = millis();

  if (now - last_report_ms >= REPORT_INTERVAL_MS) {
    last_report_ms = now;

    noInterrupts();
    unsigned long lp = left_pulses;
    unsigned long rp = right_pulses;
    unsigned long lt = left_total_pulses;
    unsigned long rt = right_total_pulses;
    left_pulses = 0;
    right_pulses = 0;
    interrupts();

    float left_hz = (lp * 1000.0) / REPORT_INTERVAL_MS;
    float right_hz = (rp * 1000.0) / REPORT_INTERVAL_MS;

    Serial.print("SPEED_COUNTS ");
    Serial.print(lp);
    Serial.print(" ");
    Serial.print(rp);
    Serial.print(" HZ ");
    Serial.print(left_hz, 1);
    Serial.print(" ");
    Serial.print(right_hz, 1);
    Serial.print(" TOTAL ");
    Serial.print(lt);
    Serial.print(" ");
    Serial.print(rt);
    Serial.print(" PWM ");
    Serial.println(pwm_value);
  }
}
