/*
 * @file link_cortex_m33.ld
 * @brief Linker code for Cortex-M33 processor
 *
 * Copyright (c) 2025 Miikka Lukumies
 */

/* ================ DEFINITIONS ================ */
__ROM_BASE_NS   = 0x08000000;       /* Non-Secure Flash start address */
__ROM_BASE_S    = 0x0C000000;       /* Secure Flash start address */
__ROM_SIZE      = 512K;             /* Flash size total */
/* TODO: separate sizes for secure, non-secure flash */

__RAM_BASE      = 0x20000000;       /* RAM start address */
__RAM_SIZE      = 256K;             /* RAM size total */
/* TODO: separate RAM for secure, non-secure */

__STACK_SIZE    = 1K;               /* Stack size */
__HEAP_SIZE     = 2K;               /* Heap size */
/* TODO: separate stack, heap for secure and non-secure */


/* ================ MEMORY REGIONS ================ */
MEMORY
{
   S_FLASH   (rx) : ORIGIN = __ROM_BASE_S,  LENGTH = __ROM_SIZE     /* Secure Flash */
   NS_FLASH  (rx) : ORIGIN = __ROM_BASE_NS, LENGTH = __ROM_SIZE     /* Non-Secure Flash */
   RAM      (rwx) : ORIGIN = __RAM_BASE,    LENGTH = __RAM_SIZE     /* RAM */
}

__MAX_NUM_TASKS = 5;
__TASK_STACK_SIZE = 0x400;

/* ================ SECTIONS ================ */
SECTIONS
{

    /* Code, and read-only data go into .text section,
     * stored at the beginning of FLASH.
    */
    .text : 
    {
        KEEP(*(.isr_vector))                    /* Vector table must be the first element */
        *(.text*)                               /* Main program code */
    	*(.rodata*)                             /* Const data */
    } > S_FLASH                                 /* Store into FLASH */


    /* Instructions for copying .data from FLASH to RAM */
    .copy.table :
    {
        . = ALIGN(4);                           /* Ensure alignment so that following constants can
                                                    be accessed as a word */
        __copy_tbl_start = .;               /* Constant for locating this section */
        LONG(__data_lma_start)                  /* .data section start address in FLASH (src) */
        LONG(__data_start)                      /* Start address of .data in RAM (dest) */
        LONG((__data_end - __data_start) / 4)   /* No. of words in .data section */
    } > S_FLASH


    /* Instructions for zeroing .bss section in RAM */
    .zero.table :
    {
        . = ALIGN(4);                           /* Align to word boundary */
        __zero_tbl_start = .;               /* Constant for locating this section */
        LONG(__bss_start)                       /* .bss section start address in RAM (dest) */
        LONG((__bss_end - __bss_start) / 4)     /* No. of words in .bss sections */
    } > S_FLASH


    /* Word-align an address on FLASH to store .data contents into */
    __data_lma_start = ALIGN(4);

    /* Data section; initialized static data. Stored in FLASH, copied to RAM on startup */
    .data : AT(__data_lma_start)                /* LMA is set to word-aligned address in FLASH */
    {
        . = ALIGN(4);                           /* Align VMA (run-time) address to word boundary */
        __data_start = .;                       /* Data section VMA address start */
        *(.data*)                               /* Contents of all .data sections */
        . = ALIGN(4);                           /* Align the following symbol's address to round up size to 
                                                    next 4 byte multiple */
        __data_end = .;                         /* Data section VMA address end */
    } >RAM                                      /* VMA is set to RAM */


    /* BSS section, uninitialized static data. Space reserved in RAM, zeroed on startup */
    .bss :
    {
        . = ALIGN(4);                           /* Align VMA address to word boundary */
        __bss_start = .;                        /* BSS section VMA address start */
        *(.bss*)                                /* Contents of all .bss sections */
        *(COMMON*)                              /* Contents of all COMMON sections */ 
        . = ALIGN(4);                           /* Round up following label's address to next word boundary */
        __bss_end = .;                          /* BSS section CMA address end */
    } > RAM                                     /* VMA is set to RAM */


    /* Heap section for dynamically allocated data */
    .heap :
    {
        . = ALIGN(8);                           /* Align to eight bytes for efficient access to
                                                    double-word allocations */
        __HeapStart = .;                        /* Marker for heap start */
        . = . + __HEAP_SIZE;                    /* Reserve space based on constant size definition */
        . = ALIGN(8);                           /* Align following label to eight bytes */
        __HeapLimit = .;                        /* Marker for heap end */
    } > RAM

    /* Stack section for task stacks */
    .task_stack (ORIGIN(RAM) + LENGTH(RAM) - __STACK_SIZE - (__TASK_STACK_SIZE * __MAX_NUM_TASKS)) (COPY) :
    {
        . = ALIGN(8);                           /* Align following label to eight byte boundary */
        __TaskStackLimit = .;                   /* Stack limit, i.e. last address of the stack */
        *(*.task_stacks)                        /* Task stacks go here */
        . = ALIGN(8);                           /* Align following label to eight byte boundary */
        __TaskStackTop = .;                     /* Stack top, i.e. first address of the stack */
    }

    /* Main stack section for all temporary data. Stack grows down - place at the end of RAM */
    .stack (ORIGIN(RAM) + LENGTH(RAM) - __STACK_SIZE) (COPY) :
    {
        . = ALIGN(8);                           /* Align following label to eight byte boundary */
        __StackLimit = .;                       /* Stack limit, i.e. last address of the stack */
        . = . + __STACK_SIZE;                   /* Reserve space based on constant size definition */
        . = ALIGN(8);                           /* Align following label to eight byte boundary */
        __StackTop = .;                         /* Stack top, i.e. first address of the stack. This will
                                                    be the first entry in the vector table */
    }

    /* Stack is loaded into a constant address based on sizes only, make sure it doesn't overflow to .heap */
    ASSERT(__StackLimit >= __HeapLimit, "region .stack overflowed with .heap")
    ASSERT(__StackLimit >= __TaskStackTop, "region .task_stack overflowed with .stack")
}


