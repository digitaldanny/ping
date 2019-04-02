/*
 * Joystick.h
 *
 *  Created on: Jan 31, 2017
 *      Author: Danny
 */

#ifndef BOARDSUPPORTPACKAGE_JOYSTICK_H_
#define BOARDSUPPORTPACKAGE_JOYSTICK_H_

/*********************************************** Public Functions *********************************************************************/
/*
 * Initializes internal ADC
 * ADC input from P6.0 and P6.1 ( A15 and A14 )
 * Configured for 14-bit res
 */
void Joystick_Init_Without_Interrupt();

/*
 * Init's GPIO interrupt for joystick push button
 * Param: Function pointer to button handler
 */
void Joystick_Push_Button_Init_With_Interrupt(void (*buttonISR)(void));

/*
 * Functions returns X and Y coordinates
 */
void GetJoystickCoordinates(int16_t *x_coord, int16_t *y_coord);

/*
 * Function to disable joystick interrupt
 */
void Disable_Joystick_Interrupt();

/*********************************************** Public Functions *********************************************************************/



#endif /* BOARDSUPPORTPACKAGE_JOYSTICK_H_ */
