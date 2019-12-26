/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */

/* TODO Add any manufacture supplied header files can be included
here. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "nuclei_hal.h"

#define mainQUEUE_RECEIVE_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define mainQUEUE_SEND_TASK_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainEVENT_SEMAPHORE_TASK_PRIORITY (configMAX_PRIORITIES - 1)

#define NBIT_DEFAULT 3
#define REG_LEN 8

/* The period of the example software timer, specified in milliseconds, and
converted to ticks using the pdMS_TO_TICKS() macro. */
#define mainSOFTWARE_TIMER_PERIOD_MS pdMS_TO_TICKS(1000)

#define mainQUEUE_LENGTH (1)
static void prvSetupHardware(void);
extern void idle_task(void);
static void vExampleTimerCallback(TimerHandle_t xTimer);

/* The queue used by the queue send and queue receive tasks. */
static QueueHandle_t xQueue = NULL;

void wait_seconds(uint32_t seconds)
{
    uint64_t start_mtime, delta_mtime;
    uint64_t wait_ticks = ((uint64_t)seconds) * SOC_TIMER_FREQ;

    start_mtime = SysTimer_GetLoadValue();

    do {
        delta_mtime = SysTimer_GetLoadValue() - start_mtime;
    } while (delta_mtime < wait_ticks);

    printf("-----------------Waited %d seconds.\n", seconds);
}

// Vector interrupt
__INTERRUPT void SOC_BUTTON_1_HANDLER(void)
{
    // save mepc,mcause,msubm enable interrupts
    SAVE_IRQ_CSR_CONTEXT();

    printf("%s", "--------Higher level\n");
    printf("%s", "----Begin button1 handler----Vector mode\n");

    // Green LED toggle
    gpio_toggle(GPIO, SOC_LED_GREEN_GPIO_MASK);

    // Clear the GPIO Pending interrupt by writing 1.
    gpio_clear_interrupt(GPIO, SOC_BUTTON_1_GPIO_OFS, GPIO_INT_RISE);

    wait_seconds(1); // Wait for a while

    printf("%s", "----End button1 handler\n");

    // disable interrupts,restore mepc,mcause,msubm
    RESTORE_IRQ_CSR_CONTEXT();
}

void SOC_BUTTON_2_HANDLER(void)
{
    printf("%s", "--------Begin button2 handler----NonVector mode\n");

    // Blue LED toggle
    gpio_toggle(GPIO, SOC_LED_BLUE_GPIO_MASK);

    // Clear pending interrupt of Button 2
    gpio_clear_interrupt(GPIO, SOC_BUTTON_2_GPIO_OFS, GPIO_INT_RISE);

    wait_seconds(1); // Wait for a while

    printf("%s", "--------End button2 handler\n");
}

void board_gpio_init(void)
{
    gpio_enable_input(GPIO, SOC_BUTTON_GPIO_MASK);
    gpio_set_pue(GPIO, SOC_BUTTON_GPIO_MASK, GPIO_BIT_ALL_ONE);

    gpio_enable_output(GPIO, SOC_LED_GPIO_MASK);
    gpio_write(GPIO, SOC_LED_GPIO_MASK, GPIO_BIT_ALL_ZERO);

    // Initialize the button as rising interrupt enabled
    gpio_enable_interrupt(GPIO, SOC_BUTTON_GPIO_MASK, GPIO_INT_RISE);    
}

void prvSetupHardware(void)
{
    board_gpio_init();
}

int IRQ_register(void)
{
    int ret;
    printf("register button1 interrupt as vector mode, rising edge and level "
           "3\n\r");
    ret = ECLIC_Register_IRQ(SOC_INT49_IRQn, ECLIC_VECTOR_INTERRUPT, 2, 3, 0,
                              SOC_BUTTON_1_HANDLER);
    if (ret < 0)
        return ret;
    printf("register button2 interrupt as non_vector mode, rising edge and "
           "level 2\n\r");
    ret = ECLIC_Register_IRQ(SOC_INT50_IRQn, ECLIC_NON_VECTOR_INTERRUPT, 2, 2,
                              0, SOC_BUTTON_2_HANDLER);
    return ret;
}

TaskHandle_t StartTask_Handler;
TaskHandle_t StartTask2_Handler;

void start_task(void *pvParameters);
void start_task2(void *pvParameters);

