#pragma once

#include <syscall_nums.h>

extern syscall_t syscall0(syscall_t number);
extern syscall_t syscall1(syscall_t number, syscall_t arg1);
extern syscall_t syscall2(syscall_t number, syscall_t arg1, syscall_t arg2);
extern syscall_t syscall3(syscall_t number, syscall_t arg1, syscall_t arg2, syscall_t arg3);

#define __GET_COUNT(_1, _2, _3, _4, N, ...) N
#define __COUNT_ARGS(...) __GET_COUNT(__VA_ARGS__, 4, 3, 2, 1, dummy)

#define __CONCAT_(a, b) a##b
#define __CONCAT(a, b) __CONCAT_(a, b)

#define __syscall_arg_wrapper1(a) syscall0(a)
#define __syscall_arg_wrapper2(a, b) syscall1(a, b)
#define __syscall_arg_wrapper3(a, b, c) syscall2(a, b, c)
#define __syscall_arg_wrapper4(a, b, c, d) syscall3(a, b, c, d)

#define syscall(...) __CONCAT(__syscall_arg_wrapper, __COUNT_ARGS(__VA_ARGS__))(__VA_ARGS__)


static inline void syscall_exit(syscall_t code) { syscall(SYSCALL_EXIT, code); }

static inline void* syscall_allocate_pages(void* addr, syscall_t len, syscall_t flags) { return (void*)syscall(SYSCALL_ALLOCATE_PAGES, (syscall_t)addr, len, flags); }
static inline syscall_t syscall_free_pages(void* addr, syscall_t len) { return syscall(SYSCALL_FREE_PAGES, (syscall_t)addr, len); }

static inline syscall_t syscall_read_keyboard_event(struct syscall_keyboard_event* eventOut) { return syscall(SYSCALL_READ_KEYBOARD_EVENT, (syscall_t)eventOut); }

static inline syscall_t syscall_open_file(const char* path, int create) { return syscall(SYSCALL_OPEN_FILE, (syscall_t)path, (syscall_t)create); }
static inline syscall_t syscall_close_file(syscall_t handle) { return syscall(SYSCALL_CLOSE_FILE, handle); }

static inline syscall_t syscall_open_dir(const char* path) { return syscall(SYSCALL_OPEN_DIR, (syscall_t)path); }
static inline syscall_t syscall_close_dir(syscall_t handle) { return syscall(SYSCALL_CLOSE_DIR, handle); }

static inline syscall_t syscall_read(syscall_t handle, void* data, syscall_t len) { return syscall(SYSCALL_READ, handle, (syscall_t)data, len); }
static inline syscall_t syscall_write(syscall_t handle, const void* data, syscall_t len) { return syscall(SYSCALL_WRITE, handle, (syscall_t)data, len); }

static inline syscall_t syscall_readdir(syscall_t handle, struct syscall_entry* entryOut) { return syscall(SYSCALL_READDIR, handle, (syscall_t)entryOut); }
static inline syscall_t syscall_makedir(const char* path) { return syscall(SYSCALL_MAKEDIR, (syscall_t)path); }

static inline syscall_t syscall_move(const char* sourcePath, const char* destinationPath) { return syscall(SYSCALL_MOVE, (syscall_t)sourcePath, (syscall_t)destinationPath); }
static inline syscall_t syscall_remove(const char* path) { return syscall(SYSCALL_REMOVE, (syscall_t)path); }
