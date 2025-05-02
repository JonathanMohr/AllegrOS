#ifndef REBOOT_H
#define REBOOT_H

void reboot(void);

void __attribute__((cdecl)) triple_fault(void);

#endif