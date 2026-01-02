#include <Arduino.h>
#include <ESP32Servo.h>  // Use ESP32Servo library (install via Library Manager)
#include <math.h>

Servo Base, Shoulder, Arm, Wrist, Gripper;

// Current joint states
float curBase = 90;
float curShoulder = 90;
float curArm = 90;
float curWrist = 90;
int curGripper = 1;

const float K_CAL = 314.5f;
const int FLEX = 90;
const int GRIPPER_OPEN_ANGLE = 130;
const int GRIPPER_CLOSED_ANGLE = 180;

// ESP32 PWM Pins for Servos
// (Avoiding pins used by sensors: 4,15,16,17,21,22,23,25,33,34)
const int PIN_BASE = 13;      // GPIO13 - Base servo
const int PIN_SHOULDER = 12;  // GPIO12 - Shoulder servo
const int PIN_ELBOW = 14;     // GPIO14 - Elbow/Arm servo
const int PIN_WRIST = 27;     // GPIO27 - Wrist servo  
const int PIN_GRIPPER = 26;   // GPIO26 - Gripper servo

// ------------------------------
// S-curve easing for velocity shaping
// ------------------------------
float getSCurveProgress(float t) {
  t = constrain(t, 0.0f, 1.0f);
  return t * t * t * (10 - 15 * t + 6 * t * t);
}

// ------------------------------
// Move 4 joints + gripper SIMULTANEOUSLY
// ------------------------------
void move4Joints(float targetBase, float targetShoulder, float targetArm, int targetGripper, int duration_ms = 1200) {
  int totalSteps = 90;
  int stepDelay = max(5, duration_ms / totalSteps);

  float diffBase = targetBase - curBase;
  float diffShoulder = targetShoulder - curShoulder;
  float diffArm = targetArm - curArm;
  int diffGripper = targetGripper - curGripper;

  for (int i = 0; i <= totalSteps; i++) {
    float p = getSCurveProgress((float)i / totalSteps);

    float newBase = curBase + diffBase * p;
    float newShoulder = curShoulder + diffShoulder * p;
    float newArm = curArm + diffArm * p;

    float newWrist = K_CAL - (newShoulder + newArm);

    Base.write(constrain((int)newBase, 0, 180));
    Shoulder.write(constrain((int)newShoulder, 0, 180));
    Arm.write(constrain((int)newArm, 0, 180));
    Wrist.write(constrain((int)newWrist, 0, 180));

    if (i == totalSteps) {
      if (targetGripper == 0) Gripper.write(GRIPPER_CLOSED_ANGLE);
      else if (targetGripper == 1) Gripper.write(GRIPPER_OPEN_ANGLE);
    }

    delay(stepDelay);
  }

  curBase = targetBase;
  curShoulder = targetShoulder;
  curArm = targetArm;
  curWrist = K_CAL - (targetShoulder + targetArm);
  curGripper = targetGripper;
}

// ------------------------------
// Inverse Kinematics
// ------------------------------
bool inverseKinematics(float x, float y, float z, float angles[6]) {

  // ------------------------------
  // Link lengths (mm)
  // ------------------------------
  const float d1 = 2.5;
  const float a2 = 10.5;
  const float a3 = 14.5;
  const float d5 = 7.0;

  // ------------------------------
  // θ1: Base rotation
  // ------------------------------
  angles[0] = atan2(y, x) * 180.0 / M_PI;

  // ------------------------------
  // Find wrist centre
  // ------------------------------
  float r = sqrt(x * x + y * y);
  float s = z - d1;

  // ------------------------------
  // Planar IK for shoulder & elbow
  // ------------------------------
  float D = (r * r + s * s - a2 * a2 - a3 * a3) / (2 * a2 * a3);
  if (D < -1.0 || D > 1.0) return false;

  float elbowRad = -acos(D);
  angles[2] = elbowRad * 180.0 / M_PI;

  float shoulderRad = atan2(s, r) - atan2(a3 * sin(elbowRad), a2 + a3 * cos(elbowRad));
  angles[1] = shoulderRad * 180.0 / M_PI;

  // ------------------------------
  // Apply servo calibration
  // ------------------------------
  angles[1] = angles[1] + 90.0;
  angles[2] = angles[2] + 90.0;

  // ------------------------------
  // θ4: Wrist
  // ------------------------------
  angles[3] = K_CAL - (angles[1] + angles[2]);

  angles[4] = 0;
  angles[5] = 0;

  return true;
}

// ------------------------------
// Setup
// ------------------------------
void setup() {
  Serial.begin(9600);

  Base.attach(PIN_BASE);
  Shoulder.attach(PIN_SHOULDER);
  Arm.attach(PIN_ELBOW);
  Wrist.attach(PIN_WRIST);
  Gripper.attach(PIN_GRIPPER);

  Base.write((int)curBase);
  Shoulder.write((int)curShoulder);
  Arm.write((int)curArm);
  Wrist.write((int)curWrist);
  Gripper.write(GRIPPER_OPEN_ANGLE);

  Serial.println("Ready. Send {base,shoulder,arm,gripCmd} or type SEQ");
}

