ENTRY(start)
OUTPUT_FORMAT("binary")
phys = 0x7C00;                  /* BIOS puts us at 0x7C00 */

SECTIONS
{
    . = phys;

    /* Entry section */
    .entry              : { __entry_start = .;
                            *(.entry)   
                          }

    /* Code */
    .text               : { *(.text)            }

    /* Modifiable data */
    .data               : { *(.data)            }

    /* Read-only data */
    .rodata             : { *(.rodata)          }

    /* MBR Partition Table - 4 entries à 16 bytes = 64 bytes, at 0x7DBE */
    .mbr_partition_table 0x7DBE : { *(.mbr_partition_table) }

    /* Required for the BIOS to boot our OS */
    .bios_footer 0x7DFE : { SHORT(0xAA55)       }

    /* Uninitialized memory (declared with resb/resw/resq). We can use memory
       beyond this binary for that. 
    */
    .bss                : { *(.bss)             }
}
