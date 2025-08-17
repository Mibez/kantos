### KANTOS ###

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

## Usage ##

To use the operating system, modify the `os/app.c`, or write your own. You must:
1. Implement an entry point for the application, `void main(void)`. This entry point must call `scheduler_start()` to kick off the scheduler/os
2. Implement at least one task to be run by the OS, with signature `void task(void* arg1, void* arg2, void* arg3)`. The task shall not return ever
3. Register the task(s) with the OS, using the `OS_TASKS_INIT` macro, for example
```
OS_TASKS_INIT(
    OS_TASK_DEFINE(taskA, 0, 0, 0, OS_LOWEST_PRIO + 1),
    OS_TASK_DEFINE(taskB, 0, 0, 0, OS_LOWEST_PRIO + 1),
);
```
The tasks should be listed in priority order, with the highest priority task first

## How it works ##

The KantOS is a pre-emptive co-operative kernel. This means that a running task can be pre-empted by another task, if it is higher priority, and that a task can voluntarily yield control to other tasks.

Determining, which task should run at any given time is done based on the tasks' state, and their priority. A task can be, at any point, in one of three states (plus a few internal states utilized by the OS); READY, PENDING, or RUNNING
* READY - a task being ready means, that it is not currently executing, but it's ready for doing so - it is not pending a timer or other event
* PENDING - a task is not ready to run - it's waiting for some trigger, such as a timer event
* RUNNING - a task is currently being run

All tasks are set to READY state on startup. The first task defined in `OS_TASKS_INIT` will get selected as the first task to run, and marked as RUNNING. From there on, the normal scheduling takes place. A RUNNING task may become READY, or PENDING, by calling `yield` or `sleep` respectively, or if pre-empted by another higher priority task becoming READY. A PENDING task will move to READY, once the condition that it is waiting on, a timer or other event, has happened. A READY task is selected as RUNNING task once all other higher priority READY tasks have become PENDING.

The KantOS scheduler relies on two built-in interrupts for it's function; the PendSV interrupt, and the SysTick interrupt. The PendSV interrupt handler is responsible for performing the context switch - it stores the context of the currently RUNNING task, and restores the context of the next READY task. The SysTick interrupt handler updates the system tick count, and readies any tasks waiting for a specific tick count, when it has been reached.