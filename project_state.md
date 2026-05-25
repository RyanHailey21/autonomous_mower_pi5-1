Here’s a brief repo-ready summary you can put in `README.md` or `docs/project_summary.md`.

````markdown
# Autonomous Mower Pi 5 Project

This project is a ROS2-based autonomous mower platform using a Raspberry Pi 5 as the main computer and an Arduino Uno R4 Minima as the low-level motor/sensor interface.

## Current System Status

The Raspberry Pi 5 is running Ubuntu Server 24.04 with ROS2 Jazzy installed. Development is done from a Windows machine using VS Code Remote SSH into the Pi.

The Arduino Uno R4 Minima is connected to the Pi over USB and appears as:

```bash
/dev/ttyACM0
````

Arduino CLI is installed on the Pi, and the Uno R4 Minima target is:

```bash
arduino:renesas_uno:minima
```

The current ROS2 package is:

```bash
mower_serial
```

It contains a Python ROS2 node named:

```bash
arduino_bridge
```

The bridge subscribes to:

```bash
/mower/arduino_cmd
```

and forwards string commands over USB serial to the Arduino.

End-to-end ROS2 communication has been tested successfully:

```bash
ros2 topic pub --once /mower/arduino_cmd std_msgs/msg/String "{data: 'LED_ON'}"
ros2 topic pub --once /mower/arduino_cmd std_msgs/msg/String "{data: 'LED_OFF'}"
```

The Arduino replied:

```text
OK LED_ON
OK LED_OFF
```

## Hardware Overview

Main compute:

* Raspberry Pi 5, 16GB
* Ubuntu Server 24.04
* ROS2 Jazzy

Low-level controller:

* Arduino Uno R4 Minima
* Connected to Pi over USB-C
* Used for PWM output, speed pulse counting, and future motor interface logic

Power:

* 24V 20Ah LiFePO4 battery
* Two UCTRONICS U6112 24V-to-5V converters
* One 5V rail intended for Pi
* One 5V rail intended for sensors/accessories
* 6-way PBT fuse box for 24V distribution

Drive system:

* Two LINIX 250W BLDC motors
* Two ZS-X11H BLDC drivers
* Drivers accept 0–5V analog speed command via PWM-to-voltage bridge
* ZS-X11H speed pulse output has been tested with Arduino interrupt input

Cutting system:

* 775 DC motor
* BTN7960B H-bridge
* 20 cm manganese steel blade

Sensors:

* YDLIDAR X2L 2D LiDAR
* AS5600 magnetic encoders via TCA9548A I2C multiplexer
* ICM-20948 IMU
* ATGM336H GPS

## Current Arduino Test Firmware

The Arduino firmware currently supports:

* PWM output on pin D9
* Left speed pulse input on D2
* Right speed pulse input on D3
* Serial commands such as `PWM_OFF`, `PWM_SLOW`, `PWM <value>`, `RESET_COUNTS`, `PING`, `LED_ON`, and `LED_OFF`
* Periodic serial output of speed pulse counts, frequency, total counts, and PWM value

Example output:

```text
SPEED_COUNTS 7 0 HZ 35.0 0.0 TOTAL 587 0 PWM 20
```

Only one motor was connected during the latest test. At `PWM 20`, the connected ZS-X11H speed output produced a stable signal around 35–40 Hz.

## Current Repo Layout

```text
mower_ws/
  src/
    mower_serial/
      mower_serial/
        arduino_bridge.py
      package.xml
      setup.py
  firmware/
    uno_r4_minima/
      uno_r4_minima.ino
  .gitignore
```

ROS2 build artifacts are intentionally ignored:

```text
build/
install/
log/
__pycache__/
*.pyc
.vscode/
```

## Build and Run

Build the ROS2 workspace:

```bash
cd ~/mower_ws
colcon build
source install/setup.bash
```

Run the serial bridge:

```bash
ros2 run mower_serial arduino_bridge
```

In a second terminal, publish a command:

```bash
source /opt/ros/jazzy/setup.bash
source ~/mower_ws/install/setup.bash

ros2 topic pub --once /mower/arduino_cmd std_msgs/msg/String "{data: 'LED_ON'}"
```

Compile Arduino firmware:

```bash
arduino-cli compile --fqbn arduino:renesas_uno:minima ~/mower_ws/firmware/uno_r4_minima
```

Upload Arduino firmware:

```bash
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:renesas_uno:minima ~/mower_ws/firmware/uno_r4_minima
```

## Near-Term Development Goals

1. Connect and test both BLDC motor driver speed pulse outputs.
2. Calibrate pulses per wheel revolution.
3. Convert pulse counts to wheel speed.
4. Add left/right PWM outputs for differential drive.
5. Update the serial protocol from raw string commands to structured mower commands.
6. Publish wheel speed or odometry data into ROS2.
7. Add IMU integration and fuse wheel odometry with IMU using `robot_localization`.
8. Later integrate LiDAR, GPS, Nav2, and full mower behavior.

## Intended Long-Term Architecture

The Arduino should act as the low-level hardware interface. It will handle PWM outputs, speed pulse counting, and basic motor feedback.

The Raspberry Pi should run ROS2, including sensor drivers, odometry processing, localization, planning, and high-level control.

Planned control/data flow:

```text
ROS2 /cmd_vel
  -> Pi serial bridge or ros2_control hardware interface
  -> Arduino Uno R4 Minima
  -> PWM-to-voltage bridge
  -> ZS-X11H motor drivers
  -> BLDC motors

ZS-X11H speed pulse output
  -> Arduino interrupt pins
  -> serial feedback to Pi
  -> ROS2 wheel speed / odometry
  -> robot_localization with IMU
```

The project may eventually migrate from the custom serial bridge to a `ros2_control` hardware interface with `diff_drive_controller`.

```
```
