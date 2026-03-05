#include "RobotArm.h"
#include <ESP32Servo.h>
#include <Arduino.h>

// ─── Hardware pins ────────────────────────────────────────────────────────
#define STEP_PIN 5
#define DIR_PIN 23 // PIN_SOIL_DIGITAL — only analog output used on that sensor
#define LIMIT_SWITCH 32
#define SERVO1_PIN 18
#define SERVO2_PIN 19

namespace RobotArm
{

    // ─── Location lookup tables ───────────────────────────────────────────────
    static long stepperPosFor(int loc)
    {
        return (loc == 1 || loc == 2) ? STEPPER_POS_NEAR : STEPPER_POS_FAR;
    }
    // Servo 1: +100 µs from centre for A-side (loc 1 & 3), -100 µs for B-side
    static int servo1UsFor(int loc)
    {
        return (loc == 1 || loc == 3) ? SERVO1_US_A : SERVO1_US_B;
    }
    // Servo 2: +500 µs from centre for A-side (loc 1 & 3), -500 µs for B-side
    static int servo2UsFor(int loc)
    {
        return (loc == 1 || loc == 3) ? SERVO2_US_A : SERVO2_US_B;
    }

    // ─── Module state ─────────────────────────────────────────────────────────
    static State state = STATE_IDLE;
    static int currentLocation = 0; // 0 = homed, location not yet known
    static int targetLocation = 0;
    static bool justArrived = false;

    static long currentSteps = 0;
    static long targetSteps = 0;

    static Servo servo1, servo2;
    static int svCurrent1, svCurrent2; // current microseconds
    static int svStart1, svStart2;
    static int svEnd1, svEnd2;
    static unsigned long svMoveStart = 0;
    static int svPhase = 0; // 0 = servo2 moving first, 1 = servo1 moving

    // ─── Servo smooth-move helpers ────────────────────────────────────────────
    // Always moves servo 2 fully first, then servo 1 (sequential, not simultaneous)
    static void startServoMove(int end1, int end2)
    {
        svStart1 = svCurrent1;
        svStart2 = svCurrent2;
        svEnd1 = end1;
        svEnd2 = end2;
        svPhase = 0;            // servo 2 goes first
        svMoveStart = millis();
    }

    // Returns true when both servo phases are complete
    static bool tickServoMove()
    {
        unsigned long elapsed = millis() - svMoveStart;

        if (svPhase == 0)
        {
            // ── Phase 0: move servo 2 to target ──────────────────────────────
            if (elapsed >= (unsigned long)SERVO_MOVE_MS)
            {
                svCurrent2 = svEnd2;
                servo2.writeMicroseconds(svCurrent2);
                servo1.writeMicroseconds(svCurrent1); // servo 1 holds position
                // Transition to phase 1
                svPhase = 1;
                svMoveStart = millis();
                return false;
            }
            float t = (float)elapsed / SERVO_MOVE_MS;
            svCurrent2 = svStart2 + (int)((svEnd2 - svStart2) * t);
            servo2.writeMicroseconds(svCurrent2);
            servo1.writeMicroseconds(svCurrent1); // servo 1 holds
        }
        else
        {
            // ── Phase 1: move servo 1 to target ──────────────────────────────
            if (elapsed >= (unsigned long)SERVO_MOVE_MS)
            {
                svCurrent1 = svEnd1;
                servo1.writeMicroseconds(svCurrent1);
                servo2.writeMicroseconds(svCurrent2); // servo 2 stays at end
                return true; // Both phases done
            }
            float t = (float)elapsed / SERVO_MOVE_MS;
            svCurrent1 = svStart1 + (int)((svEnd1 - svStart1) * t);
            servo1.writeMicroseconds(svCurrent1);
            servo2.writeMicroseconds(svCurrent2); // servo 2 stays at end
        }
        return false;
    }

    // ─── Stepper homing (blocking — called once in begin()) ──────────────────
    static void homeStepper()
    {
        Serial.println("[RobotArm] Homing...");
        digitalWrite(DIR_PIN, HIGH); // backward
        delay(100);
        while (digitalRead(LIMIT_SWITCH) == HIGH)
        {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(STEP_INTERVAL_US);
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(STEP_INTERVAL_US);
        }
        delay(200);
        // Back off slightly to release the switch
        digitalWrite(DIR_PIN, LOW);
        for (int i = 0; i < 600; i++)
        {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(STEP_INTERVAL_US);
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(STEP_INTERVAL_US);
        }
        currentSteps = 0;
        Serial.println("[RobotArm] Homing complete. Position = 0");
    }

