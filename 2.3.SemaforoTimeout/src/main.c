#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <debounce.h>

#define	DEBOUNCE_PERIOD_MILLIS	1

#define	STEPS					5
#define	PWM						1000

static xSemaphoreHandle sem;
static int counter = 0;

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Inicialización de periféricos */

	// RGB Rojo
	Chip_GPIO_SetDir(LPC_GPIO, 2, 0, true);

	// RGB Verde
	Chip_GPIO_SetDir(LPC_GPIO, 2, 1, true);

	// SW4
	Chip_GPIO_SetDir(LPC_GPIO, 1, 31, false);
}

static int isPushed(void* args) {
	// activo bajo
	return !Chip_GPIO_GetPinState(LPC_GPIO, 1, 31);
}

static void task_control() {
	debounce_t sw4 = debounce_add(20, isPushed, NULL);

	while (1) {
		portTickType tick = xTaskGetTickCount();
		debounce_update(&sw4);

		if (sw4.change == ROSE) {
			xSemaphoreGive(sem);
			if (counter == 18) {
				counter = 0;
			} else {
				counter = counter + 2;
			}
		}

		vTaskDelayUntil(&tick, DEBOUNCE_PERIOD_MILLIS / portTICK_RATE_MS);
	}
}

static void task_pwm_alt() {
//	int i;
	while (1) {
//		for (i = 0; i < PWM; i++) {
//			if (i < (PWM / STEPS) * counter) {
//				Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);
//			} else {
//				Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
//			}
//		}
//		vTaskDelay(5 / portTICK_RATE_MS);
	}
}

static void task_pwm() {
	while (1) {
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, true);
		vTaskDelay(counter);
		Chip_GPIO_SetPinState(LPC_GPIO, 2, 0, false);
		vTaskDelay(20 - counter);
	}
}

int main(void) {
	prvSetupHardware();

	vSemaphoreCreateBinary(sem);

	xSemaphoreTake(sem, portMAX_DELAY);

	xTaskCreate(task_pwm, (signed char * ) "task_pwm", configMINIMAL_STACK_SIZE,
			NULL, (tskIDLE_PRIORITY + 1UL), (xTaskHandle *) NULL);

	xTaskCreate(task_pwm_alt, (signed char * ) "task_pwm_alt",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	xTaskCreate(task_control, (signed char * ) "task_control",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
			(xTaskHandle *) NULL);

	vTaskStartScheduler();

	return 1;
}

void vApplicationTickHook(void) {
}
