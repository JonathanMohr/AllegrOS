ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
addr = 0xC0000000;

SECTIONS
{
    . = addr;

    .stack              : {                         *(.stack)   }
    .text               : { __text_start = .;       *(.text)    }
    .data               : { __data_start = .;       *(.data)    }
    .rodata             : { __rodata_start = .;     *(.rodata)  }
    .bss                : { __bss_start = .;        *(.bss)     }
    
    __end = .;
}