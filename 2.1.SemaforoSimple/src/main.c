#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <debounce.h>

#define	DEBOUNCE_PERIOD_MILLIS	1

static xSemaphoreHandle sem;

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	// RGB Rojo
	Chip_GPIO_SetDir(LPC_GPIO, 2, 0, true);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
}

static int isPushed(void* args) {
	// activo bajo
	return !Chip_GPIO_GetPinState(LPC_GPIO, 1, 31);
}

static void blink() {
	int i;
	for (i = 0; i < 5; i++) {
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);
		vTaskDelay(100 / portTICK_RATE_MS);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

static void task_debounce() {
	debounce_t sw4 = debounce_add(20, isPushed, NULL);

	while (1) {
		portTickType tick = xTaskGetTickCount();

		debounce_update(&sw4);

		if (sw4.change == FELL) {
			// no hago nada
		} else if (sw4.change == ROSE) {
			xSemaphoreGive(sem);
		}

		vTaskDelayUntil(&tick, DEBOUNCE_PERIOD_MILLIS / portTICK_RATE_MS);
	}
}

static void task_consumer() {
	while (1) {
		if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
			blink();
		}
	}
}

int main(void) {
	prvSetupHardware();

	vSemaphoreCreateBinary(sem);

	xSemaphoreTake(sem, portMAX_DELAY);

	xTaskCreate(task_debounce, (signed char * ) "debounce",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);
	xTaskCreate(task_consumer, (signed char * ) "consumer",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
}