// ------------------------------
// Main loop
// ------------------------------
void loop() {
  if (Serial.available() == 0) return;

  String input = Serial.readStringUntil('\n');
  input.trim();

  // ------------------------------
  // Predefined sequence
  // ------------------------------
  if (input.equalsIgnoreCase("SEQ")) {
    const int duration = 1500;

    int sequence[][4] = {
      { 0, 60, 90, 1 },
      { 0, 130, 100, 1 },
      { 0, 130, 100, 0 },
      { 0, 60, 90, 0 },
      { 90, 60, 90, 0 },
      { 90, 130, 135, 0 },
      { 90, 130, 135, 1 },
      { 90, 60, 90, 1 },
      { 0, 60, 90, 0 },
      { 0, 130, 100, 0 },
    };

    int steps = sizeof(sequence) / sizeof(sequence[0]);
    Serial.println("Running programmed sequence...");

    for (int i = 0; i < steps; i++) {
      float base = sequence[i][0];
      float shoulder = sequence[i][1];
      float arm = sequence[i][2];
      int gripCmd = sequence[i][3];

      int targetGripper = gripCmd;
      float computedWrist = constrain(round(K_CAL - (shoulder + arm)), 0, 180);

      Serial.print("Step ");
      Serial.print(i);
      Serial.print(" → B:");
      Serial.print(base);
      Serial.print(" S:");
      Serial.print(shoulder);
      Serial.print(" A:");
      Serial.print(arm);
      Serial.print(" W:");
      Serial.print(computedWrist);
      Serial.print(" G:");
      Serial.println(targetGripper);

      move4Joints(base, shoulder, arm, targetGripper, duration);
      delay(duration);
    }

    Serial.println("Sequence done.");
    return;
  }

  // ------------------------------
  // Normal {base,shoulder,arm,grip} input
  // ------------------------------
  if (input.length() >= 5 && input.charAt(0) == '{' && input.charAt(input.length() - 1) == '}') {
    input.remove(0, 1);
    input.remove(input.length() - 1, 1);

    int c1 = input.indexOf(',');
    int c2 = input.indexOf(',', c1 + 1);
    int c3 = input.indexOf(',', c2 + 1);

    if (c1 == -1 || c2 == -1 || c3 == -1 || input.indexOf(',', c3 + 1) != -1) {
      Serial.println("Comma error");
      return;
    }

    float targetBase = constrain(input.substring(0, c1).toFloat(), 0, 180);
    float targetShoulder = constrain(input.substring(c1 + 1, c2).toFloat(), 0, 180);
    float targetArm = constrain(input.substring(c2 + 1, c3).toFloat(), 0, 180);
    int gripCmd = (int)round(input.substring(c3 + 1).toFloat());

    int targetGripper;
    if (gripCmd == 0) targetGripper = GRIPPER_CLOSED_ANGLE;
    else if (gripCmd == 1) targetGripper = GRIPPER_OPEN_ANGLE;
    else targetGripper = curGripper;

    Serial.print("Moving -> Base:");
    Serial.print(targetBase);
    Serial.print(" Shoulder:");
    Serial.print(targetShoulder);
    Serial.print(" Arm:");
    Serial.print(targetArm);

    float computedWrist = K_CAL - (targetShoulder + targetArm);
    Serial.print(" Wrist:");
    Serial.print(computedWrist);
    Serial.print(" Gripper:");
    Serial.println(targetGripper);

    move4Joints(targetBase, targetShoulder, targetArm, targetGripper, 1500);
    return;
  }

  // ------------------------------
  // Handle [x,y,z] Cartesian input (Inverse Kinematics)
  // ------------------------------
  if (input.length() >= 5 && input.charAt(0) == '[' && input.charAt(input.length() - 1) == ']') {
    input.remove(0, 1);
    input.remove(input.length() - 1, 1);

    int c1 = input.indexOf(',');
    int c2 = input.indexOf(',', c1 + 1);

    if (c1 == -1 || c2 == -1 || input.indexOf(',', c2 + 1) != -1) {
      Serial.println("Comma error in coordinates");
      return;
    }

    float x = input.substring(0, c1).toFloat();
    float y = input.substring(c1 + 1, c2).toFloat();
    float z = input.substring(c2 + 1).toFloat();

    // Offset by wrist-to-gripper distance
    const float d5 = 7.0;

    float dx = x;
    float dy = y;
    float mag = sqrt(dx*dx + dy*dy);

    float wrist_x, wrist_y;
    if (mag > 0.001f) {
        wrist_x = x - dx/mag * d5;
        wrist_y = y - dy/mag * d5;
    } else {
        wrist_x = 0;
        wrist_y = 0;
    }

    float wrist_z = z;

    float wristAngles[6];
    if (!inverseKinematics(wrist_x, wrist_y, wrist_z, wristAngles)) {
      Serial.println("Target unreachable");
      return;
    }

    float targetBase = wristAngles[0];
    float targetShoulder = wristAngles[1];
    float targetArm = wristAngles[2];
    int targetGripper = curGripper;

    Serial.print("Moving gripper to X:");
    Serial.print(x);
    Serial.print(" Y:");
    Serial.print(y);
    Serial.print(" Z:");
    Serial.print(z);
    Serial.print(" -> Base:");
    Serial.print(targetBase);
    Serial.print(" Shoulder:");
    Serial.print(targetShoulder);
    Serial.print(" Arm:");
    Serial.println(targetArm);

    move4Joints(targetBase, targetShoulder, targetArm, targetGripper, 1500);
    return;
  }

  // If nothing matched
  Serial.println("Unrecognized input. Use: {base,shoulder,arm,grip} or [x,y,z] or SEQ");
}


