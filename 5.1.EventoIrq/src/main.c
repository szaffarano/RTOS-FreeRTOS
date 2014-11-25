#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

xSemaphoreHandle semBin;

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
	xSemaphoreTake(semBin, portMAX_DELAY);

	while (1) {
		xSemaphoreTake(semBin, portMAX_DELAY);
		portTickType now = xTaskGetTickCount();
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, 1);
		vTaskDelayUntil(&now, 1000 / portTICK_RATE_MS);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, 0);
	}
}

// handler de interrupción
void EINT3_IRQHandler(void) {
	portBASE_TYPE xSwitchRequired = pdFALSE;

	if (Chip_GPIOINT_GetIntFalling(LPC_GPIOINT, GPIOINT_PORT0) & (1 << 15)) {
		Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, GPIOINT_PORT0, (1 << 15));
		xSemaphoreGiveFromISR(semBin, &xSwitchRequired);
	}

	portEND_SWITCHING_ISR(xSwitchRequired);
}

int main(void) {
	prvSetupHardware();

	vSemaphoreCreateBinary(semBin);

	xTaskCreate(taskLED, (signed char* ) "led", configMINIMAL_STACK_SIZE, NULL,
			tskIDLE_PRIORITY, NULL);

	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
}
