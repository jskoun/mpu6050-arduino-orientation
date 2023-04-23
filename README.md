# mpu6050-arduino-orientation
A simple code that does not use too many libraries, to control an MPU6050 chip (tested on a GY-521). 

-By reading linear acceleration and angular velocity, orientation measurements are produced.
-Because the error and offsets are generally device specific, one should first test and tweak the 6 OFFSET parameters. Also use the code to read sensor configuration and change measurement scales accordingly.
To be added:
*Auto scaling based on reading config on startup
*Function for OFFSET setting based on readings
