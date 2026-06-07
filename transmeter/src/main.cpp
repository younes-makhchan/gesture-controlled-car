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

// ======================================================
// ROTATION TUNING
// ======================================================
// rotationAxisChangeThreshold: raise if normal gestures trigger rotation,
// lower if rotation is not detected.
// rotationCooldownMs: raise if one circle sends repeated rotate commands.
const float rotationAxisChangeThreshold = 2.0f;
const unsigned long rotationCooldownMs = 600;
const bool mpuDebugLogs = true;
const bool rotationDebugLogs = true;

float gravityX = 0.0f;
float gravityY = 0.0f;
float gravityZ = 0.0f;

unsigned long lastRotationCommandMs = 0;

float debugFx = 0.0f;
float debugFy = 0.0f;
float debugFxDelta = 0.0f;
float debugFyDelta = 0.0f;
bool debugRotationCandidate = false;

float previousFx = 0.0f;
float previousFy = 0.0f;
bool hasPreviousMappedAxes = false;

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
    CMD_ROTATION,
    CMD_COUNT
};

const char *commandNames[CMD_COUNT] = {
    "STOP",
    "FORWARD",
    "BACKWARD",
    "MOVE_RIGHT",
    "MOVE_LEFT",
    "ROTATION",
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

    // FIX AXIS ORIENTATION (MOST IMPORTANT PART)
    float fx = ay; // forward/back now based on Y
    float fy = ax; // left/right now based on X

    debugFx = fx;
    debugFy = fy;

    if (hasPreviousMappedAxes)
    {
        debugFxDelta = fx - previousFx;
        debugFyDelta = fy - previousFy;
    }
    else
    {
        debugFxDelta = 0.0f;
        debugFyDelta = 0.0f;
        hasPreviousMappedAxes = true;
    }

    previousFx = fx;
    previousFy = fy;

    unsigned long now = millis();

    debugRotationCandidate = fabs(debugFxDelta) >= rotationAxisChangeThreshold &&
                             fabs(debugFyDelta) >= rotationAxisChangeThreshold;

    if (debugRotationCandidate && now - lastRotationCommandMs >= rotationCooldownMs)
    {
        lastRotationCommandMs = now;
        return CMD_ROTATION;
    }

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
        return CMD_MOVE_LEFT;

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

    if (mpuDebugLogs)
    {
        Serial.print("MPU ax=");
        Serial.print(a.acceleration.x);
        Serial.print(" ay=");
        Serial.print(a.acceleration.y);
        Serial.print(" az=");
        Serial.print(a.acceleration.z);
        Serial.print(" gx=");
        Serial.print(g.gyro.x);
        Serial.print(" gy=");
        Serial.print(g.gyro.y);
        Serial.print(" gz=");
        Serial.print(g.gyro.z);
        Serial.print(" temp=");
        Serial.print(t.temperature);
        Serial.print(" fx=");
        Serial.print(debugFx);
        Serial.print(" fy=");
        Serial.print(debugFy);
        Serial.print(" dfx=");
        Serial.print(debugFxDelta);
        Serial.print(" dfy=");
        Serial.print(debugFyDelta);
        Serial.print(" cmd=");
        Serial.println(myData.command);
    }

    if (rotationDebugLogs)
    {
        Serial.print("ROTATION_CHECK dfx=");
        Serial.print(debugFxDelta);
        Serial.print(" dfy=");
        Serial.print(debugFyDelta);
        Serial.print(" candidate=");
        Serial.print(debugRotationCandidate ? "YES" : "NO");
        Serial.print(" cmd=");
        Serial.println(myData.command);
    }

    delay(100);
}
