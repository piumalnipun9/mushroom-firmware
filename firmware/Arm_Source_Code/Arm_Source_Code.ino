  #include <WiFi.h>
  #include <ESP32Servo.h>
  #include <WebServer.h>

  const char* ssid = "Pixel_9483";
  const char* password = "12345678";

  WebServer server(80);

  Servo servo1, servo2, servo3, servo4;

  double now_time = 0;

  const int servoPin1 = 18;
  const int servoPin2 = 19;
  const int servoPin3 = 21;
  const int servoPin4 = 22;

  int currentPositions[4] = {500, 1400, 600, 2000};

  const int duration = 200;
  const int updateInterval = 1;

  void setup() {
    Serial.begin(115200);
    connectToWiFi();
    servo1.attach(servoPin1);
    servo2.attach(servoPin2);
    servo3.attach(servoPin3);
    servo4.attach(servoPin4);
    server.on("/set_positions", handleSetPositions);
    server.begin();
    moveToPositions(currentPositions);
  }

  void loop() {
    server.handleClient();
  }

  void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }

  void handleSetPositions() {
    Serial.println("--- Incoming Request ---");
    if (server.hasArg("pos")) {
      String positionsArg = server.arg("pos");
      Serial.print("Received positions: ");
      Serial.println(positionsArg);
      int newPositions[4];
      if (parsePositions(positionsArg, newPositions)) {
        Serial.print("Parsed: [");
        for (int i = 0; i < 4; i++) {
          Serial.print(newPositions[i]);
          if (i < 3) Serial.print(", ");
        }
        Serial.println("]");
        smoothMove(servo1, servo2, servo3, servo4, currentPositions, newPositions, duration);
        for (int i = 0; i < 4; i++) {
          currentPositions[i] = newPositions[i];
        }
        Serial.println("Positions updated successfully");
        server.send(200, "text/plain", "Positions updated");
      } else {
        Serial.println("ERROR: Invalid position data");
        server.send(400, "text/plain", "Invalid position data");
      }
    } else {
      Serial.println("ERROR: Missing 'pos' parameter");
      server.send(400, "text/plain", "Missing 'pos' parameter");
    }
  }

  bool parsePositions(String input, int positions[]) {
    int index = 0;
    int startIndex = 0;
    int commaIndex;
    while ((commaIndex = input.indexOf(',', startIndex)) != -1 && index < 4) {
      positions[index++] = input.substring(startIndex, commaIndex).toInt();
      startIndex = commaIndex + 1;
    }
    if (index < 3 || startIndex >= input.length()) {
      return false;
    }
    positions[index++] = input.substring(startIndex).toInt();
    return true;
  }

  void moveToPositions(int positions[]) {
    servo1.writeMicroseconds(positions[0]);
    servo2.writeMicroseconds(positions[1]);
    servo3.writeMicroseconds(positions[2]);
    servo4.writeMicroseconds(positions[3]);
  }

  void smoothMove(Servo &s1, Servo &s2, Servo &s3, Servo &s4, int start[], int end[], int duration) {
    int steps = duration / updateInterval;
    float increments[4];
    for (int i = 0; i < 4; i++) {
      increments[i] = (float)(end[i] - start[i]) / steps;
    }
    for (int i = 0; i <= steps; i++) {
      int currentPositions[4] = {
        start[0] + increments[0] * i,
        start[1] + increments[1] * i,
        start[2] + increments[2] * i,
        start[3] + increments[3] * i
      };
      now_time = millis();
      while (millis() - now_time < updateInterval) {
        s1.writeMicroseconds(currentPositions[0]);
        s2.writeMicroseconds(currentPositions[1]);
        s3.writeMicroseconds(currentPositions[2]);
        s4.writeMicroseconds(currentPositions[3]);
      }
    }
  }