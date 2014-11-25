#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include <debounce.h>

#define	DEBOUNCE_PERIOD_MILLIS	1

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
}

typedef enum _state_t {
	ESPERANDO_APRETAR, RUIDO_APRETAR, ESPERANDO_SOLTAR, RUIDO_SOLTAR
} state_t;

static int application_ticks = 0;
static int pushed_ticks = 0;

static int pushed(void* args) {
	// activo bajo
	return !Chip_GPIO_GetPinState(LPC_GPIO, 1, 31);
}

static void task_debounce() {
	debounce_t sw4 = debounce_add(20, pushed, NULL);
	while (1) {
		portTickType tick = xTaskGetTickCount();

		debounce_update(&sw4);
		if (sw4.change == FELL) {
			// apretaron, inicializo ticks
			application_ticks = 0;
		} else if (sw4.change == ROSE) {
			// soltaron, calculo los ticks
			taskENTER_CRITICAL();
			pushed_ticks = application_ticks;
			taskEXIT_CRITICAL();
		}
		vTaskDelayUntil(&tick, DEBOUNCE_PERIOD_MILLIS / portTICK_RATE_MS);
	}
}

static void task_duty() {
	static int delay;
	while (1) {
		portTickType tick = xTaskGetTickCount();
		Board_LED_Set(0, true);

		taskENTER_CRITICAL();
		delay = pushed_ticks;
		taskEXIT_CRITICAL();

		vTaskDelay(delay / portTICK_RATE_MS);

		Board_LED_Set(0, false);
		vTaskDelayUntil(&tick, 1000 / portTICK_RATE_MS);
	}
}

int main(void) {
	prvSetupHardware();
	xTaskCreate(task_debounce, (signed char * ) "debounce",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);
	xTaskCreate(task_duty, (signed char * ) "blink", configMINIMAL_STACK_SIZE,
			NULL, (tskIDLE_PRIORITY + 1UL), (xTaskHandle *) NULL);
	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
	application_ticks++;
}
