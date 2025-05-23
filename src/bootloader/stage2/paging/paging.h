#pragma once

#include <boot/arch/i686/paging.h>
#include <boot/bootparams.h>

PageDirectory* i686_paging_Initialize(uint64_t bootLength,
                                      void* ptr,
                                      uintptr_t kernelVirt,
                                      uint32_t kernelPages,
                                      uint8_t* kernelBegin);
void i686_enable_paging();

uint32_t i686_prepare_paging(MemoryInfo* memInfo,
                             uint32_t bootSize,
                             uint32_t kernelSize,
                             uint8_t** pageDirectoryPtrOut,
                             uint32_t* kernelPagesOut,
                             uint8_t** kernelBeginOut);