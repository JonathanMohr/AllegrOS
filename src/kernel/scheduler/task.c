#include "task.h"
#include "../panic/panic.h"

void Task_Returned(void)
{
    PanicMessage("Task spawn function returned\n");
    Panic();
}
