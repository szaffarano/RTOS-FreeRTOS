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
	Chip_IOCON_PinMux(LPC_IOCON, 0, 26, IOCON_MODE_PULLUP, IOCON_FUNC0);
}

void tareaLED(void* p) {
	devGPIO_pin_t* pin = (devGPIO_pin_t*) p;

	Chip_GPIO_SetPinDIR(LPC_GPIO, pin->port, pin->bit, 1);

	while (1) {
		char state;
		if (xQueueReceive(queue, &state, portMAX_DELAY) == pdTRUE) {
			Chip_GPIO_SetPinState(LPC_GPIO, pin->port, pin->bit, state);
		} else {
			// timeout
		}
	}
}

void tareaCola(void* p) {
	char state = 0;
	while (1) {
		xQueueSend(queue, &state, portMAX_DELAY);
		state = !state;
		xQueueSend(queue, &state, portMAX_DELAY);
		vTaskDelay(200 / portTICK_RATE_MS);
	}
}

int main(void) {
	// en el stack del main() y eso puede traer problemas.  Al definirlas
	// como estáticas, son globales, pero con alcance acotado a main().
	static const devGPIO_pin_t ledBlue = { 0, 26, 1 };
	static const devGPIO_pin_t ledRed = { 2, 0, 1 };

	prvSetupHardware();

	queue = xQueueCreate(10, sizeof(char));

	// variables estaticas! esto es porque si no lo son, van a vivir
	xTaskCreate(tareaLED, (signed char* ) "blue", configMINIMAL_STACK_SIZE * 2,
			(void* )&ledBlue, tskIDLE_PRIORITY+1, NULL);

	xTaskCreate(tareaLED, (signed char* ) "red", configMINIMAL_STACK_SIZE * 2,
			(void* )&ledRed, tskIDLE_PRIORITY+1, NULL);

	xTaskCreate(tareaCola, (signed char* ) "tareaCola",
			configMINIMAL_STACK_SIZE * 2, NULL, tskIDLE_PRIORITY+1, NULL);

	vTaskStartScheduler();

	return 1;
}

void vApplicationTickHook(void) {
}
