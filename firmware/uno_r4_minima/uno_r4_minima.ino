void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "LED_ON") {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("OK LED_ON");
    } else if (cmd == "LED_OFF") {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("OK LED_OFF");
    } else {
      Serial.print("UNKNOWN ");
      Serial.println(cmd);
    }
  }
}