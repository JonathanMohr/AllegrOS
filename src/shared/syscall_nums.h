#pragma once

typedef unsigned int syscall_t;
#define SYSCALL_EXIT 0

#define SYSCALL_ALLOCATE_PAGES 2
#define SYSCALL_FREE_PAGES 3
#define SYSCALL_READ_KEYBOARD_EVENT 4
#define SYSCALL_OPEN_FILE 5
#define SYSCALL_CLOSE_FILE 6
#define SYSCALL_OPEN_DIR 7
#define SYSCALL_CLOSE_DIR 8
#define SYSCALL_READ 9
#define SYSCALL_WRITE 10
#define SYSCALL_READDIR 11
#define SYSCALL_MAKEDIR 12
#define SYSCALL_MOVE 13
#define SYSCALL_REMOVE 14


typedef unsigned int syscall_memory_flags_t;
#define SYSCALL_MEMORY_READABLE 1
#define SYSCALL_MEMORY_WRITABLE 2
#define SYSCALL_MEMORY_EXECUTABLE 3

struct syscall_keyboard_event
{
    unsigned char keycode;
    unsigned char released; // zero if pressed, non-zero if released
};


#define SYSCALL_ENTRY_FILE      0
#define SYSCALL_ENTRY_DIRECTORY 1

#define SYSCALL_ENTRY_READONLY   0x01
#define SYSCALL_ENTRY_EXECUTABLE 0x02
#define SYSCALL_ENTRY_HIDDEN     0x04
#define SYSCALL_ENTRY_SYSTEM     0x08

struct syscall_entry
{
    char name[1024];
    unsigned long size;
    unsigned char type;
    unsigned char attributes;
};
