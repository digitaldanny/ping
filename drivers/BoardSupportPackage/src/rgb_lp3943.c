/*
 * rgb_lp3943.c
 *
 *  Created on: Jan 23, 2019
 *      Author: dhami
 */

#include "rgb_lp3943.h"

// fix inputs to the setLedMode function by reversing the
// bits before sending to the LED drivers
uint16_t rev_bit_16( uint16_t orig )
{
    uint16_t rev = 0;

    for (int i = 0; i < 16; i++)
    {
        if ( (orig & (1 << i) ) )
            rev |= 1 << ( (16 - 1) - i );
    }

    return rev;
}

// all functions below use this function to determine the address based
// on the chosen ENUM values.
uint32_t findSlaveAddr( rgb_t rgb_unit )
{
    // filter through enum list
    uint32_t slaveAddr = 0;
    if (rgb_unit == BLUE)
        slaveAddr = 0;
    else if (rgb_unit == GREEN)
        slaveAddr = 1;
    else
        slaveAddr = 2;

    return 0x60 + slaveAddr; // allow auto-incrementing
}

// This function sets the PWM0 duty cycle for the selected RGB unit
// using a default PWM frequency of 60 Hz
void setLedPwm_lp3943( rgb_t rgb_unit, uint8_t led_pwm )
{
    #define PWM_FREQ 160 //Hz

    // determine initial slave address
    uint8_t slaveAddr = findSlaveAddr( rgb_unit );

    // wait for transmission availability
    UCB2I2CSA = slaveAddr;              // set slave address
    UCB2CTLW0 |= UCTXSTT;               // Set START condition
    while ( UCB2CTLW0 & UCTXSTT );    // wait for SST BIT INSTEAD**************

    //*******************************************************************
    // increment to next led register
    UCB2TXBUF = 0x12;
    while( !(UCB2IFG & UCTXIFG0) );
    //*******************************************************************

    // Fill txbuf with the pwm frequency data for the lp3943.
    UCB2TXBUF = (uint8_t)(160/PWM_FREQ - 1);    // load 1 byte at a time
    while ( !(UCB2IFG & UCTXIFG0) );            // wait for the tx buffer availability

    // Fill txbuf with the pwm duty cycle data for the lp3943.
    UCB2TXBUF = led_pwm;                // load 1 byte at a time
    while ( !(UCB2IFG & UCTXIFG0) );    // wait for the tx buffer availability

    // generate STOP condition
    UCB2CTLW0 |= UCTXSTP;
    while ( UCB2CTLW0 & UCTXSTP );

    // send data through the I2C module
    //i2cWrite( slaveAddr, &led_pwm_array, sizeof(led_pwm_array) );
}

// This function sets each of the LEDs to the operating mode
// of OFF (00b), ON (01b), PWM1 (10b), PWM2 (11b)
void setLedMode_lp3943( rgb_t rgb_unit, uint16_t led_mode)
{
    uint32_t slaveAddr = findSlaveAddr(rgb_unit);

    // LS0 | LED0-3 selector
    // LS1 | LED4-7 selector
    // LS2 | LED8-11 selector
    // LS3 | LED12-15 selector

    #define ON_ 0x01     // PWM0 is the ON value instead of Vdc
    #define OFF_ 0x00    // GND is the OFF value

    // consider each nibble as the data for LEDX->X+3
    // Each bit is analyze as an LED ON or OFF and
    uint8_t LS[5];
    LS[0] = 0x16;   // increment starting from the LS0 register to LS3
    uint8_t nextLS = 1;
    uint8_t nextLED = 0;
    uint8_t nextBit;
    for (int i = 0; i < 16; i++)
    {
        // next bit of the LED values
        nextBit = (led_mode >> i) & 0x1;

        // determine if the next LED is ON or OFF
        if (nextBit == ON_)
        {
            LS[nextLS] |= ( BIT0 << nextLED ); //BIT1 for PWM - BIT0 for ON
            LS[nextLS] &= ~( BIT1 << nextLED ); //BIT0 for PWM - BIT1 for ON
        }
        else // nextBit == OFF_
        {
            LS[nextLS] &= ~( (BIT1 | BIT0) << nextLED ); //11
        }

        // go to the next LS byte and reset LED bit shifter
        nextLED += 2;
        if (nextLED >= 8)
        {
            nextLED = 0;
            nextLS++;
        }
    }

    // wait for transmission availability
    UCB2I2CSA = slaveAddr;          // set slave address
    UCB2CTLW0 |= UCTXSTT;           // Set START condition
    while ( UCB2CTLW0 & UCTXSTT );  // wait for buffer availability************************************

    // Fill txbuf with the data for the lp3943.
    for (int i = 0; i < 5; i++)
    {
        UCB2TXBUF = LS[i];                                  // load 1 byte at a time
        while ( !(UCB2IFG & UCTXIFG0) );                    // wait for the tx buffer availability
    }

    // generate STOP condition
    UCB2CTLW0 |= UCTXSTP;
    while ( UCB2CTLW0 & UCTXSTP );

    //i2cWrite( slaveAddr, &LS, sizeof(LS) );
}

// Initialize I2C peripheral on eUSCI_B_2 port and
// set LEDs to be off. This driver enables PWM mode
// for LP3943 with default prescaler = XXXXXX and
// default period = YYYYYY
void config_pwm_lp3943( void )
{
    uint16_t UNIT_OFF = 0x0000;

    // software reset enable
    UCB2CTLW0 = UCSWRST;

    // Initialize I2C master - master, i2c mode, clock sync, smclk, transmitter
    #define UCMST_MASTEREN      (0x1 << 11) // master mode enabled
    #define UCMST_I2CEN         (0x3 << 9)  // i2c mode enabled
    #define UCMST_CLKSYNC       (0x1 << 8)  // synchronous mode enabled
    #define UCMST_SMCLK         (0x3 << 6)  // SMCLK set at source
    #define UCMST_TRANSMITTER   (0x1 << 4)  // transmitter mode enabled
    UCB2CTLW0 |= UCMST_MASTEREN | UCMST_I2CEN | UCMST_CLKSYNC | UCMST_SMCLK | UCMST_TRANSMITTER;

    // Set Fclk as 400 kHz - SMCLK selected as source, FSMCLK is 12MHz
    UCB2BRW = 30;   // prescaler

    // Set pins as I2C mode. (Table on p160 of SLAS826E)
    // Set P3.6 as UCB2_SDA and 3.7 as UCB2_SLC
    //P3DIR |= BIT7 | BIT6;
    P3SEL1 &= BIT7 | BIT6;  // 0b10 for SEL10 to allow I2C module
    P3SEL0 |= BIT7 | BIT6;  // 0b10 for SEL10 to allow I2C module

    // Bitwise anding of all bits except UCSWRST to disable reset
    UCB2CTLW0 &= ~UCSWRST;

    setLedMode_lp3943(RED, UNIT_OFF);
    setLedMode_lp3943(GREEN, UNIT_OFF);
    setLedMode_lp3943(BLUE, UNIT_OFF);
}


