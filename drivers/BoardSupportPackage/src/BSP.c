/*
 * BSP.c
 *
 *  Created on: Dec 30, 2016
 *      Author: Raz Aloni
 */

#include <driverlib.h>
#include "BSP.h"
#include "i2c_driver.h"


/* Initializes the entire board */
void BSP_InitBoard()
{
	/* Disable Watchdog */
	WDT_A_clearTimer();
	WDT_A_holdTimer();

	/* Initialize Clock */
	ClockSys_SetMaxFreq();

	/* Init i2c */
	initI2C();

	/* Init Opt3001 */
	sensorOpt3001Enable(true);

	/* Init Tmp007 */
	sensorTmp007Enable(true);

	/* Init Bmi160 */
    bmi160_initialize_sensor();

    /* Init joystick without interrupts */
	Joystick_Init_Without_Interrupt();

	/* Init Bme280 */
	bme280_initialize_sensor();

	/* Init BackChannel UART */
	BackChannelInit();

	/* Init RGB LEDs */
	config_pwm_lp3943();
}


