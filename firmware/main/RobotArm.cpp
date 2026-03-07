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
    static bool abortRequested = false; // set by stop() to break stepper loop early
    static AbortPollFn abortPollFn = nullptr; // optional callback polled during stepper move

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

    // Returns true when both servo phases are complete (now blocking)
    static bool tickServoMove()
    {
        int updateInterval = 2; // fast update interval

        // ── Phase 0: move servo 2 to target ──────────────────────────────
        int diff2 = abs(svEnd2 - svStart2);
        if (diff2 > 0)
        {
            float totalTimeMs2 = (diff2 / SERVO_SPEED_US_PER_S) * 1000.0;
            int steps2 = totalTimeMs2 / updateInterval;
            if (steps2 < 1) steps2 = 1;
            
            float inc2 = (float)(svEnd2 - svStart2) / steps2;
            
            // Hold servo 1 steady
            servo1.writeMicroseconds(svCurrent1); 
            
            for (int i = 0; i <= steps2; i++)
            {
                svCurrent2 = svStart2 + (int)(inc2 * i);
                servo2.writeMicroseconds(svCurrent2);
                delay(updateInterval); // Use delay to keep timing precise and let WiFi breathe smoothly
            }
        }
        svCurrent2 = svEnd2;

        // ── Phase 1: move servo 1 to target ──────────────────────────────
        int diff1 = abs(svEnd1 - svStart1);
        if (diff1 > 0)
        {
            float totalTimeMs1 = (diff1 / SERVO_SPEED_US_PER_S) * 1000.0;
            int steps1 = totalTimeMs1 / updateInterval;
            if (steps1 < 1) steps1 = 1;
            
            float inc1 = (float)(svEnd1 - svStart1) / steps1;
            
            // Hold servo 2 steady
            servo2.writeMicroseconds(svCurrent2); 
            
            for (int i = 0; i <= steps1; i++)
            {
                svCurrent1 = svStart1 + (int)(inc1 * i);
                servo1.writeMicroseconds(svCurrent1);
                delay(updateInterval); // Use delay to keep timing precise and let WiFi breathe smoothly
            }
        }
        svCurrent1 = svEnd1;

        return true; // Both phases done
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

        // Step 1: retract arms to centre (idle position) before stepper moves
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
                    // goHome() sets targetLocation=0; regular moves set 1–4
                    if (targetLocation == 0)
                    {
                        // Hand off to STATE_HOMING_STEPPER (drives stepper to position 0)
                        state = STATE_HOMING_STEPPER;
                        break;
                    }

                    // Normal plot move — run stepper to targetSteps inline
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

                        // Yield every 200 steps, call abort-poll callback, check abort flag
                        if (++stepsDone % 200 == 0)
                        {
                            yield();
                            if (abortPollFn) abortPollFn(); // let main.ino check Firebase stop
                            if (abortRequested)
                                break;
                        }
                    }

                    if (abortRequested)
                    {
                        // Abort: retract arms to neutral and go idle
                        abortRequested = false;
                        Serial.println("[RobotArm] Abort! Retracting arms.");
                        startServoMove(SERVO_CENTER_US, SERVO_CENTER_US);
                        state = STATE_STOPPING;
                    }
                    else
                    {
                        Serial.printf("[RobotArm] Stepper done at %ld steps. Deploying arms.\n",
                                      currentSteps);
                        // Deploy arms to final position
                        startServoMove(servo1UsFor(targetLocation), servo2UsFor(targetLocation));
                        state = STATE_MOVING_TO_TARGET_ARM;
                    }
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

        // ── 3. Abort: retract to neutral then idle ─────────────────────────────
        case STATE_STOPPING:
        {
            if (tickServoMove())
            {
                justArrived = true; // triggers Firebase update in main.ino
                state = STATE_IDLE;
                Serial.println("[RobotArm] Stopped. Arms retracted to neutral.");
            }
            break;
        }

        // ── 4. Go home: stepper drives to 0, servos stay at centre ────────────
        case STATE_HOMING_STEPPER:
        {
            // This runs inline (blocking) similar to STATE_MOVING_STEPPER
            Serial.printf("[RobotArm] Homing stepper: %ld -> 0\n", currentSteps);
            digitalWrite(DIR_PIN, currentSteps > 0 ? HIGH : LOW);
            delayMicroseconds(10);

            long stepsDone = 0;
            while (currentSteps != 0)
            {
                digitalWrite(STEP_PIN, HIGH);
                delayMicroseconds(STEP_INTERVAL_US);
                digitalWrite(STEP_PIN, LOW);
                delayMicroseconds(STEP_INTERVAL_US);
                currentSteps > 0 ? currentSteps-- : currentSteps++;

                if (++stepsDone % 200 == 0)
                {
                    yield();
                    if (abortRequested)
                        break;
                }
            }

            abortRequested = false;
            currentLocation = 0;
            justArrived = true;
            state = STATE_IDLE;
            Serial.println("[RobotArm] Stepper homed to 0. At Home.");
            break;
        }
        }
    }

    State getState() { return state; }
    int getCurrentLocation() { return currentLocation; }
    bool isIdle() { return state == STATE_IDLE; }
    bool isStopping() { return state == STATE_STOPPING; }

    bool stop()
    {
        if (state == STATE_IDLE)
            return false; // nothing to stop

        if (state == STATE_MOVING_STEPPER ||
            state == STATE_MOVING_TO_IDLE_ARM ||
            state == STATE_MOVING_TO_TARGET_ARM)
        {
            // Signal the stepper loop to break, or let the current servo
            // phase complete naturally — both paths land in STATE_STOPPING.
            abortRequested = true;

            // If we're in a servo phase, skip straight to stopping state
            // so the servo phase completes and then we go idle safely.
            if (state != STATE_MOVING_STEPPER)
            {
                // Overwrite pending servo target with neutral (retract)
                svEnd1 = SERVO_CENTER_US;
                svEnd2 = SERVO_CENTER_US;
                state = STATE_STOPPING;
            }
            Serial.println("[RobotArm] Stop requested.");
            return true;
        }
        return false;
    }

    bool goHome()
    {
        if (state != STATE_IDLE)
            return false; // busy
        if (currentLocation == 0)
            return false; // already home

        // Step 1: retract servos to centre before homing
        startServoMove(SERVO_CENTER_US, SERVO_CENTER_US);
        state = STATE_MOVING_TO_IDLE_ARM;
        // After retract completes, update() will see currentSteps != 0
        // and enter STATE_HOMING_STEPPER (targetSteps = 0 is set below)
        targetSteps = 0;
        targetLocation = 0; // mark we're heading home
        Serial.println("[RobotArm] Go Home dispatched.");
        return true;
    }

    bool checkAndClearArrival()
    {
        if (justArrived)
        {
            justArrived = false;
            return true;
        }
        return false;
    }

    void setAbortPollFn(AbortPollFn fn)
    {
        abortPollFn = fn;
    }

} // namespace RobotArm
