#include <board.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <debounce.h>

static long system_ticks;
static xSemaphoreHandle mutex;
static xQueueHandle queue;

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

static void squareTask(void* p) {
	while (1) {
		if (xSemaphoreTake(mutex, portMAX_DELAY)) {
			portTickType tick = xTaskGetTickCount();
			Board_LED_Toggle(0);
			vTaskDelayUntil(&tick, 500 / portTICK_RATE_MS);
			xSemaphoreGive(mutex);
		}
	}
}

static void controlTask(void *p) {
	debounce_t sw4 = debounce_add(0, isPushed, NULL);
	long delay = 0;

	while (1) {
		debounce_update(&sw4);
		if (sw4.change == FELL) {
			system_ticks = 0;
		} else if (sw4.change == ROSE) {
			delay = system_ticks;
			xQueueSend(queue, &delay, 0);
		}
		vTaskDelay(1 / portTICK_RATE_MS);
	}
}

static void ledTask(void* p) {
	long delay;
	while (1) {
		if (xQueueReceive(queue, &delay, portMAX_DELAY) == pdTRUE) {
			if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
				portTickType tick = xTaskGetTickCount();
				Board_LED_Set(0, 1);
				vTaskDelayUntil(&tick, delay / portTICK_RATE_MS);
				Board_LED_Set(0, 0);
				vTaskDelay(1000 / portTICK_RATE_MS);
				xSemaphoreGive(mutex);
			}
		}
	}
}

int main(void) {
	prvSetupHardware();

	mutex = xSemaphoreCreateMutex();
	queue = xQueueCreate(10, sizeof(long));

	xTaskCreate(squareTask, (signed char* ) "square",
			configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY+1, NULL);

	xTaskCreate(controlTask, (signed char* ) "control",
			configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY+1, NULL);

	xTaskCreate(ledTask, (signed char* ) "led", configMINIMAL_STACK_SIZE * 2,
			NULL, tskIDLE_PRIORITY+1, NULL);

	vTaskStartScheduler();

	return 1;
}

void vApplicationTickHook(void) {
	system_ticks++;
}
