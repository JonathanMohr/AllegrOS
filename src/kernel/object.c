#include "object.h"

#include "memory/memory.h"
#include "panic/panic.h"

Kernel_Object* Create_Object(Kernel_Object_Type type, size_t size, void (*onClose)(struct Kernel_Object* object))
{
    Kernel_Object* object = Memory_KernelZallocate(size);
    if (!object) return NULL;

    object->type = type;
    object->referenceCount = 1;
    object->onClose = onClose;

    return object;
}

Kernel_Object* Get_Object(Kernel_Object* object)
{
    if (object->referenceCount + 1 <= object->referenceCount)
    {
        PanicMessageInfo("Get_Object", "Maximal object reference count for object at %p reached\n", object);
        return NULL;
    }

    object->referenceCount++;
    return object;
}

void Pop_Object(Kernel_Object* object)
{
    if (object->referenceCount-- == 0)
    {
        PanicMessageInfo("Pop_Object", "Object reference count for object at %p was already zero\n", object);
        object->referenceCount = 0;
    }

    if (object->referenceCount == 0)
    {
        if (object->onClose)
            object->onClose(object);
        Memory_KernelFree(object);
    }
}
