/*
 * @brief FreeRTOS Blinky example
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include "FreeRTOSConfig.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Inicialización de periféricos */
	Board_LED_Set(0, false);

}

static void taskLED(void *pvParameters) {
	while (1) {
		//Board_LED_Toggle(0);
		Board_LED_Set(0, true);
		vTaskDelay(500 / portTICK_RATE_MS);
		Board_LED_Set(0, false);
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
}

int main(void) {
	prvSetupHardware();
	xTaskCreate(taskLED, (signed char * ) "taskLED", configMINIMAL_STACK_SIZE,
			NULL, (tskIDLE_PRIORITY + 1UL), (xTaskHandle *) NULL);

	vTaskStartScheduler();
	return 1;
}

void vApplicationTickHook(void) {
}
