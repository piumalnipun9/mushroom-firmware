#include <ESP32Servo.h>
#include <Arduino.h>

// ─── Hardware pins ────────────────────────────────────────────────────────
#define SERVO1_PIN 18
#define SERVO2_PIN 19

// Movement speed (microseconds of pulse width change per second)
const float SERVO_SPEED_US_PER_S = 3000.0;

// ─── State ────────────────────────────────────────────────────────────────
Servo servo1;
Servo servo2;

int svCurrent1 = 1500; // start at true centre
int svCurrent2 = 1500;

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
    
    Serial.println("Move complete. Enter new positions (e.g. 1300 1800):");
}

void setup()
{
    Serial.begin(9600);
    delay(1000);
    
    Serial.println("\n\n--- Interactive Servo Tester ---");
    Serial.println("Enter two microsecond values separated by a space or comma.");
    Serial.println("Example:  1300 1800");
    
    // Allocate timers for ESP32 servos
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    
    servo1.setPeriodHertz(50);
    servo2.setPeriodHertz(50);
    
    servo1.attach(SERVO1_PIN);
    servo2.attach(SERVO2_PIN);

    // Start at a safe centre
    servo1.writeMicroseconds(svCurrent1);
    servo2.writeMicroseconds(svCurrent2);
    Serial.println("Servos initialized to 1500 1500.");
    Serial.println("Waiting for input...");
}

void loop()
{
    if (Serial.available() > 0)
    {
        // Read the incoming string until a newline
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() > 0)
        {
            // Parse two integers from the string
            int spaceIndex = input.indexOf(' ');
            if (spaceIndex == -1) spaceIndex = input.indexOf(',');
            
            if (spaceIndex > 0)
            {
                String val1Str = input.substring(0, spaceIndex);
                String val2Str = input.substring(spaceIndex + 1);
                
                int target1 = val1Str.toInt();
                int target2 = val2Str.toInt();
                
                // Basic validation (standard servo pulse ranges are roughly 500 to 2500)
                if (target1 >= 500 && target1 <= 2500 && target2 >= 500 && target2 <= 2500)
                {
                    Serial.printf("\nReceived command: %d %d\n", target1, target2);
                    moveServosSimultaneously(svCurrent1, svCurrent2, target1, target2);
                }
                else
                {
                    Serial.println("Error: Values out of bounds! Must be between 500 and 2500 microseconds.");
                }
            }
            else
            {
                Serial.println("Error: Please enter TWO numbers separated by a space (e.g. 1450 1100).");
            }
        }
    }
}
