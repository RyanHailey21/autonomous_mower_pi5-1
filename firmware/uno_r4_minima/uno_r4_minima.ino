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
bool calibration_active = false;
unsigned long calibration_left_start = 0;
unsigned long calibration_right_start = 0;

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
  calibration_active = false;

  Serial.println("OK RESET_COUNTS");
}

void getTotalCounts(unsigned long &left_total, unsigned long &right_total) {
  noInterrupts();
  left_total = left_total_pulses;
  right_total = right_total_pulses;
  interrupts();
}

void startCalibration() {
  getTotalCounts(calibration_left_start, calibration_right_start);
  calibration_active = true;

  Serial.print("OK CAL_START LEFT_TOTAL ");
  Serial.print(calibration_left_start);
  Serial.print(" RIGHT_TOTAL ");
  Serial.println(calibration_right_start);
}

void printCalibrationCounts(const char *label) {
  unsigned long left_total;
  unsigned long right_total;
  getTotalCounts(left_total, right_total);

  Serial.print(label);
  Serial.print(" LEFT_COUNTS ");
  Serial.print(left_total - calibration_left_start);
  Serial.print(" RIGHT_COUNTS ");
  Serial.println(right_total - calibration_right_start);
}

void stopCalibration(float revolutions) {
  if (!calibration_active) {
    Serial.println("ERR CAL_NOT_ACTIVE");
    return;
  }

  if (revolutions <= 0.0) {
    Serial.println("ERR CAL_STOP_REVS_MUST_BE_POSITIVE");
    return;
  }

  unsigned long left_total;
  unsigned long right_total;
  getTotalCounts(left_total, right_total);

  unsigned long left_counts = left_total - calibration_left_start;
  unsigned long right_counts = right_total - calibration_right_start;
  float left_counts_per_rev = left_counts / revolutions;
  float right_counts_per_rev = right_counts / revolutions;

  Serial.print("CAL_RESULT REVS ");
  Serial.print(revolutions, 3);
  Serial.print(" LEFT_COUNTS ");
  Serial.print(left_counts);
  Serial.print(" RIGHT_COUNTS ");
  Serial.print(right_counts);
  Serial.print(" LEFT_COUNTS_PER_REV ");
  Serial.print(left_counts_per_rev, 3);
  Serial.print(" RIGHT_COUNTS_PER_REV ");
  Serial.println(right_counts_per_rev, 3);

  calibration_active = false;
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
      Serial.print(pwm_value);
      Serial.print(" CAL_ACTIVE ");
      Serial.println(calibration_active ? 1 : 0);
    } else if (cmd == "CAL_START") {
      startCalibration();
    } else if (cmd == "CAL_STATUS") {
      if (calibration_active) {
        printCalibrationCounts("CAL_STATUS");
      } else {
        Serial.println("ERR CAL_NOT_ACTIVE");
      }
    } else if (cmd == "CAL_CANCEL") {
      calibration_active = false;
      Serial.println("OK CAL_CANCEL");
    } else if (cmd.startsWith("CAL_STOP ")) {
      float revolutions = cmd.substring(9).toFloat();
      stopCalibration(revolutions);
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
