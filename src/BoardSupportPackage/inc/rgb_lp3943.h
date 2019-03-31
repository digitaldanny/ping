/*
 * rgb_lp3943.h
 *
 *  Created on: Jan 22, 2019
 *      Author: Daniel Hamilton
 */

#ifndef RGB_LP3943_H_
#define RGB_LP3943_H_


#include "msp.h"
#include "driverlib.h"

// Address bit settings for A2:A1:A0 of RGB processors
// to control which processor is being used
typedef enum
{
    RED,    // 0x0
    GREEN,  // 0x1
    BLUE    // 0x2
} rgb_t;

typedef enum
{
    OFF,    // 0x0
    ON,     // 0x1
    PWM0,   // 0x2
    PWM1    // 0x3
} rgbmode_t;

// all functions below use this function to determine the address based
// on the chosen ENUM values.
uint32_t findSlaveAddr( rgb_t rgb_unit );

/*
 * POINTER WASN'T WORKING PROPERLY????
void i2cWrite( uint8_t slaveAddr, uint8_t *dataPointer, int count)
{
    // wait for transmission availability
    UCB2I2CSA = slaveAddr;          // set slave address
    UCB2CTLW0 |= UCTXSTT;           // Set START condition
    while ( !(UCB2IFG & UCTXIFG0) );  // wait for buffer availability

    // Fill txbuf with the data for the lp3943.
    for (int nextByte = 0; nextByte < count; nextByte++)
    {
        UCB2TXBUF = *(dataPointer + nextByte) & 0xFF;     // load 1 byte at a time
        while ( !(UCB2IFG & UCTXIFG0) );                    // wait for the tx buffer availability
    }

    // generate STOP condition
    UCB2CTLW0 |= UCTXSTP;
    while ( !(UCB2IFG & UCTXIFG0) );
}
*/

uint16_t rev_bit_16( uint16_t );

// This function sets the PWM duty cycle for the selected RGB unit
void setLedPwm_lp3943( rgb_t rgb_unit, uint8_t led_pwm );

// This function sets each of the LEDs to the operating mode
// of OFF (00b), ON (01b), PWM1 (10b), PWM2 (11b)
void setLedMode_lp3943( rgb_t rgb_unit, uint16_t led_mode);

// Initialize I2C peripheral on eUSCI_B_2 port and
// set LEDs to be off. This driver enables PWM mode
// for LP3943 with default prescaler = XXXXXX and
// default period = YYYYYY
void config_pwm_lp3943( void );

#endif /* RGB_LP3943_H_ */
