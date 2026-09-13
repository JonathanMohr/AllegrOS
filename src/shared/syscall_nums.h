#pragma once

typedef unsigned int syscall_t;
#define SYSCALL_EXIT 0
#define SYSCALL_WRITE 1
#define SYSCALL_ALLOCATE_PAGES 2
#define SYSCALL_FREE_PAGES 3

typedef unsigned int syscall_memory_flags_t;
#define SYSCALL_MEMORY_READABLE 1
#define SYSCALL_MEMORY_WRITABLE 2
#define SYSCALL_MEMORY_EXECUTABLE 3
