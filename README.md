# Custom Bootloader Implmentation on STM32L053R8

This project are the part of self-learning in Embedded System as a 4th-Year Computer Engineering Student <br>
Note! This project still be in progress 
---

## Goal
Be able to upload a firmware from application on a computer via USB-to-UART (ST-LINK) to STM32L053R8 with custom bootloader and packet protocol

## Future plan
Implement a firmware encryption, firmware integrity and firmware upload capability via USB

## Toolchain
1. libopencm3
2. arm-none-eabi-gcc 
3. make
4. stlink
5. openocd
6. cortex-debug

## Hardware Memory Map
![STM32L053R8_Overview_Hardware_Memory_Map](pictures/STM32L053R8_Overview_Hardware_Memory_Map.png)

## Learning Resource & Reference
1. STM32L053R Datasheet
2. [YouTube: Low Byte Productions (Blinky To Bootloader: Bare Metal Programming Series)](https://youtube.com/playlist?list=PLP29wDx6QmW7HaCrRydOnxcy8QmW0SNdQ&si=wKLBIT67plQATxr1)
3. [Website: DEEPBLUE MBEDDED](https://deepbluembedded.com/stm32-hal-library-tutorial-examples/)
4. [YouTube: Embedded Systems and Deep Learning](https://youtube.com/playlist?list=PLRJhV4hUhIymmp5CCeIFPyxbknsdcXCc8&si=Y3LhIALU49Qpa-L7)