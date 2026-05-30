
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
* Serial commands such as `PWM_OFF`, `PWM_SLOW`, `PWM <value>`, `RESET_COUNTS`, `CAL_START`, `CAL_STOP <revolutions>`, `BRIDGE_HEARTBEAT`, `PING`, `LED_ON`, and `LED_OFF`
* PWM defaults to 0 on boot and nonzero PWM commands require an active ROS2 bridge heartbeat
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
make ros-build
```

Run the serial bridge:

```bash
make bridge
```

In a second terminal, publish a command:

```bash
cd ~/mower_ws
make led-on
make led-off
```

Compile Arduino firmware:

```bash
make arduino-build
```

Upload Arduino firmware:

```bash
make arduino-upload
```

Show all shortcut commands:

```bash
make help
```

## Reading Hall Sensor Counts in the Terminal

The current Arduino firmware counts speed pulses from the BLDC driver speed outputs:

```text
Left speed input:  Arduino D2
Right speed input: Arduino D3
Baud rate:         115200
Serial port:       /dev/ttyACM0
```

Only one program can use `/dev/ttyACM0` at a time. Stop the ROS2 `arduino_bridge` before opening the Arduino serial monitor directly.

Start the serial monitor from the Pi:

```bash
cd ~/mower_ws
make monitor
```

After reset, the Arduino should print:

```text
BOOT UNO_R4_SPEED_PWM_TOTAL_TEST
PWM_START 0
```

The firmware then prints hall/speed counts about every 200 ms:

```text
SPEED_COUNTS 7 0 HZ 35.0 0.0 TOTAL 587 0 PWM 20
```

Field meaning:

```text
SPEED_COUNTS <left_count_this_window> <right_count_this_window>
HZ           <left_frequency_hz>     <right_frequency_hz>
TOTAL        <left_total_count>      <right_total_count>
PWM          <current_pwm_value>
```

For the example above:

* `7 0` means 7 left pulses and 0 right pulses during the latest 200 ms window.
* `35.0 0.0` means the left speed signal is currently about 35 Hz and the right is 0 Hz.
* `587 0` means the Arduino has counted 587 total left pulses and 0 total right pulses since boot or the last reset.
* `PWM 20` means the PWM output on D9 is currently set to 20 out of 255.

Useful commands can be typed into the serial monitor and sent with Enter:

```text
RESET_COUNTS
PWM_OFF
PWM_SLOW
PWM 20
PWM 30
PWM_UP
PWM_DOWN
STATUS
CAL_START
CAL_STATUS
CAL_STOP 10
CAL_CANCEL
PING
```

Suggested test flow:

Use `make bridge` in one terminal, then send motor commands through ROS2 from a second terminal. The bridge sends `BRIDGE_HEARTBEAT` automatically.

```bash
cd ~/mower_ws
make reset
make pwm VALUE=20
```

The firmware stops PWM if the bridge heartbeat is missing for about 1 second. Direct serial monitor mode is best for reading counts and calibration; motor motion commands should go through the bridge heartbeat path. Increase PWM slowly only when the mower is safely supported, the wheels are clear, and the motor driver wiring has been checked.

## Calibrating Speed Counts Per Revolution

The firmware can measure how many speed pulses correspond to a known number of wheel revolutions.

Basic direct-serial calibration flow:

```text
CAL_START
```

Rotate the wheel by a known number of revolutions, for example 10 full wheel turns. Then send:

```text
CAL_STOP 10
```

The Arduino prints a result like:

```text
CAL_RESULT REVS 10.000 LEFT_COUNTS 420 RIGHT_COUNTS 0 LEFT_COUNTS_PER_REV 42.000 RIGHT_COUNTS_PER_REV 0.000
```

Latest measured calibration:

```text
speed_counts_per_wheel_rev: 205
```

This means:

```text
1 wheel revolution = 205 speed counts
1 speed count      = 1 / 205 wheel revolutions
1 speed count      = 2*pi / 205 wheel radians
```

Useful calibration commands:

```text
CAL_START             Start a calibration window from the current total counts
CAL_STATUS            Show counts accumulated since CAL_START
CAL_STOP <revs>       End calibration and print counts per revolution
CAL_CANCEL            Exit calibration without calculating a result
```

For best accuracy, calibrate one wheel at a time, use several revolutions, and keep the wheel turning in one direction. If only one motor speed output is connected, the other side should stay near zero.

If the ROS2 bridge is running instead of the direct serial monitor, send commands from a second terminal:

```bash
cd ~/mower_ws
make reset
make pwm VALUE=20
make cal-start
make cal-stop REVS=10
make pwm-off
```

The ROS2 bridge continuously drains and logs Arduino serial output while it is running, including `SPEED_COUNTS` telemetry and command replies. This keeps the Arduino from blocking on serial output after the first command. The direct `arduino-cli monitor` terminal is still useful when you want to interact with the Arduino without ROS2 in the middle.

## Near-Term Development Goals

1. Connect and test both BLDC motor driver speed pulse outputs.
2. Verify the 205 counts/rev calibration on both wheels.
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
