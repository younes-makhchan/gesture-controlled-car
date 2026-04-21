#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <esp_now.h>
#include <WiFi.h>
Adafruit_MPU6050 mpu;

typedef struct struct_message
{
    char command[10];
} struct_message;

struct_message myData;
uint8_t receiverMAC[] = {0x00, 0x4B, 0x12, 0x2C, 0xAB, 0xB0};
const float gyroThreshold = 0.6; // rad/s, about 40 deg/s
const float accelYThreshold = 5.0;
const float gravityAlpha = 0.02;
float gravityX = 0.0;
float gravityY = 0.0;
float gravityZ = 0.0;

const int commandHistorySize = 5;
int commandHistory[commandHistorySize] = {0};
int historyIndex = 0;

enum GestureCommand
{
    CMD_STOP = 0,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_RIGHT,
    CMD_LEFT,
};

const char *commandNames[] = {"STOP", "FORWARD", "BACKWARD", "RIGHT", "LEFT"};

void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    Serial.print("Message Sent Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    if (!mpu.begin())
    {
        Serial.println("Failed to find MPU6050 chip");
        while (true)
        {
            delay(10);
        }
    }
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return;
    }
    esp_now_register_send_cb(OnSent);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return;
    }
}
void loop()
{
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x;
    float ay = a.acceleration.y;
    float az = a.acceleration.z;
    float gx = g.gyro.x;
    float gy = g.gyro.y;
    float gz = g.gyro.z;

    gravityX = gravityX * (1.0 - gravityAlpha) + ax * gravityAlpha;
    gravityY = gravityY * (1.0 - gravityAlpha) + ay * gravityAlpha;
    gravityZ = gravityZ * (1.0 - gravityAlpha) + az * gravityAlpha;

    float gNorm = sqrt(gravityX * gravityX + gravityY * gravityY + gravityZ * gravityZ);
    float gxDeg = gx * 180.0 / PI;
    float gyDeg = gy * 180.0 / PI;
    float gzDeg = gz * 180.0 / PI;

    float omegaGravity = 0.0;
    if (gNorm > 0.1f)
    {
        omegaGravity = (gx * gravityX + gy * gravityY + gz * gravityZ) / gNorm;
    }

    int currentCmd;
    if (omegaGravity > gyroThreshold)
    {
        currentCmd = CMD_RIGHT;
    }
    else if (omegaGravity < -gyroThreshold)
    {
        currentCmd = CMD_LEFT;
    }
    else if (ay > accelYThreshold)
    {
        currentCmd = CMD_FORWARD;
    }
    else if (ay < -accelYThreshold)
    {
        currentCmd = CMD_BACKWARD;
    }
    else
    {
        currentCmd = CMD_STOP;
    }

    commandHistory[historyIndex] = currentCmd;
    historyIndex = (historyIndex + 1) % commandHistorySize;

    int counts[5] = {0};
    for (int i = 0; i < commandHistorySize; ++i)
    {
        int cmd = commandHistory[i];
        if (cmd >= 0 && cmd <= CMD_LEFT)
        {
            counts[cmd]++;
        }
    }

    int popularCmd = 0;
    int popularCount = counts[0];
    for (int i = 1; i <= CMD_LEFT; ++i)
    {
        if (counts[i] > popularCount)
        {
            popularCount = counts[i];
            popularCmd = i;
        }
    }

    strcpy(myData.command, commandNames[popularCmd]);
    esp_now_send(receiverMAC, (uint8_t *)&myData, sizeof(myData));

    float omegaGravityDeg = omegaGravity * 180.0 / PI;
    Serial.print("AX:");
    Serial.print(ax, 2);
    Serial.print(" AY:");
    Serial.print(ay, 2);
    Serial.print(" GZ:");
    Serial.print(gzDeg, 1);
    Serial.print("d/s OG:");
    Serial.print(omegaGravityDeg, 1);
    Serial.print("d/s | Raw:");
    Serial.print(commandNames[currentCmd]);
    Serial.print(" | Vote:");
    Serial.print(commandNames[popularCmd]);
    Serial.print(" | Cmd:");
    Serial.println(myData.command);

    delay(100);
}
