#!/usr/bin/env bash

# @file flash.sh
# @brief Script to flash an STM board on Windows host
#
# Copyright (c) 2025 Miikka Lukumies


echo "Flashing the board..."
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 PATH_TO_BINARY"
    exit 1
fi
PATH="/C/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/":$PATH
stm32programmercli="STM32_Programmer_CLI"
connect="-c port=SWD mode=UR"

$stm32programmercli $connect -d $1 -v
