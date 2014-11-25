#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define	DEBOUNCE_DELAY_MILLIS	20

static xQueueHandle queue;
static xSemaphoreHandle mutexFalling, mutexRising;
static long ticks;

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	// joystick, configurar IRQ de P0, 15
	Chip_GPIOINT_Init(LPC_GPIOINT);
	Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
	// por defecto todos los pines son input, no hace falta setear la dirección

	NVIC_EnableIRQ(EINT3_IRQn);

	// LED Rojo
	Chip_GPIO_SetDir(LPC_GPIO, 2, 0, 1);
}

static void taskLED(void* p) {
	long delay = 500;
	while (1) {
		if (xQueueReceive(queue, &delay, 1 / portTICK_RATE_MS)) {
			delay = delay;
		}
		Chip_GPIO_SetPinToggle(LPC_GPIO, 2, 0);
		vTaskDelay(delay / portTICK_RATE_MS);
	}
}

static void taskHandleRising(void* p) {
	long delay;
	while (1) {
		if (xSemaphoreTake(mutexRising, portMAX_DELAY) == pdTRUE) {
			vTaskDelay(DEBOUNCE_DELAY_MILLIS / portTICK_RATE_MS);
			if (Chip_GPIO_GetPinState(LPC_GPIO, 0, 15)) {
				Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
				taskENTER_CRITICAL();
				delay = xTaskGetTickCount() - ticks;
				taskEXIT_CRITICAL();

				xQueueSend(queue, &delay, portMAX_DELAY);
			} else {
				Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
			}
		}
	}
}

static void taskHandleFalling(void* p) {
	while (1) {
		if (xSemaphoreTake(mutexFalling, portMAX_DELAY) == pdTRUE) {
			vTaskDelay(DEBOUNCE_DELAY_MILLIS / portTICK_RATE_MS);
			if (!Chip_GPIO_GetPinState(LPC_GPIO, 0, 15)) {
				// comienzo a contar ticks
				taskENTER_CRITICAL();
				ticks = xTaskGetTickCount();
				taskEXIT_CRITICAL();
				Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
			} else {
				Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
			}
		}
	}
}

// handler de interrupción
void EINT3_IRQHandler(void) {
	portBASE_TYPE xSwitchRequired = pdFALSE;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	uint32_t fallingInterrupts = Chip_GPIOINT_GetIntFalling(LPC_GPIOINT, GPIOINT_PORT0);
	uint32_t risingInterrupts = Chip_GPIOINT_GetIntRising(LPC_GPIOINT, GPIOINT_PORT0);
	if (fallingInterrupts & (1 << 15)) {
		xSemaphoreGiveFromISR(mutexFalling, &xHigherPriorityTaskWoken);
		Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, GPIOINT_PORT0, fallingInterrupts & ~(1 << 15));
	} else if (risingInterrupts & (1 << 15)) {
		xSemaphoreGiveFromISR(mutexRising, &xHigherPriorityTaskWoken);
		Chip_GPIOINT_SetIntRising(LPC_GPIOINT, GPIOINT_PORT0, risingInterrupts & ~(1 << 15));
	}

	Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));

	if (xHigherPriorityTaskWoken) {
		taskYIELD();
	}

	portEND_SWITCHING_ISR(xSwitchRequired);
}

int main(void) {
	prvSetupHardware();

	queue = xQueueCreate(10, sizeof(long));
	mutexFalling = xSemaphoreCreateMutex();
	mutexRising = xSemaphoreCreateMutex();

	xSemaphoreTake(mutexFalling, portMAX_DELAY);
	xSemaphoreTake(mutexRising, portMAX_DELAY);

	xTaskCreate(taskLED, (signed char* ) "led", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(taskHandleFalling, (signed char* ) "falling", configMINIMAL_STACK_SIZE, NULL,
			tskIDLE_PRIORITY, NULL);
	xTaskCreate(taskHandleRising, (signed char* ) "rising", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY,
			NULL);

	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
}
