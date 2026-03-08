#include <ESP32Servo.h>
#include <Arduino.h>

// ─── Hardware pins ────────────────────────────────────────────────────────
#define SERVO1_PIN 18
#define SERVO2_PIN 19

// ─── Servo Angles (User Configured) ──────────────────────────────────────
const int SERVO1_IDLE_US   = 1600;
const int SERVO2_IDLE_US   = 1100;

const int SERVO1_DEPLOY_US = 1300; 
const int SERVO2_DEPLOY_US = 1800;

// Movement speed (microseconds of pulse width change per second)
const float SERVO_SPEED_US_PER_S = 3000.0;

// ─── State ────────────────────────────────────────────────────────────────
Servo servo1;
Servo servo2;

int svCurrent1 = SERVO1_IDLE_US;
int svCurrent2 = SERVO2_IDLE_US;

bool isDeployed = false;

// ─── Simultaneous Move Function ───────────────────────────────────────────
void moveServosSimultaneously(int start1, int start2, int end1, int end2)
{
    int updateInterval = 10; // 10ms update interval for simultaneous movement
    
    int diff1 = abs(end1 - start1);
    int diff2 = abs(end2 - start2);

    float totalTimeMs1 = (diff1 / SERVO_SPEED_US_PER_S) * 1000.0;
    float totalTimeMs2 = (diff2 / SERVO_SPEED_US_PER_S) * 1000.0;

    int steps1 = (int)(totalTimeMs1 / updateInterval);
    int steps2 = (int)(totalTimeMs2 / updateInterval);
    
    if (steps1 < 1 && diff1 > 0) steps1 = 1;
    if (steps2 < 1 && diff2 > 0) steps2 = 1;

    float inc1 = steps1 > 0 ? (float)(end1 - start1) / steps1 : 0;
    float inc2 = steps2 > 0 ? (float)(end2 - start2) / steps2 : 0;

    int maxSteps = max(steps1, steps2);

    Serial.printf("Moving S1: %d->%d, S2: %d->%d\n", start1, end1, start2, end2);

    for (int i = 0; i <= maxSteps; i++)
    {
        if (i <= steps1) {
            svCurrent1 = start1 + (int)(inc1 * i);
            servo1.writeMicroseconds(svCurrent1);
        }
        if (i <= steps2) {
            svCurrent2 = start2 + (int)(inc2 * i);
            servo2.writeMicroseconds(svCurrent2);
        }
        delay(updateInterval);
    }

    // Final snap to target to correct any float truncation errors
    svCurrent1 = end1;
    svCurrent2 = end2;
    servo1.writeMicroseconds(svCurrent1);
    servo2.writeMicroseconds(svCurrent2);
    
    Serial.println("Move complete.");
}

void setup()
{
    Serial.begin(9600);
    Serial.println("Starting Servo Simultaneous Movement Test");
    
    // Allocate timers for ESP32 servos
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    
    servo1.setPeriodHertz(50);
    servo2.setPeriodHertz(50);
    
    servo1.attach(SERVO1_PIN);
    servo2.attach(SERVO2_PIN);

    // Start at IDLE
    servo1.writeMicroseconds(SERVO1_IDLE_US);
    servo2.writeMicroseconds(SERVO2_IDLE_US);
    delay(2000); // Wait 2s before starting ping-pong loop
}

void loop()
{
    if (!isDeployed)
    {
        // Move to DEPLOY
        moveServosSimultaneously(SERVO1_IDLE_US, SERVO2_IDLE_US, SERVO1_DEPLOY_US, SERVO2_DEPLOY_US);
        isDeployed = true;
    }
    else
    {
        // Move back to IDLE
        moveServosSimultaneously(SERVO1_DEPLOY_US, SERVO2_DEPLOY_US, SERVO1_IDLE_US, SERVO2_IDLE_US);
        isDeployed = false;
    }
    
    // Wait 5 seconds between movements so you can observe the resting positions
    delay(5000);
}
