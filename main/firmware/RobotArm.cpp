#include "RobotArm.h"
#include <Arduino.h>

namespace RobotArm
{

    void begin()
    {
        // Initialize motors, endstops, etc.
    }

    void moveToPlot(int plotNumber)
    {
        Serial.printf("RobotArm: moving to plot %d\n", plotNumber);
        // Replace with stepper/servo control; simulate delay
        delay(1500);
        Serial.println("RobotArm: arrived");
    }

} // namespace RobotArm
