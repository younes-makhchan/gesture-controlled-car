#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

typedef struct struct_message {
    char command[10];
} struct_message;

struct_message myData;

const float gyroZThreshold = 0.7; // rad/s, about 40 deg/s
const float accelYThreshold = 5.0;

void setup() {
    Serial.begin(115200);
    Serial.println("\nStarting MPU6050 Gesture Debug...");

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (true) {
            delay(10);
        }
    }

    Serial.println("MPU6050 initialized successfully");
}

void loop() {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float gz = g.gyro.z;

    float gzDeg = gz * 180.0 / PI;

    if (gz > gyroZThreshold) {
        strcpy(myData.command, "RIGHT");
    } else if (gz < -gyroZThreshold) {
        strcpy(myData.command, "LEFT");
    } else if (ay > accelYThreshold) {
        strcpy(myData.command, "FORWARD");
    } else if (ay < -accelYThreshold) {
        strcpy(myData.command, "BACKWARD");
    } else {
        strcpy(myData.command, "STOP");
    }

    Serial.print("AX:");
    Serial.print(ax, 2);
    Serial.print(" AY:");
    Serial.print(ay, 2);
    Serial.print(" GZ:");
    Serial.print(gzDeg, 1);
    Serial.print("d/s | Cmd:");
    Serial.println(myData.command);

    delay(100);
}