    // ─── Public API ───────────────────────────────────────────────────────────

    void begin()
    {
        pinMode(STEP_PIN, OUTPUT);
        pinMode(DIR_PIN, OUTPUT);
        pinMode(LIMIT_SWITCH, INPUT_PULLUP);

        servo1.attach(SERVO1_PIN);
        servo2.attach(SERVO2_PIN);

        // Park servos at centre position (1500 µs)
        svCurrent1 = svCurrent2 = SERVO_CENTER_US;
        servo1.writeMicroseconds(SERVO_CENTER_US);
        servo2.writeMicroseconds(SERVO_CENTER_US);

        homeStepper();

        state = STATE_IDLE;
        currentLocation = 0;
        Serial.println("[RobotArm] Ready.");
    }

    bool setTarget(int location)
    {
        if (state != STATE_IDLE)
            return false; // busy
        if (location < 1 || location > 4)
            return false;
        if (location == currentLocation)
            return false; // already there

        targetLocation = location;
        targetSteps = stepperPosFor(location);

        // Step 1: retract arms to neutral (centre = 1500 µs)
        startServoMove(SERVO_CENTER_US, SERVO_CENTER_US);
        state = STATE_MOVING_TO_IDLE_ARM;
        Serial.printf("[RobotArm] Target: location %d  stepper target: %ld steps\n",
                      location, targetSteps);
        return true;
    }

    void update()
    {
        switch (state)
        {

        case STATE_IDLE:
            break;

        // ── 1. Retract arms to neutral ─────────────────────────────────────────
        case STATE_MOVING_TO_IDLE_ARM:
        {
            if (tickServoMove())
            {
                if (currentSteps == targetSteps)
                {
                    // Stepper already at target — go straight to final arm position
                    startServoMove(servo1UsFor(targetLocation), servo2UsFor(targetLocation));
                    state = STATE_MOVING_TO_TARGET_ARM;
                }
                else
                {
                    // Move stepper (blocking, yields to WiFi/WDT every 200 steps)
                    state = STATE_MOVING_STEPPER;
                    Serial.printf("[RobotArm] Moving stepper: %ld -> %ld\n",
                                  currentSteps, targetSteps);

                    digitalWrite(DIR_PIN, targetSteps > currentSteps ? LOW : HIGH);
                    delayMicroseconds(10); // direction-signal settle

                    long stepsDone = 0;
                    while (currentSteps != targetSteps)
                    {
                        digitalWrite(STEP_PIN, HIGH);
                        delayMicroseconds(STEP_INTERVAL_US);
                        digitalWrite(STEP_PIN, LOW);
                        delayMicroseconds(STEP_INTERVAL_US);

                        if (targetSteps > currentSteps)
                            currentSteps++;
                        else
                            currentSteps--;

                        // Yield every 200 steps to keep WiFi stack and WDT alive
                        if (++stepsDone % 200 == 0)
                            yield();
                    }

                    Serial.printf("[RobotArm] Stepper done at %ld steps. Deploying arms.\n",
                                  currentSteps);
                    // Deploy arms to final position
                    startServoMove(servo1UsFor(targetLocation), servo2UsFor(targetLocation));
                    state = STATE_MOVING_TO_TARGET_ARM;
                }
            }
            break;
        }

        // STATE_MOVING_STEPPER is handled inline above (blocking)
        case STATE_MOVING_STEPPER:
            break;

        // ── 2. Deploy arms to final angle ─────────────────────────────────────
        case STATE_MOVING_TO_TARGET_ARM:
        {
            if (tickServoMove())
            {
                currentLocation = targetLocation;
                justArrived = true;
                state = STATE_IDLE;
                Serial.printf("[RobotArm] Arrived at location %d\n", currentLocation);
            }
            break;
        }
        }
    }

    State getState() { return state; }
    int getCurrentLocation() { return currentLocation; }
    bool isIdle() { return state == STATE_IDLE; }

    bool checkAndClearArrival()
    {
        if (justArrived)
        {
            justArrived = false;
            return true;
        }
        return false;
    }

} // namespace RobotArm
