# Serial Communication Library

Optimization of wjwwood/serial

We recommend using this repository for serial port programming: [SmartSerial](https://github.com/shuai132/SmartSerial).
Based on this library, it provides more convenient features and thread-safe support.

## Features

- Optimized CMake file to remove catkin dependency.
- Fixed some compilation warnings and API restrictions.
- Improved exception inheritance hierarchy, with all serial-related exceptions inheriting from SerialException.
