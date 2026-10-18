#pragma once

#include <stdint.h>

typedef uint64_t Kernel_Object_Type;
typedef uintptr_t Kernel_Object_RCount;

typedef struct Kernel_Object
{
    Kernel_Object_Type type;
    Kernel_Object_RCount referenceCount;
    void (*onClose)(struct Kernel_Object* object);
} Kernel_Object;

Kernel_Object* Create_Object(Kernel_Object_Type type, size_t size, void (*onClose)(struct Kernel_Object* object));
Kernel_Object* Get_Object(Kernel_Object* object);
void Pop_Object(Kernel_Object* object);
