/*
 * sensor.h
 *
 *  Created on: 23 mars 2020
 *      Author: mimok
 */

#ifndef SENSOR_H_
#define SENSOR_H_

uint8_t getTemp(apdu_ctx_t *ctx, uint16_t *temp, attestation_t *attestation);

#endif /* SENSOR_H_ */