int main(void)
{	
    TimerHandle_t xExampleSoftwareTimer = NULL;

    /* Configure the system ready to run the demo.  The clock configuration
    can be done here if it was not done before main() was called. */
    prvSetupHardware();

    /* register interrupt handler for button1 and button2 interrupt */
    IRQ_register();

    xQueue = xQueueCreate(/* The number of items the queue can hold. */
                          mainQUEUE_LENGTH,
                          /* The size of each item the queue holds. */
                          sizeof(uint32_t));

    if (xQueue == NULL) {
        for (;;) ;
    }
    xTaskCreate((TaskFunction_t)start_task, (const char *)"start_task",
                (uint16_t)256, (void *)NULL, (UBaseType_t)2,
                (TaskHandle_t *)&StartTask_Handler);

    xTaskCreate((TaskFunction_t)start_task2, (const char *)"start_task2",
                (uint16_t)256, (void *)NULL, (UBaseType_t)1,
                (TaskHandle_t *)&StartTask2_Handler);

    xExampleSoftwareTimer =
        xTimerCreate((const char *)"LEDTimer", mainSOFTWARE_TIMER_PERIOD_MS,
                     pdTRUE, (void *)0, vExampleTimerCallback);

    xTimerStart(xExampleSoftwareTimer, 0);
    printf("Before StartScheduler\r\n"); // Bob: I added it to here to easy
                                         // analysis

    vTaskStartScheduler();

    printf("post   ok \r\n");

    for (;;) {
        ;
    };
}

void start_task(void *pvParameters)
{
    TickType_t xNextWakeTime;
    int x;
    printf("task_1\r\n");
    while (1) {
        printf("task1_running..... \r\n");

        vTaskDelay(200);
    }
}

void start_task2(void *pvParameters)
{
    uint32_t ulReceivedValue;
    printf("task_2\r\n");
    /* Initialise xNextWakeTime - this only needs to be done once. */

    while (1) {
        printf("task2_running..... \r\n");

        vTaskDelay(200);
    }
}

static void vExampleTimerCallback(TimerHandle_t xTimer)
{
    /* The timer has expired.  Count the number of times this happens.  The
    timer that calls this function is an auto re-load timer, so it will
    execute periodically. */

    gpio_toggle(GPIO, SOC_LED_RED_GPIO_MASK);
    printf("timers Callback\r\n");
}

void vApplicationTickHook(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint32_t ulCount = 0;

    /* The RTOS tick hook function is enabled by setting configUSE_TICK_HOOK to
    1 in FreeRTOSConfig.h.

    "Give" the semaphore on every 500th tick interrupt. */


    /* If xHigherPriorityTaskWoken is pdTRUE then a context switch should
    normally be performed before leaving the interrupt (because during the
    execution of the interrupt a task of equal or higher priority than the
    running task was unblocked).  The syntax required to context switch from
    an interrupt is port dependent, so check the documentation of the port you
    are using.

    In this case, the function is running in the context of the tick interrupt,
    which will automatically check for the higher priority task to run anyway,
    so no further action is required. */
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    /* The malloc failed hook is enabled by setting
    configUSE_MALLOC_FAILED_HOOK to 1 in FreeRTOSConfig.h.

    Called if a call to pvPortMalloc() fails because there is insufficient
    free memory available in the FreeRTOS heap.  pvPortMalloc() is called
    internally by FreeRTOS API functions that create tasks, queues, software
    timers, and semaphores.  The size of the FreeRTOS heap is set by the
    configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
    printf("malloc failed\n");
    for (;;)
        ;
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName)
{
    //  ( void ) pcTaskName;
    // ( void ) xTask;

    /* Run time stack overflow checking is performed if
    configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
    function is called if a stack overflow is detected.  pxCurrentTCB can be
    inspected in the debugger if the task name passed into this function is
    corrupt. */
    printf("Stack Overflow\n");
    for (;;)
        ;
}
/*-----------------------------------------------------------*/

extern UBaseType_t uxCriticalNesting;
void vApplicationIdleHook(void)
{
    volatile size_t xFreeStackSpace;
    /* The idle task hook is enabled by setting configUSE_IDLE_HOOK to 1 in
    FreeRTOSConfig.h.

    This function is called on each cycle of the idle task.  In this case it
    does nothing useful, other than report the amount of FreeRTOS heap that
    remains unallocated. */
    /* By now, the kernel has allocated everything it is going to, so
    if there is a lot of heap remaining unallocated then
    the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
    reduced accordingly. */
}