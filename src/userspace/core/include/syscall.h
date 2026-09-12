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
