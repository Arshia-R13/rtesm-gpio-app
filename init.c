// init.c :

#define CONFIGURE_INIT
//#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_SIMPLE_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_MAXIMUM_TASKS 4
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_STACK_SIZE (RTEMS_MINIMUM_STACK_SIZE * 2)
#define CONFIGURE_INIT_TASK_PRIORITY 1

#include <rtems.h>

// Vorwärtsdeklaration – wichtig, damit der Init-Task korrekt eingebunden wird:
rtems_task Init(rtems_task_argument argument);

#include <rtems/confdefs.h>




