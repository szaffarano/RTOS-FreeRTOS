#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "debounce.h"

static int application_ticks = 0;

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
}

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
			application_ticks = 0;
		} else if (sw4.change == ROSE) {
			Board_LED_Set(0, true);
			vTaskDelay(application_ticks / portTICK_RATE_MS);
			Board_LED_Set(0, false);
		}
		vTaskDelayUntil(&tick, 1 / portTICK_RATE_MS);
	}
}

int main(void) {
	prvSetupHardware();
	xTaskCreate(task_debounce, (signed char * ) "taskLED",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
	application_ticks++;
}
/**
 * @}
 */
