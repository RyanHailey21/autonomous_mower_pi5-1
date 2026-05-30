SHELL := /bin/bash
.ONESHELL:

PORT ?= /dev/ttyACM0
BAUD ?= 115200
FQBN ?= arduino:renesas_uno:minima
ARDUINO_DIR := firmware/uno_r4_minima
ROS_SETUP := /opt/ros/jazzy/setup.bash

CMD ?= PING
VALUE ?= 20
REVS ?= 10
LINEAR ?= 0.10
ANGULAR ?= 0.00

.PHONY: help ros-build bridge teleop cmd-echo cmd twist stop ping led-on led-off reset pwm pwm-off cal-start cal-status cal-stop cal-cancel arduino-build arduino-upload monitor

help:
	@printf '%s\n' \
	  'Common commands:' \
	  '  make ros-build        Build the ROS2 workspace' \
	  '  make bridge           Run the ROS2 Arduino bridge' \
	  '  make teleop           Drive with keyboard teleop on /cmd_vel' \
	  '  make cmd-echo         Watch /cmd_vel Twist messages' \
	  '  make twist            Send one /cmd_vel Twist sample' \
	  '  make stop             Send a zero /cmd_vel Twist sample' \
	  '  make cmd CMD=PING     Send any Arduino command through ROS2' \
	  '  make ping             Send PING' \
	  '  make led-on           Turn built-in LED on' \
	  '  make led-off          Turn built-in LED off' \
	  '  make reset            Reset speed counts' \
	  '  make pwm VALUE=20     Set PWM value' \
	  '  make pwm-off          Stop PWM output' \
	  '  make cal-start        Start counts-per-rev calibration' \
	  '  make cal-status       Show calibration counts so far' \
	  '  make cal-stop REVS=10 Stop calibration using known revolutions' \
	  '  make cal-cancel       Cancel calibration mode' \
	  '  make arduino-build    Compile Arduino firmware' \
	  '  make arduino-upload   Upload Arduino firmware to PORT=/dev/ttyACM0' \
	  '  make monitor          Open Arduino serial monitor' \
	  '' \
	  'Overrides:' \
	  '  PORT=/dev/ttyACM1 BAUD=115200 VALUE=30 REVS=5 LINEAR=0.10 ANGULAR=0.00'

ros-build:
	colcon build

bridge:
	source $(ROS_SETUP)
	source install/setup.bash
	ros2 run mower_serial arduino_bridge

teleop:
	source $(ROS_SETUP)
	source install/setup.bash
	ros2 run teleop_twist_keyboard teleop_twist_keyboard

cmd-echo:
	source $(ROS_SETUP)
	source install/setup.bash
	ros2 topic echo /cmd_vel

cmd:
	source $(ROS_SETUP)
	source install/setup.bash
	ros2 topic pub --once /mower/arduino_cmd std_msgs/msg/String "{data: '$(CMD)'}"

twist:
	source $(ROS_SETUP)
	source install/setup.bash
	ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist "{linear: {x: $(LINEAR)}, angular: {z: $(ANGULAR)}}"

stop:
	$(MAKE) twist LINEAR=0.0 ANGULAR=0.0

ping:
	$(MAKE) cmd CMD="PING"

led-on:
	$(MAKE) cmd CMD="LED_ON"

led-off:
	$(MAKE) cmd CMD="LED_OFF"

reset:
	$(MAKE) cmd CMD="RESET_COUNTS"

pwm:
	$(MAKE) cmd CMD="PWM $(VALUE)"

pwm-off:
	$(MAKE) cmd CMD="PWM_OFF"

cal-start:
	$(MAKE) cmd CMD="CAL_START"

cal-status:
	$(MAKE) cmd CMD="CAL_STATUS"

cal-stop:
	$(MAKE) cmd CMD="CAL_STOP $(REVS)"

cal-cancel:
	$(MAKE) cmd CMD="CAL_CANCEL"

arduino-build:
	arduino-cli compile --fqbn $(FQBN) $(ARDUINO_DIR)

arduino-upload:
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) $(ARDUINO_DIR)

monitor:
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)
