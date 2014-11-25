#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <debounce.h>

static long system_ticks;

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	// led del stick
	Board_LED_Set(0, false);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
}

static int isPushed(void* args) {
	// activo bajo
	return !Chip_GPIO_GetPinState(LPC_GPIO, 1, 31);
}

static void tareaLED(void* p) {
	xQueueHandle queue = p;
	long delay = 500;
	while (1) {
        portTickType tick = xTaskGetTickCount();
		xQueueReceive(queue, &delay, 1 / portTICK_RATE_MS);
		Board_LED_Toggle(0);
		vTaskDelayUntil(&tick, delay / portTICK_RATE_MS);
	}
}

static void tareaCola(void* p) {
	xQueueHandle queue = p;
	debounce_t sw4 = debounce_add(0, isPushed, NULL);
	long delay = 0;
	while (1) {
		debounce_update(&sw4);
		if (sw4.change == FELL) {
			taskENTER_CRITICAL();
			system_ticks = 0;
			taskEXIT_CRITICAL();
		} else if (sw4.change == ROSE) {
			taskENTER_CRITICAL();
			delay = system_ticks;
			taskEXIT_CRITICAL();
			xQueueSend(queue, &delay, 0);
		}
		vTaskDelay(1 / portTICK_RATE_MS);
	}
}

int main(void) {
	xQueueHandle queue;

	prvSetupHardware();

	queue = xQueueCreate(10, sizeof(long));

	xTaskCreate(tareaLED, (signed char* ) "tareaLED",
			configMINIMAL_STACK_SIZE * 2, queue, tskIDLE_PRIORITY+1, NULL);

	xTaskCreate(tareaCola, (signed char* ) "tareaCola",
			configMINIMAL_STACK_SIZE * 2, queue, tskIDLE_PRIORITY+1, NULL);

	vTaskStartScheduler();

	return 1;
}

void vApplicationTickHook(void) {
	system_ticks++;
}
