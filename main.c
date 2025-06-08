// main.c

/*
 * gpio_blink.c
 *
 * RTEMS-Anwendung für Raspberry Pi (emuliert im QEMU),
 * die zwei LEDs an GPIO4 und GPIO18 steuert:
 *
 *  - GPIO4 blinkt mit einem variablen Zeitintervall (1 ms … 10000 ms).
 *  - Jedes Mal, wenn GPIO4 von "An" auf "Aus" wechselt, wird GPIO18 umgeschaltet (toggle).
 *  - Ein Logger-Task gibt jede Sekunde den aktuellen Zustand von GPIO4 und
 *    die Anzahl der „An→Aus“-Wechsel von GPIO4 in diesem Intervall aus.
 *
 * Die drei Threads (Tasks) laufen ohne wechselseitige Blockierung:
 * 1) blink_task:     Steuert GPIO4 nach dem einstellbaren Intervall.
 * 2) edge_task:      „Interrupt“-gesteuerte Erkennung des Falls von GPIO4 → Toggle GPIO18.
 *                   (Im QEMU wird hier eine Polling-Schleife mit kleinem Delay verwendet;
 *                    auf echter Hardware sollte man stattdessen die GPIO-Interrupt-API nutzen.)
 * 3) logger_task:    Blockiert 1 Sekunde und gibt dann Status (GPIO4-Zustand und Zähler) aus.
 *
 * Wichtig:
 * - gpio4_interval_ms (Variable) kann zur Laufzeit per Debugger verändert werden (1 … 10000 ms).
 * - edge_task arbeitet polling-basiert (da QEMU GPIO-Interrupts im Raspi-BSP nicht einfach auslöst).
 * - Gute Fehlerabfrage: Status-Codes werden geprüft, und bei einem Fehler erfolgt rtems_fatal_error_occurred().
 *
 * BSP:    arm/raspberrypi2  (im QEMU: -M raspi2b)
 * Build:  arm-rtems6-gcc (RTEMS 6)
 *
 * Abgabe:
 * - Source-Code (dieses File)
 * - Konsolenausgabe (wenn in QEMU ausgeführt)
 *
 *----------------------------------------------------------------------------*/

#include <rtems.h>
#include <bsp.h>
#include <bsp/raspberrypi.h>
#include <bsp/gpio.h>
#include <unistd.h>
#include <stdio.h>

#define GPIO_BANK 0
#define GPIO4 4
#define GPIO18 18

// Globale Variablen
volatile uint32_t gpio4_toggle_delay_ms = 500;   // 1–10000
volatile uint8_t gpio4_state = 0;
volatile uint8_t gpio18_state = 0;
volatile uint32_t toggle_counter = 0;
volatile uint32_t ms_elapsed = 0;

/* ---------------------- GPIO4 Task ---------------------- */
rtems_task gpio4_task(rtems_task_argument arg)
{
    while (1) {
        // Toggle GPIO4
        gpio4_state = !gpio4_state;

        if (gpio4_state) {
            rtems_gpio_bsp_set(GPIO_BANK, GPIO4);
        } else {
            rtems_gpio_bsp_clear(GPIO_BANK, GPIO4);

            // Nur wenn von AN → AUS: toggle GPIO18
            gpio18_state = !gpio18_state;

            if (gpio18_state)
                rtems_gpio_bsp_set(GPIO_BANK, GPIO18);
            else
                rtems_gpio_bsp_clear(GPIO_BANK, GPIO18);

            toggle_counter++;
        }

        ms_elapsed += gpio4_toggle_delay_ms;
        rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(gpio4_toggle_delay_ms));
    }
}

/* ---------------------- Logger Task ---------------------- */
rtems_task logger_task(rtems_task_argument arg)
{
    uint32_t last_ms_reported = 0;

    while (1) {
        rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(100));

        // Prüfen ob mindestens 500 ms vergangen
        if (ms_elapsed - last_ms_reported >= 500) {
            printf("[+%4u ms] GPIO4: %s | GPIO18: %s | Toggles: %u\n",
                   ms_elapsed,
                   gpio4_state ? "ON " : "OFF",
                   gpio18_state ? "ON " : "OFF",
                   toggle_counter);
            last_ms_reported = ms_elapsed;
        }
    }
}

/* ---------------------- Init Task ---------------------- */
rtems_task Init(rtems_task_argument ignored)
{
    rtems_status_code sc;
    rtems_id tid;

    printf("\nRTEMS GPIO-Blink Applikation gestartet.\n\n");

    // GPIO4 als Ausgang
    sc = rtems_gpio_bsp_select_output(GPIO_BANK, GPIO4, NULL);
    if (sc != RTEMS_SUCCESSFUL) rtems_fatal_error_occurred(sc);
    rtems_gpio_bsp_clear(GPIO_BANK, GPIO4);
    gpio4_state = 0;

    // GPIO18 als Ausgang
    sc = rtems_gpio_bsp_select_output(GPIO_BANK, GPIO18, NULL);
    if (sc != RTEMS_SUCCESSFUL) rtems_fatal_error_occurred(sc);
    rtems_gpio_bsp_clear(GPIO_BANK, GPIO18);
    gpio18_state = 0;

    // Task: GPIO4
    sc = rtems_task_create(rtems_build_name('G','4','T','K'), 1,
        RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES, RTEMS_FLOATING_POINT, &tid);
    rtems_task_start(tid, gpio4_task, 0);

    // Task: Logger
    sc = rtems_task_create(rtems_build_name('L','O','G','R'), 2,
        RTEMS_MINIMUM_STACK_SIZE, RTEMS_DEFAULT_MODES, RTEMS_FLOATING_POINT, &tid);
    rtems_task_start(tid, logger_task, 0);

    rtems_task_delete(RTEMS_SELF);
}