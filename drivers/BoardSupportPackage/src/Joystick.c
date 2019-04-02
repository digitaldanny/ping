/*
 * Joystick.c
 *
 *  Created on: Jan 31, 2017
 *      Author: Danny
 */
/*********************************************** Dependencies and Externs *************************************************************/
#include <stdint.h>
#include "msp.h"
#include "Joystick.h"
#include "driverlib.h"
/*********************************************** Dependencies and Externs *************************************************************/


/*********************************************** Defines *********************************************************************/
#define X_COORD_ADC_PIN 0
#define Y_COORD_ADC_PIN 1
/*********************************************** Defines *********************************************************************/



/*********************************************** Public Variables ********************************************************************/

/* Function Pointer for calling user defined function for joystick button press */
void (*ButtonFunction)(void);

/*********************************************** Public Variables ********************************************************************/


/*********************************************** Public Functions *********************************************************************/
/*
 * Initializes internal ADC
 * ADC input from P6.0 and P6.1 ( A15 and A14 )
 * Configured for 14-bit res
 */
void Joystick_Init_Without_Interrupt()
{
    /* Initialize ADCs for x and y coordinates */
    // Configure Port6.0 GPIO function for A15 (ADC) - 6.12.11 in Datasheet
    P6SEL1 |= BIT0;
    P6SEL0 |= BIT0;
    P6->DIR &= ~BIT0;   // set pin as input for good measure but not needed

    // Configure Port6.1 GPIO function for A14 (ADC) - 6.12.11 in Datasheet
    P6SEL1 |= BIT1;
    P6SEL0 |= BIT1;
    P6->DIR &= ~BIT1;   // set pin as input for good measure but not needed

    // Enable ADC, sample-and-hold pulse-mode, SM CLK, multiple sample
    ADC14->CTL0 = ADC14_CTL0_ON | ADC14_CTL0_SHP | ADC14_CTL0_SSEL__SMCLK | ADC14_CTL0_MSC | ADC14_CTL0_CONSEQ_1;

    // Select CH0, 14bit res
    ADC14->CTL1 = ADC14_CTL1_CH0MAP | ADC14_CTL1_RES__14BIT;

    // X coordinate -- P6.0 -- A15
    ADC14->MCTL[X_COORD_ADC_PIN] |= ADC14_MCTLN_INCH_15;

    // Y coordinate -- P6.1 -- A14
    ADC14->MCTL[Y_COORD_ADC_PIN] |= ADC14_MCTLN_INCH_14 | ADC14_MCTLN_EOS;  // End of sequence
}

/*
 * Init's GPIO interrupt for joystick push button
 * Param: Function pointer to button handler
 */
void Joystick_Push_Button_Init_With_Interrupt(void (*UserButtonFunction)(void))
{
    /* Initialize Button Press Interrupt */
    // Configure Port4.3 to be used as GPIO interrupt for joystick button press -- falling edge triggered
    P4->DIR &= ~BIT3;        // Set P4.3 as input
    P4->IFG &= ~BIT3;       // P4.3 IFG cleared
    P4->IE |= BIT3;         // Enable interrupt on P4.3
    P4->IES |= BIT3;        // high-to-low transition
    P4->REN |= BIT3;        // Pull-up resister
    P4->OUT |= BIT3;

    NVIC_EnableIRQ(PORT4_IRQn);
    MAP_Interrupt_setPriority(INT_PORT4, 6);
    ButtonFunction = UserButtonFunction;
}

/*
 * Function to disable joystick interrupt
 */
void Disable_Joystick_Interrupt()
{
    P4->IFG &= ~BIT3;       // P4.3 IFG cleared
    P4->IE &= ~BIT3;         // Disable interrupt on P4.3
}

/*
 * Functions returns X and Y coordinates
 */
void GetJoystickCoordinates(int16_t *x_coord, int16_t *y_coord)
{
    // Start conversion
    ADC14->CTL0 |= ADC14_CTL0_ENC | ADC14_CTL0_SC;

    // Wait until conversion is complete
    while(ADC14->CTL0 & ADC14_CTL0_BUSY); // consider implementing ISR for conversion complete

    // Read from x and y coordinates
    *x_coord = ADC14->MEM[X_COORD_ADC_PIN] - 0x1FFF;
    *y_coord = ADC14->MEM[Y_COORD_ADC_PIN] - 0x1FFF;
}

//__interrupt void PORT4_IRQHandler (void)
//{
//    ButtonFunction();
//
//    P4->IFG &= ~BIT3;       // P4.3 IFG cleared
//}

/*********************************************** Public Functions *********************************************************************/

