# Serial Communication Library

This is a cross-platform library for interfacing with rs-232 serial-like ports written in C++.

It provides a modern C++ interface with a workflow designed to look and feel like PySerial, but with the speed and control provided by C++.

Optimized version of [wjwwood/serial](https://github.com/wjwwood/serial).

It is recommended to use this repository for serial port programming: [SmartSerial](https://github.com/shuai132/SmartSerial).
Based on this library, it provides more convenient functions and thread-safe support.

## Features

- Optimized cmake file, removed catkin.
- Fixed some compilation warnings and API restrictions
- Optimized the exception inheritance hierarchy. All serial-related.exceptions inherit from SerialException.
