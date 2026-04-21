# ESP32 Hand Gesture Car Control

This project has two PlatformIO firmware folders:

- `transmeter/`: the hand-held controller with an `ESP32 + MPU6050`
- `car-receiver/`: the car controller with an `ESP32 + L298N motor driver`

The transmitter reads motion from the MPU6050 and sends one of these commands over ESP-NOW:

- `FORWARD`
- `BACKWARD`
- `LEFT`
- `RIGHT`
- `STOP`

The receiver gets the command and drives the car motors through the L298N module.

## Project structure

```text
remote-car-mpu6050-transmeter/
├── transmeter/
│   ├── platformio.ini
│   └── src/main.cpp
└── car-receiver/
    ├── platformio.ini
    └── src/main.cpp
```

## Hardware needed

- 2x ESP32 DevKit boards
- 1x MPU6050 sensor
- 1x L298N motor driver
- 1x car chassis with 2 DC motors
- Battery for the car motors
- USB cable for each ESP32
- Jumper wires

## 1. Hand unit wiring: MPU6050 to ESP32

The transmitter code uses the default ESP32 I2C pins. On `esp32doit-devkit-v1`, that is usually:

- `GPIO21` = `SDA`
- `GPIO22` = `SCL`

Connect the MPU6050 like this:

| MPU6050 pin | ESP32 pin |
| --- | --- |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SDA` | `GPIO21` |
| `SCL` | `GPIO22` |

Not used in this code:

- `INT`
- `XDA`
- `XCL`
- `AD0`

Important:

- Power the MPU6050 from `3.3V`, not from the ESP32 `5V` pin.
- If your MPU6050 breakout has different voltage requirements, follow the sensor board label.

## 2. Car wiring: ESP32 to L298N

The receiver code uses these pins:

| ESP32 pin | L298N pin | Purpose |
| --- | --- | --- |
| `GPIO5` | `ENA` | Motor A speed PWM |
| `GPIO22` | `IN1` | Motor A direction |
| `GPIO21` | `IN2` | Motor A direction |
| `GPIO23` | `ENB` | Motor B speed PWM |
| `GPIO19` | `IN3` | Motor B direction |
| `GPIO18` | `IN4` | Motor B direction |
| `GND` | `GND` | Common ground |

Motor power wiring:

- Connect the car battery to the L298N motor power input.
- Connect the two motors to the L298N motor output terminals.
- Connect `ESP32 GND` to `L298N GND`.

Important:

- Do not power the motors from the ESP32 `3.3V` pin.
- The ESP32 and L298N must share the same ground.
- Power the ESP32 with USB or a proper regulated supply.

## 3. Upload the receiver code to the car ESP32

Open the `car-receiver` folder as a PlatformIO project, or use the terminal in that folder.

### Using PlatformIO in VS Code

1. Connect the car ESP32 with USB.
2. Open the `car-receiver` project.
3. Build the project.
4. Upload the project.
5. Open Serial Monitor at `115200` baud if you want to check messages.

### Using PlatformIO CLI

```bash
cd car-receiver
pio run
pio run -t upload
pio device monitor -b 115200
```

When it starts correctly, the receiver should print:

```text
Receiver ready (L298N + ESP32 v3)!
```

## 4. Set the receiver MAC address in the transmitter code

Before uploading the hand controller, make sure the transmitter is sending to the correct MAC address of the car ESP32.

In `transmeter/src/main.cpp`, the current receiver MAC is:

```cpp
uint8_t receiverMAC[] = {0x00, 0x4B, 0x12, 0x2C, 0xAB, 0xB0};
```

If your car ESP32 has a different Wi-Fi station MAC address, change this value before uploading the transmitter firmware.

## 5. Upload the transmitter code to the hand ESP32

Open the `transmeter` folder as a PlatformIO project, or use the terminal in that folder.

### Using PlatformIO in VS Code

1. Connect the hand ESP32 with USB.
2. Open the `transmeter` project.
3. Build the project.
4. Upload the project.
5. Open Serial Monitor at `115200` baud to see sensor values and sent commands.

### Using PlatformIO CLI

```bash
cd transmeter
pio run
pio run -t upload
pio device monitor -b 115200
```

The transmitter project depends on these libraries from `transmeter/platformio.ini`:

- `Adafruit MPU6050`
- `Adafruit Unified Sensor`
- `Adafruit BusIO`

PlatformIO installs them automatically during build.

## 6. How the gesture control works

The hand controller reads acceleration and gyro data from the MPU6050 and converts motion to commands:

- tilt/move forward -> `FORWARD`
- tilt/move backward -> `BACKWARD`
- rotate one way -> `LEFT`
- rotate the other way -> `RIGHT`
- no strong movement -> `STOP`

The receiver listens for those commands and drives the motors.

## 7. Running the full system

1. Wire the `car-receiver` ESP32 to the L298N.
2. Wire the `transmeter` ESP32 to the MPU6050.
3. Upload the receiver firmware to the car ESP32.
4. Make sure the transmitter code contains the correct receiver MAC address.
5. Upload the transmitter firmware to the hand ESP32.
6. Power the car.
7. Move the hand unit and watch the commands in Serial Monitor.

## 8. Troubleshooting

### `Failed to find MPU6050 chip`

Check:

- `VCC` to `3V3`
- `GND` to `GND`
- `SDA` to `GPIO21`
- `SCL` to `GPIO22`

### `Failed to add peer` or ESP-NOW does not control the car

Check:

- the receiver MAC address in `transmeter/src/main.cpp`
- both boards are powered
- both boards use ESP32 Wi-Fi in station mode

### Motors do not move

Check:

- L298N wiring
- battery connection to the L298N
- common ground between ESP32 and L298N
- motor wires connected to the correct L298N output terminals

## Notes

- Both PlatformIO projects use the `esp32doit-devkit-v1` board.
- The transmitter uses `ESP-NOW`, so both boards must be ESP32 boards.
- Serial monitor speed for both projects is `115200`.
