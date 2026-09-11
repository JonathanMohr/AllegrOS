#pragma once

void Kernel_Lock(void);
void Kernel_Unlock(void);

void IRQ_PushDisable(void);
void IRQ_PopDisable(void);
