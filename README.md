# Real-Time Battery Management System Using FreeRTOS

## Overview
This project focuses on implementing a Battery Management System (BMS) using the STM32F103C6T6 microcontroller and FreeRTOS. The main idea is to explore how a real-time operating system can be used to organize battery management operations into independent tasks, making firmware more modular and easier to manage.

The system uses Embedded C for application development and FreeRTOS for task scheduling and real-time execution. Battery-related data and system status are managed through a structured firmware architecture, with an emphasis on coordinating operations efficiently within a resource-constrained microcontroller.

## Implementation
The firmware follows an RTOS-based approach in which different operations can be organized into separate tasks. This provides practical experience with task creation, scheduling, inter-task communication, and synchronization in embedded systems. The project also involves configuring the STM32 microcontroller and its peripherals using STM32CubeMX and STM32CubeIDE.

## Technologies Used
- **Microcontroller:** STM32F103C6T6
- **Programming Language:** Embedded C
- **Real-Time Operating System:** FreeRTOS
- **IDE:** STM32CubeIDE
- **Configuration Tool:** STM32CubeMX

## Objective
The project demonstrates the application of real-time operating system concepts to embedded battery management, with a focus on modular firmware design, predictable task execution, and efficient resource utilization.
