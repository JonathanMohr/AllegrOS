ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
phys = 0x7C00;                  /* BIOS puts us at 0x7C00 */

SECTIONS
{
    . = phys;

    .entry              : { __entry_start = .;      *(.entry)   }
    .text               : { __text_start = .;       *(.text)    }
    .data               : { __data_start = .;       *(.data)    }
    .rodata             : { __rodata_start = .;     *(.rodata)  }

    .mbr_partition_table 0x7DBE : { *(.mbr_partition_table) }
    .bios_footer 0x7DFE         : { SHORT(0xAA55)           }

    /* Uninitialized memory (declared with resb/resw/resq). We can use memory
       beyond this binary for that. 
    */
    .bss                : { __bss_start = .;        *(.bss)     }
}
