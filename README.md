### KANTO OS ###

My very first OS, primarily for learning purposes. Targeting at least initially only ARM Cortex-M33 processors, since it supports TrustZone/Secure Boot, and it's all I have (specifically, NUCLEO-U545RE-Q)

All instructions are written for Ubuntu in WSL, Windows 10 host, Git bash as shell (I'm sorry). If you're using native
Linux, you will know how to adapt the scripts and instructions

## Design Goals ##
* Minimal footprint, no external dependencies (HAL usage optional)
* Everything is built from scratch
* Very heavily commented code
* Zephyr-like Kconfig system, but actually usable
* Build system focuses on readability
* Built for ARM Cortex-M33 (targeting STM32)
* Some experimentation on visionary locking tactics (?)
* Targeting to facilitate Secure Boot someday


## Building ##

Download and extract your favorite cross-compile toolchain from https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads

(tested on ```arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi```)

```
wget https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
mkdir -p toolchain
cp arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz ./toolchain/
cd toolchain
tar -xvf arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz
```

when toolchain is extracted, run make in project base

## Running ##

1. Download STM32CubeProgrammer and STM32CubeCLT from st.com
2. Run `scripts/flash.sh`

## Debugging ##
1. See "Running"
2. Run GDB server (from CubeCLT) on host `./ST-LINK_gdbserver.exe -cp "/C/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/" -k`
3. Run GDB client (from toolchain) `toolchain/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gdb -ex "target remote $(hostname).local:61234" --se=build/kernel.elf`

