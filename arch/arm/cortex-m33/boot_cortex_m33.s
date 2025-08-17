/*
 * @file boot_cortex_m33.s
 * @brief Startup code for Cortex-M33 processor
 *
 * INTERRUPT DEFINITIONS:
 *     Copyright (c) 2009-2021 Arm Limited. All rights reserved.
 *
 *     SPDX-License-Identifier: Apache-2.0
 *
 *     Licensed under the Apache License, Version 2.0 (the License); you may
 *     not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an AS IS BASIS, WITHOUT
 *     WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 * EVERYTHING ELSE:
 *      Copyright (c) 2025 Miikka Lukumies
 */

    .syntax unified                 /* Use unified syntax (not divided) */
    .cpu    cortex-m33              /* Specify the target processor, will select armv8-m.main .arch */

/* ============== INTERRUPT VECTORS ==============  */

    .align 4                        /* Align to word boundary */
    .section .isr_vector            /* Declare section .isr_vector. This allows specific placement */

    /* During bootup, the processor will load its initial Stack Pointer (SP) from the first
        entry in the vector table, and the initial Program Counter (PC) value from the second
        entry in the vector table. The least significant bit (LSB) of the second entry (PC, program
        start address) MUST be 1, since the Cortex-M33 processor only supports the Thumb
        instruction set. A Thumb function/branch address is indicated with LSB of 1 */

    .long    __StackTop             /* Initial Top of Stack */
    .long    Reset_Handler          /* Reset Handler */
    .long    NMI_Handler            /* -14 NMI Handler */
    .long    HardFault_Handler      /* -13 Hard Fault Handler */
    .long    MemManage_Handler      /* -12 MPU Fault Handler */
    .long    BusFault_Handler       /* -11 Bus Fault Handler */
    .long    UsageFault_Handler     /* -10 Usage Fault Handler */
    .long    SecureFault_Handler    /*  -9 Secure Fault Handler */
    .long    0                      /*     Reserved */
    .long    0                      /*     Reserved */
    .long    0                      /*     Reserved */
    .long    SVC_Handler            /*  -5 SVCall Handler */
    .long    DebugMon_Handler       /*  -4 Debug Monitor Handler */
    .long    0                      /*     Reserved */
    .long    PendSV_Handler         /*  -2 PendSV Handler */
    .long    SysTick_Handler        /*  -1 SysTick Handler */

    /* Rest of the interrupts go here. Refer to the RM0456 Chapter 22.3 for 
        full set of maskable interrupts. U5 series processors support 140 + 16 interrupts */

    .space      (140 * 4)           /* Allocate space for the rest of the interrupts */


/* ============== TEXT SECTION ==============  */
    .Thumb                          /* Specify that the Thumb instruction set shall be used */
    .section .text                  /* Declare section .text for holding all code */
    .align 4                        /* Strictly not needed, but re-align to word boundary */

/* Reset Handler - the very first piece of code that runs after the CPU is reset */

    .thumb_func                     /* Mark label as Thumb function to ensure LSB of address is set to 1 */

Reset_Handler:
    ldr     r0, =__StackTop         /* Load Stack top to register r0 */
    msr     psp, r0                 /* Write r0 = __StackTop to PSP (Process Stack Pointer) */
    msr     msp, r0                 /* Write r0 = __StackTop to MSP (Main Stack Pointer) (?)*/

    ldr     r0, = __TaskStackLimit  /* Load Stack Limit to register r0 */
    msr     msplim, r0              /* Write r0 = __StackLimit to MSPLIM (Main Stack Pointer LIMit) */
    msr     psplim, r0              /* Write r0 = __StackLimit to PSPLIM (Process Stack Pointer LIMit) */


/* Preparations for running C code - copy .data section from FLASH to RAM. 
    Loads source and target addresses, and number of words to copy
    from constants defined by linker in section .copy.table */
