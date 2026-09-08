#pragma once

#include <stdint.h>

uintptr_t Arch_TemporaryMap(uphysptr_t physicalMapAddress);
void Arch_TemporaryUnmap(uintptr_t virtualMapAddress);
