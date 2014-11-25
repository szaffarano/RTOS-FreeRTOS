#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <debounce.h>

static long system_ticks;

typedef struct _pin_t {
	uint32_t port;
	uint32_t bit;
	uint32_t value;
} pin_t;

typedef struct _blinky_data_t {
	pin_t pin;
	long delay;
} blinky_data_t;

blinky_data_t data;

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

	while (1) {
		portTickType tick = xTaskGetTickCount();
		xQueueReceive(queue, &data, 1 / portTICK_RATE_MS);

		Chip_GPIO_SetPinToggle(LPC_GPIO, data.pin.port, data.pin.bit);

		vTaskDelayUntil(&tick, data.delay / portTICK_RATE_MS);
	}
}

static void tareaCola(void* p) {
	xQueueHandle queue = p;
	debounce_t sw4 = debounce_add(0, isPushed, NULL);

	while (1) {
		debounce_update(&sw4);
		if (sw4.change == FELL) {
			taskENTER_CRITICAL();
			system_ticks = 0;
			taskEXIT_CRITICAL();
		} else if (sw4.change == ROSE) {
			taskENTER_CRITICAL();
			data.delay = system_ticks;
			taskEXIT_CRITICAL();

			xQueueSend(queue, &data, 0);
		}
		vTaskDelay(1 / portTICK_RATE_MS);
	}
}

int main(void) {
	xQueueHandle queue;

	prvSetupHardware();

	queue = xQueueCreate(10, sizeof(data));

	/* led rojo */
	data.pin = (pin_t ) { 2, 0, 1 };
	data.delay = 500;

	Chip_GPIO_SetDir(LPC_GPIO, data.pin.port, data.pin.bit, true);

	xQueueSend(queue, &data, 0);

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
