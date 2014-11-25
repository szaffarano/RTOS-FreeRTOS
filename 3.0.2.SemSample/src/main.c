#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

typedef struct {
	uint32_t port;
	uint32_t bit;
	uint32_t value;
} devGPIO_pin_t;

xQueueHandle queue;

static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	Board_LED_Set(0, false);
}

void tareaLED(void* p) {
	devGPIO_pin_t* pin = (devGPIO_pin_t*) p;

	Chip_GPIO_SetPinDIR(LPC_GPIO, pin->port, pin->bit, 1);

	while (1) {
		char state;
		if (xQueueReceive(queue, &state, 10 / portTICK_RATE_MS) == pdTRUE) {
			Chip_GPIO_SetPinState(LPC_GPIO, pin->port, pin->bit, state);
		} else {
			// timeout
		}
	}
}

void tareaCola(void* p) {
	char state = 0;
	while (1) {

		if (xQueueSend(queue, &state, 0) == pdTRUE) {
			state = !state;
			vTaskDelay(100 / portTICK_RATE_MS);
		} else {
			// full queue
		}
	}
}

int main(void) {
	// variables estaticas! esto es porque si no lo son, van a vivir
	// en el stack del main() y eso puede traer problemas.  Al definirlas
	// como estáticas, son globales, pero con alcance acotado a main().
	static const devGPIO_pin_t ledBlue = { 2, 0, 1 };
	static const devGPIO_pin_t ledRed = { 2, 1, 1 };

	prvSetupHardware();

	queue = xQueueCreate(10, sizeof(char));

	xTaskCreate(tareaLED, (signed char* ) "tareaLED",
			configMINIMAL_STACK_SIZE * 2, (void* )&ledBlue, tskIDLE_PRIORITY+1,
			NULL);

	xTaskCreate(tareaLED, (signed char* ) "tareaLED2",
			configMINIMAL_STACK_SIZE * 2, (void* )&ledRed, tskIDLE_PRIORITY+1,
			NULL);

	xTaskCreate(tareaCola, (signed char* ) "tareaCola",
			configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY+1, NULL);

	vTaskStartScheduler();

	return 1;
}

void vApplicationTickHook(void) {
}
