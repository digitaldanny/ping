/*
 * BackChannelUart.h
 *
 * Holds all logic and functionality for communicating via the Back Channel UART
 * The Back Channel UART is configured to run at 115200 Baud with 1 start bit, 8 data bits, 1 stop bit, and no parity bit
 * The Back Channel UART will transmit board related data in JSON to be read by the COM port receiver (Host PC)
 *
 *  Created on: Dec 31, 2016
 *      Author: Raz Aloni
 */

#ifndef BACKCHANNELUART_H_
#define BACKCHANNELUART_H_

#include "BSP.h"

typedef enum
{
	BackChannel_Info,
	BackChannel_Warning,
	BackChannel_Error
} BackChannelTextStyle_t;

/* Initializes back channel UART */
extern void BackChannelInit();

/*
 * Prints string to the back channel UART
 * Param 'string': String to be displayed
 * Param 'textStyle': Style of the the text to be written
 */
extern void BackChannelPrint(const char * string, BackChannelTextStyle_t textStyle);

/*
 * Prints the value of an integer to the back channel UART
 * Param 'name': Name of the integer variable
 * Param 'value': Value of integer variable
 */
extern void BackChannelPrintIntVariable(const char * name, int32_t value);

/*
 * Sends an event trigger to the back channel UART
 * Param 'eventNumber': Event number indicator
 */
extern void BackChannelEventTrigger(uint_fast8_t eventNumber);

/*
 * Sends Opt3001 raw data to back channel UART
 * Param 'rawData': Opt3001 Raw data
 */
extern void BackChannelOpt3001PrintRaw(uint16_t rawData);

/*
 * Sends Tmp007 raw data to back channel UART
 * Param 'rawAmbTemp': Tmp007 Raw Ambient Temperature Data
 * Param 'rawObjTemp': Tmp007 Raw Object Temperature Data
 */
extern void BackupChannelTmp007PrintRaw(uint16_t rawAmbTemp, uint16_t rawObjTemp);

/*
 * Sends Bmi160 raw Accelerometer data to back channel UART
 * Param 'accelData': Bmi160 Raw Accelerometer Data
 */
extern void BackupChannelBmi160PrintAccel(struct bmi160_accel_t * accelData);

/*
 * Sends Bmi160 raw Gyro data to back channel UART
 * Param 'gyroData': Bmi160 Raw Gyro Data
 */
extern void BackupChannelBmi160PrintGyro(struct bmi160_gyro_t * gyroData);

/*
 * Sends Bmi160 raw Magnetometer data to back channel UART
 * Param 'magData': Bmi160 Raw Magnetometer Data
 */
extern void BackupChannelBmi160PrintMag(struct bmi160_mag_t * magData);

#endif /* BACKCHANNELUART_H_ */
