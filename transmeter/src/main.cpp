#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <esp_now.h>
#include <WiFi.h>
#include <math.h>
#include <string.h>

Adafruit_MPU6050 mpu;

struct EspNowMessage
{
    char command[20];
};

EspNowMessage myData;

// CHANGE THIS TO YOUR RECEIVER MAC
uint8_t receiverMAC[] = {0x00, 0x70, 0x07, 0xA6, 0x24, 0x7C};

// ======================================================
// THRESHOLDS
// ======================================================

const float accelForwardThreshold = 5.0f;
const float accelSideThreshold = 5.0f;
const float gravityAlpha = 0.02f;

float gravityX = 0.0f;
float gravityY = 0.0f;
float gravityZ = 0.0f;

// ======================================================
// COMMAND SYSTEM (SIMPLIFIED)
// ======================================================

enum GestureCommand
{
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_MOVE_RIGHT,
    CMD_MOVE_LEFT,
    CMD_COUNT
};

const char *commandNames[CMD_COUNT] = {
    "STOP",
    "FORWARD",
    "BACKWARD",
    "MOVE_RIGHT",
    "MOVE_LEFT",
};

// ======================================================
// ESP-NOW CALLBACK
// ======================================================

void onSent(const uint8_t *macAddress, esp_now_send_status_t status)
{
    Serial.print("Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// ======================================================
// SIMPLE GESTURE DETECTION
// ======================================================

int detectGesture(float ax, float ay, float az)
{
    gravityX = gravityX * (1.0f - gravityAlpha) + ax * gravityAlpha;
    gravityY = gravityY * (1.0f - gravityAlpha) + ay * gravityAlpha;

    // 🔁 FIX AXIS ORIENTATION (MOST IMPORTANT PART)
    float fx = ay;   // forward/back now based on Y
    float fy = ax;   // left/right now based on X

    // DEAD ZONE SAFETY (prevents random STOP spam)
    if (fabs(fx) < 3.0f && fabs(fy) < 3.0f)
        return CMD_STOP;

    // forward = finger down
    if (fx > accelForwardThreshold)
        return CMD_BACKWARD;

    // backward = finger up
    if (fx < -accelForwardThreshold)
        return CMD_FORWARD;

    // right tilt
    if (fy > accelSideThreshold)
        return CMD_MOVE_LEFT ;

    // left tilt
    if (fy < -accelSideThreshold)
        return CMD_MOVE_RIGHT;

    return CMD_STOP;
}
// ======================================================
// SETUP
// ======================================================

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    // MPU
    if (!mpu.begin())
    {
        Serial.println("MPU6050 not found");
        while (1)
            delay(10);
    }

    // ESP-NOW init
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW init failed");
        return;
    }

    esp_now_register_send_cb(onSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }

    Serial.print("Transmitter MAC: ");
    Serial.println(WiFi.macAddress());
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
    sensors_event_t a, g, t;
    mpu.getEvent(&a, &g, &t);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;

    int cmd = detectGesture(ax, ay, a.acceleration.z);

    strlcpy(myData.command,
            commandNames[cmd],
            sizeof(myData.command));

    esp_now_send(receiverMAC,
                 (uint8_t *)&myData,
                 sizeof(myData));

    Serial.print("CMD: ");
    Serial.println(myData.command);

    delay(100);
}