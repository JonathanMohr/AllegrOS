#pragma once

#include "../arch/i686/paging.h"
#include <stdbool.h>

PageDirectory getPageDirectory();
bool Paging_Map(uintptr_t virtual, uintptr_t physical);