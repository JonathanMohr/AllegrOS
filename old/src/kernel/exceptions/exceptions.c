#include "exceptions.h"

#include "../arch/i686/io.h"
#include "../debug.h"
#include <stdbool.h>

void logRegsCrit(Registers* regs)
{
    log_crit("ISR", "  eax=0x%x  ebx=0x%x  ecx=0x%x  edx=0x%x  esi=0x%x  edi=0x%x",
        regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

    log_crit("ISR", "  esp=0x%x  ebp=0x%x  eip=0x%x  eflags=0x%x  cs=0x%x  ds=0x%x  ss=0x%x",
        regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds, regs->ss);

    log_crit("ISR", "  interrupt=0x%x  errorcode=0x%x", regs->interrupt, regs->error);
}

void logRegs(Registers* regs)
{
    log_verbose("ISR", "  eax=0x%x  ebx=0x%x  ecx=0x%x  edx=0x%x  esi=0x%x  edi=0x%x",
        regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);

    log_verbose("ISR", "  esp=0x%x  ebp=0x%x  eip=0x%x  eflags=0x%x  cs=0x%x  ds=0x%x  ss=0x%x",
        regs->esp, regs->ebp, regs->eip, regs->eflags, regs->cs, regs->ds, regs->ss);

    log_verbose("ISR", "  interrupt=0x%x  errorcode=0x%x", regs->interrupt, regs->error);
}

void divByZero(Registers* regs)
{
    log_err("ISR", "Division by zero!");
    logRegs(regs);

    log_debug("ISR", "Kernel panic!");
    i686_Panic();
}

void debug(Registers* regs)
{
    log_debug("ISR", "Debug interrupt");
}

void nonMask(Registers* regs)
{
    log_crit("ISR", "Non-maskable Interrupt!");
    logRegsCrit(regs);

    uint8_t status = i686_inb(0x61);
    log_crit("ISR", "  status=0x%x", status);

    log_crit("ISR", "Kernel panic!");
    i686_Panic();
}

void breakpoint(Registers* regs)
{
    log_debug("ISR", "Breakpoint");
}

void overflow(Registers* regs)
{
    log_warn("ISR", "Overflow");
    logRegs(regs);
}

void bound(Registers* regs)
{
    log_warn("ISR", "Bound Range Exceeded");
    logRegs(regs);
}

void invalidOpcode(Registers* regs)
{
    log_crit("ISR", "Invalid Opcode!");
    logRegsCrit(regs);

    uint8_t* faulting_instr = (uint8_t*)regs->eip;
    log_crit("ISR", "Faulting instruction: 0x%x", faulting_instr);

    log_crit("ISR", "Kernel panic!");
    i686_Panic();
}

void deviceNotAvailable(Registers* regs)
{
    log_debug("ISR", "Device Not Available (FPU access with TS set)");
    //TODO
}

void doubleFault(Registers* regs)
{
    log_crit("ISR", "Double fault!");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel panic!");
    i686_Panic();
}

void coprocesserSegmentOverrun(Registers* regs)
{
    log_err("ISR", "Coprocessor Segment Overrun Exception");
    logRegs(regs);
}

void invalidTSS(Registers* regs)
{
    log_crit("ISR", "Invalid TSS!");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel panic!");
    i686_Panic();
}

void segmentNotPresent(Registers* regs)
{
    log_err("ISR", "Segment not present!");
    logRegs(regs);

    log_debug("ISR", "Kernel panic!");
    i686_Panic();
}

void stackSegmentFault(Registers* regs)
{
    log_crit("ISR", "Stack-Segment Fault!");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel panic!");
    i686_Panic();
}

void generalProtectionFault(Registers* regs)
{
    log_err("ISR", "General Protection Fault!");
    log_err("ISR", "Error code: 0x%x", regs->error);
    logRegs(regs);

    log_debug("ISR", "Kernel panic!");
    i686_Panic();
}

void pageFault(Registers* regs)
{
    log_err("ISR", "Page Fault!");
    logRegs(regs);

    log_debug("ISR", "Kernel Panic!");
    i686_Panic();
    //TODO
}

void x87FloatingPoint(Registers* regs)
{
    log_err("ISR", "x87 Floating-Point Exception!");
    logRegs(regs);

    log_debug("ISR", "Kernel Panic!");
    i686_Panic();
}

void alignmentCheck(Registers* regs)
{
    log_err("ISR", "Alignment Check Exception!");
    logRegs(regs);

    log_debug("ISR", "Kernel Panic!");
    i686_Panic();
}

void machineCheck(Registers* regs)
{
    log_crit("ISR", "Machine Check Exception!");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel Panic!");
    i686_Panic();
}

void SIMDFloatingPoint(Registers* regs)
{
    log_err("ISR", "SIMD Floating-Point Exception!");
    logRegs(regs);

    log_debug("ISR", "Kernel Panic!");
    i686_Panic();
}

void virtualization(Registers* regs)
{
    log_crit("ISR", "Virtualization Exception!");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel Panic!");
    i686_Panic();
}

void controlProtection(Registers* regs)
{
    log_crit("ISR", "Control Protection Exception!");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel Panic!");
    i686_Panic();
}

void hypervisorInjection(Registers* regs)
{
    log_err("ISR", "Hypervisor Injection Exception");
    logRegs(regs);

    log_debug("ISR", "Kernel Panic!");
    i686_Panic();
}

void VMMCommunication(Registers* regs)
{
    log_err("ISR", "VMM Communication Exception");
    logRegs(regs);

    log_debug("ISR", "Kernel Panic!");
    i686_Panic();
}

void security(Registers* regs)
{
    log_crit("ISR", "Security Exception");
    logRegsCrit(regs);

    log_crit("ISR", "Kernel Panic!");
    i686_Panic();
}


void reserved(Registers* regs)
{
    log_crit("ISR", "Reserved exception 0x%x caused!", regs->interrupt);
    logRegsCrit(regs);

    log_crit("ISR", "Kernel Panic!");
    i686_Panic();
}


void isr_registerExceptionHandlers()
{
    i686_ISR_RegisterHandler(0x00, divByZero);
    i686_ISR_RegisterHandler(0x01, debug);
    i686_ISR_RegisterHandler(0x02, nonMask);
    i686_ISR_RegisterHandler(0x03, breakpoint);
    i686_ISR_RegisterHandler(0x04, overflow);
    i686_ISR_RegisterHandler(0x05, bound);
    i686_ISR_RegisterHandler(0x06, invalidOpcode);
    i686_ISR_RegisterHandler(0x07, deviceNotAvailable);
    i686_ISR_RegisterHandler(0x08, doubleFault);
    i686_ISR_RegisterHandler(0x09, coprocesserSegmentOverrun);
    i686_ISR_RegisterHandler(0x0a, invalidTSS);
    i686_ISR_RegisterHandler(0x0b, segmentNotPresent);
    i686_ISR_RegisterHandler(0x0c, stackSegmentFault);
    i686_ISR_RegisterHandler(0x0d, generalProtectionFault);
    i686_ISR_RegisterHandler(0x0e, pageFault);
    i686_ISR_RegisterHandler(0x0f, reserved);
    i686_ISR_RegisterHandler(0x10, x87FloatingPoint);
    i686_ISR_RegisterHandler(0x11, alignmentCheck);
    i686_ISR_RegisterHandler(0x12, machineCheck);
    i686_ISR_RegisterHandler(0x13, SIMDFloatingPoint);
    i686_ISR_RegisterHandler(0x14, virtualization);
    i686_ISR_RegisterHandler(0x15, controlProtection);
    i686_ISR_RegisterHandler(0x16, reserved);
    i686_ISR_RegisterHandler(0x17, reserved);
    i686_ISR_RegisterHandler(0x18, reserved);
    i686_ISR_RegisterHandler(0x19, reserved);
    i686_ISR_RegisterHandler(0x1a, reserved);
    i686_ISR_RegisterHandler(0x1b, reserved);
    i686_ISR_RegisterHandler(0x1c, hypervisorInjection);
    i686_ISR_RegisterHandler(0x1d, VMMCommunication);
    i686_ISR_RegisterHandler(0x1e, security);
    i686_ISR_RegisterHandler(0x1f, reserved);
}