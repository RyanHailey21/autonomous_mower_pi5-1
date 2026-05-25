### **System Architecture Summary**

| Subsystem | Component | Specifications |
| --- | --- | --- |
| **Compute** | **Raspberry Pi 5 (16GB)** | 2.4GHz quad-core; 3.3V GPIO; optimized for SLAM and path planning. |
| **Logic Power** | **UCTRONICS U6112 (x2)** | 24V-to-5V (5A); Dual output; provides separate rails for Pi and sensors. |
| **Drivetrain** | **LINIX 250W BLDC (x2)** | 24V/15A; Sensored Hall feedback; high-torque gear reduction. |
| **Drive Control** | **ZS-X11H Drivers** | 16A nominal; requires 0-5V analog via **PWM-to-Voltage bridge**. |
| **Cutting Motor** | **775 DC Motor** | 24V / 10,000 RPM; paired with a **20 cm Manganese Steel blade**. |
| **Cutting Control** | **BTN7960B H-Bridge** | 43A rating; 3.3V PWM logic; supports soft-start and active braking. |
| **Distribution** | **6-Way PBT Fuse Box** | 100A Capacity; M5 studs for 24V rail; M4 screws for Ground Bus. |
| **Navigation** | **YDLIDAR X2L** | 360° 2D LiDAR; 8m range; Serial-to-USB interface. |
| **Odometry** | **AS5600 Encoders (x2)** | 12-bit magnetic; I2C interface via **TCA9548A Multiplexer**. |
| **Orientation** | **ICM-20948 IMU** | 9-axis (Accel/Gyro/Mag); high precision for heading correction. |
| **Global Positioning** | **ATGM336H GPS** | Ceramic antenna; UART output for geofencing. |
| **Energy** | **24V 20Ah LiFePO4** | 480Wh total capacity; Integrated BMS. |
