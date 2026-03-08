#include "RobotArm.h"
#include "Config.h"
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
    // ─── Location lookup tables ───────────────────────────────────────────────
    static long stepperPosFor(int loc)
    {
        switch (loc)
        {
            case 1: return STEPPER_POS_PLOT1;
            case 2: return STEPPER_POS_PLOT2;
            case 3: return STEPPER_POS_PLOT3;
            case 4: return STEPPER_POS_PLOT4;
            default: return 0;
        }
    }
    
    // Both servos now use the identical deployed position for all 4 plots
    static int servo1UsFor(int loc)
    {
        return SERVO1_DEPLOY_US;
    }
    
    static int servo2UsFor(int loc)
    {
        return SERVO2_DEPLOY_US;
    }

    // ─── Module state ─────────────────────────────────────────────────────────
    static State state = STATE_IDLE;
    static int currentLocation = 0; // 0 = homed, location not yet known
    static int targetLocation = 0;
    static bool justArrived = false;
    static bool justStartedCameraWait = false;

    static long currentSteps = 0;
    static long targetSteps = 0;
    static bool abortRequested = false; // set by stop() to break stepper loop early
    static AbortPollFn abortPollFn = nullptr; // optional callback polled during stepper move

    // Hardware Emergency Pause Button
    static volatile bool motionPaused = false;
    static volatile unsigned long lastInterruptTime = 0;

    // ISR for physical stop button (GPIO26)
    // Toggles pause/resume state with 200ms debounce
    void IRAM_ATTR stopButtonISR()
    {
        unsigned long interruptTime = millis();
        if (interruptTime - lastInterruptTime > 200)
        {
            motionPaused = !motionPaused;
            lastInterruptTime = interruptTime;
        }
    }

    static Servo servo1, servo2;
    static int svCurrent1, svCurrent2; // current microseconds
    static int svStart1, svStart2;
    static int svEnd1, svEnd2;
    static unsigned long svMoveLastTick = 0;
    static unsigned long cameraDelayStart = 0;

    // ─── Servo smooth-move helpers ────────────────────────────────────────────
    // Configures the targets for both servos to move simultaneously
    static void startServoMove(int end1, int end2)
    {
        svStart1 = svCurrent1;
        svStart2 = svCurrent2;
        svEnd1 = end1;
        svEnd2 = end2;
        svMoveLastTick = millis();
    }

    // Returns true when both servos have reached their targets (blocking)
    static bool tickServoMove()
    {
        int updateInterval = 10; // 10ms update interval for simultaneous movement
        
        int diff1 = abs(svEnd1 - svStart1);
        int diff2 = abs(svEnd2 - svStart2);

        // Calculate total steps needed for each servo to hit its target at the desired speed
        // Note: SERVO_SPEED_US_PER_S is in us/s, so (diff / SERVO_SPEED_US_PER_S) gives seconds.
        // Multiply by 1000 to get ms, then divide by updateInterval to get steps.
        float totalTimeMs1 = (diff1 / (float)SERVO_SPEED_US_PER_S) * 1000.0;
        float totalTimeMs2 = (diff2 / (float)SERVO_SPEED_US_PER_S) * 1000.0;

        int steps1 = (int)(totalTimeMs1 / updateInterval);
        int steps2 = (int)(totalTimeMs2 / updateInterval);
        
        // Find the longest path so both servos finish at roughly the same time 
        // (alternatively, they can move at their own independent rates, but picking the max steps
        // forces them to scale their speed to arrive together if desired. Here we will let them move
        // at independent rates but wait until BOTH are finished).
        
        if (steps1 < 1 && diff1 > 0) steps1 = 1;
        if (steps2 < 1 && diff2 > 0) steps2 = 1;

        float inc1 = steps1 > 0 ? (float)(svEnd1 - svStart1) / steps1 : 0;
        float inc2 = steps2 > 0 ? (float)(svEnd2 - svStart2) / steps2 : 0;

        int maxSteps = max(steps1, steps2);

        for (int i = 0; i <= maxSteps; i++)
        {
            // Pause handling
            if (motionPaused)
            {
                Serial.println("[RobotArm] Paused by Emergency Stop button");
                while (motionPaused)
                {
                    delay(10);
                    yield();
                }
                Serial.println("[RobotArm] Resumed");
            }

            // Calculate current position for this step
            if (i <= steps1) {
                svCurrent1 = svStart1 + (int)(inc1 * i);
                servo1.writeMicroseconds(svCurrent1);
            }
            if (i <= steps2) {
                svCurrent2 = svStart2 + (int)(inc2 * i);
                servo2.writeMicroseconds(svCurrent2);
            }

            delay(updateInterval);
        }

        // Final snap to target to correct any float truncation errors
        svCurrent1 = svEnd1;
        svCurrent2 = svEnd2;
        servo1.writeMicroseconds(svCurrent1);
        servo2.writeMicroseconds(svCurrent2);

        return true;
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
        pinMode(PIN_STOP_BUTTON, INPUT_PULLUP);

        attachInterrupt(digitalPinToInterrupt(PIN_STOP_BUTTON), stopButtonISR, FALLING);

        servo1.attach(SERVO1_PIN);
        servo2.attach(SERVO2_PIN);

        // Park servos at idle position
        svCurrent1 = SERVO1_IDLE_US;
        svCurrent2 = SERVO2_IDLE_US;
        servo1.writeMicroseconds(svCurrent1);
        servo2.writeMicroseconds(svCurrent2);

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

        // Step 1: retract arms to idle position before stepper moves
        startServoMove(SERVO1_IDLE_US, SERVO2_IDLE_US);
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

                        // Pause handling
                        if (motionPaused)
                        {
                            Serial.println("[RobotArm] Paused by Emergency Stop button");
                            while (motionPaused)
                            {
                                delay(10);
                                yield();
                            }
                            Serial.println("[RobotArm] Resumed");
                        }
                    }

                    if (abortRequested)
                    {
                        // Abort: retract arms to neutral and go idle
                        abortRequested = false;
                        Serial.println("[RobotArm] Abort! Retracting arms.");
                        startServoMove(SERVO1_IDLE_US, SERVO2_IDLE_US);
                        state = STATE_STOPPING;
                    }
                    else
                    {
                        Serial.printf("[RobotArm] Stepper done at %ld steps. Waiting 5000ms for camera...\n",
                                      currentSteps);
                        cameraDelayStart = millis();
                        justStartedCameraWait = true;
                        state = STATE_CAMERA_DELAY;
                    }
                }
            }
            break;
        }

        // STATE_MOVING_STEPPER is handled inline above (blocking)
        case STATE_MOVING_STEPPER:
            break;

        // ── 1.5. Pause 5 seconds for camera capture ───────────────────────────
        case STATE_CAMERA_DELAY:
        {
            if (millis() - cameraDelayStart >= 5000)
            {
                Serial.println("[RobotArm] Camera delay complete. Deploying arms.");
                startServoMove(servo1UsFor(targetLocation), servo2UsFor(targetLocation));
                state = STATE_MOVING_TO_TARGET_ARM;
            }
            break;
        }

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

                // Pause handling
                if (motionPaused)
                {
                    Serial.println("[RobotArm] Paused by Emergency Stop button");
                    while (motionPaused)
                    {
                        delay(10);
                        yield();
                    }
                    Serial.println("[RobotArm] Resumed");
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
                svEnd1 = SERVO1_IDLE_US;
                svEnd2 = SERVO2_IDLE_US;
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

        // Step 1: retract servos to idle before homing
        startServoMove(SERVO1_IDLE_US, SERVO2_IDLE_US);
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

    bool checkAndClearCameraReady()
    {
        if (justStartedCameraWait)
        {
            justStartedCameraWait = false;
            return true;
        }
        return false;
    }

    void setAbortPollFn(AbortPollFn fn)
    {
        abortPollFn = fn;
    }

} // namespace RobotArm
