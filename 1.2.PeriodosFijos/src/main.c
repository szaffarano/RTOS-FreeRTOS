#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);

}

static void square() {
	int i = 1;
	while (1) {
		for (i = 1; i <= 10; i++) {
			portTickType tick = xTaskGetTickCount();
			Board_LED_Set(0, true);
			vTaskDelay(i * 100);
			Board_LED_Set(0, false);
			vTaskDelayUntil(&tick, 1000 / portTICK_RATE_MS);
		}
	}
}

int main(void) {
	prvSetupHardware();
	xTaskCreate(square, (signed char * ) "taskLED", configMINIMAL_STACK_SIZE,
			NULL, (tskIDLE_PRIORITY + 1UL), (xTaskHandle *) NULL);

	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
}
