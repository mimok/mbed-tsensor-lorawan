/*
 * sensor.c
 *
 *  Created on: 23 mars 2020
 *      Author: mimok
 */

#include "apdu.h"

#define I2C_SENSOR_BUS_ADDRESS 0x48 /* I2C bus address of sensor */

//Configuration settings
#define TMP3_ONESHOT	0x80 //One Shot mode
#define TMP3_RES9		0x00 //9-bit resultion
#define TMP3_RES10		0x20 //10-bit resolution
#define TMP3_RES11		0x40 //11-bit resolution
#define TMP3_RES12		0x60 //12-bit resolution
#define TMP3_FAULT1		0x00 //1 fault queue bits
#define TMP3_FAULT2		0x08 //2 fault queue bits
#define TMP3_FAULT4		0x10 //4 fault queue bits
#define TMP3_FAULT6		0x18 //6 fault queue bits
#define TMP3_ALERTLOW	0x00 //Alert bit active-low
#define TMP3_ALERTHIGH	0x04 //Alert bit active-high
#define TMP3_CMPMODE	0x00 //comparator mode
#define TMP3_INTMODE	0x02 //interrupt mode
#define TMP3_SHUTDOWN	0x01 //Shutdown enabled
#define	TMP3_STARTUP	0x00 //Shutdown Disabled
						 //Default Startup Configuration Used, this is just so the device can be
						 //reset to startup configurations at a later time, it doesn't need to be
						 //called anywhere.
#define TMP3_CONF_DEFAULT	(TMP3_RES9 | TMP3_FAULT1 | TMP3_ALERTLOW | TMP3_CMPMODE)
#define	TMP3_MIN		-128 //Minimum input temperature for the Hyst/Limit registers
#define	TMP3_MAX		127.5 //Maximum input temperature for the Hyst/Limit registers

uint8_t getTemp(apdu_ctx_t *ctx, uint16_t *temp, attestation_t *attestation) {

	apdu_status_t status;
	i2cm_tlv_t tlv[5] = {0};
	SE050_AttestationAlgo_t algo = SE050_AttestationAlgo_EC_SHA_512;
	uint8_t random[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

	uint8_t cmd_config_i2cm[2] = {I2C_SENSOR_BUS_ADDRESS, 0x01};
	tlv[0].tag = SE050_TAG_I2CM_Config;
	tlv[0].cmd.len = 2;
	tlv[0].cmd.p_data = &cmd_config_i2cm[0];
	uint8_t cmd_config[2] = {0x01, TMP3_ONESHOT | TMP3_RES11};
	tlv[1].tag = SE050_TAG_I2CM_Write;
	tlv[1].cmd.len = 2;
	tlv[1].cmd.p_data = &cmd_config[0];
	uint8_t cmd_gettemp[1] = {0x00};
	tlv[2].tag = SE050_TAG_I2CM_Write;
	tlv[2].cmd.len = 1;
	tlv[2].cmd.p_data = &cmd_gettemp[0];
	tlv[3].tag = SE050_TAG_I2CM_Read;
	tlv[3].cmd.len = 2;

	status = se050_i2cm_attestedCmds(
			0x48,
			0x01,
			&tlv[0],
			4,
			algo,
			&random[0],
			attestation,
			ctx);
	*temp = (tlv[3].rsp.p_data[0] << 8)  | tlv[3].rsp.p_data[1];

	if(status != APDU_OK || ctx->sw != 0x9000)
		return APDU_ERROR;
	else
		return APDU_OK;
}