copy_data_to_ram_begin:
    ldr     r0, =__copy_tbl_start   /* Load start address of instructions in .copy.table to r0 */

    ldr     r1, [r0, #0]            /* Load value from address in r0 (.data source addr. in FLASH) into r1 */
    ldr     r2, [r0, #4]            /* Load .data destination addr. in RAM from mem. location r0 + 4 into r2 */
    ldr     r3, [r0, #8]            /* Load number of words to copy from mem. location r0 + 8 into r3 */

    lsls    r3, r3, #2              /* Left-shift words by 2 places (i.e. multiply by 4) to get no. of bytes
                                        to copy. Postfix instruction with 's' to set CPU flags */

    beq     zero_bss_begin          /* Skip the following section if "zero" (Z) flag was set, indicating r3 is 0 */

copy_data_to_ram_loop:              
    subs    r3, #4                  /* Subtract 4 from byte count to get the next index, starting from last index (think of
                                        array[size - 1]). Set flags to detect when the result is negative (N => 1), in which case
                                        the copying should be stopped */
    ittt    ge                      /* Mark the next three instructions as a conditional block. Block will be run if N = 1 */
    ldrge   r4, [r1, r3]            /* Load nth item (start address + r3) of .data from FLASH into r4 */
    strge   r4, [r2, r3]            /* Store nth item of .data from r4 to RAM */
    bge     copy_data_to_ram_loop   /* Jump to beginning of the section to loop over all data*/


/* Preparations for running C code - Set contents of .bss section in RAM to all zeroes */
zero_bss_begin:
    ldr     r0, =__zero_tbl_start   /* Load start address of instructions in .zero.table to r0 */
    ldr     r1, [r0, #0]            /* Load value from address in r0 (.bss address in RAM) into r1 */
    ldr     r2, [r0, #4]            /* Load number of words to zero from mem. location r0 + 4 into r2 */
    lsls    r2, r2, #2              /* Multiply by four, setting flags, to get byte count and detect when size = 0 */
    beq     jump_to_main            /* Skip the next section if Z = 1 (.bss size was zero - nothing to do) */

    movs    r4, #0                  /* Load constant '0' to r4 to be used when zeroing */

zero_bss_loop:
    subs    r2, #4                  /* Subtract 4 from byte count to get memory index, starting from last index, set flags */
    itt     ge                      /* Conditional block of 2, ran if previous operation didn't result in a negative value */
    strge   r4, [r1, r2]            /* Store contents of r4 (a zero) into memory address r1 + r2 (.bss start + r2) */
    bge     zero_bss_loop           /* Jump to beginning of the section to loop over all of .bss */

/* Perform the jump to C code, targeting label "main". This is the application entry point */
jump_to_main:

    ldr     r0, = main              /* Load address of "main" into r0 */
    bx      r0                      /* Jump to address of "main" unconditionally */




/* HardFault interrupt handler - typically run on irrecoverable error, or error during some other exception */
    .align 4                        /* Ensure alignment */
    .thumb_func                     /* Mark as Thumb function to ensure LSB set to 1  */
    .weak   HardFault_Handler       /* Mark as .weak label, allowing for overrides elsewhere */

HardFault_Handler:
    bkpt    #0xAB                   /* Breakpoint/SVC call instruction to have better visibility in
                                        a debugger instead of a loop forever */
    b        .                      /* Loop forever if invasive Debug is not enabled */



/* The default handler - used when no action for an interrupt is specified. */
    .align 4                        /* Ensure alignment */
    .thumb_func                     /* Mark as Thumb function */
    .weak    Default_Handler        /* Allow overriding */

Default_Handler:
    b        .                      /* Loop forever */


    .align 4                        /* This alignment is not needed in practice, but it makes the
                                        literal pool placed (empirically) at the end of this section
                                        aligned to word boundary and thus nicer to read in the disassembly */


/* Macro to define default exception/interrupt handlers.
 * Default handler are weak symbols with an endless loop.
 * They can be overwritten by real handlers.
 */
    .macro   Set_Default_Handler  Handler_Name
    .weak    \Handler_Name
    .set     \Handler_Name, Default_Handler
    .endm


/* Default exception/interrupt handler */

    Set_Default_Handler  NMI_Handler
    Set_Default_Handler  MemManage_Handler
    Set_Default_Handler  BusFault_Handler
    Set_Default_Handler  UsageFault_Handler
    Set_Default_Handler  SecureFault_Handler
    Set_Default_Handler  SVC_Handler
    Set_Default_Handler  DebugMon_Handler
    Set_Default_Handler  PendSV_Handler
    Set_Default_Handler  SysTick_Handler
