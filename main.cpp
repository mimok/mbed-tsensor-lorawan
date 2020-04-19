/**
 * Copyright (c) 2020, Michael Grand
 * Copyright (c) 2017, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


/* ************************************************************************** */
/* Includes                                                                   */
/* ************************************************************************** */
#include "mbed.h"

extern "C" {
#include "mbed-se050-drv/se050.h"
#include "sensor.h"
#include "util.h"
}

#include "lorawan/LoRaRadio.h"
#include "SX1276_LoRaRadio.h"
#include "lorawan/LoRaWANInterface.h"
#include "lorawan/system/lorawan_data_structures.h"
#include "events/EventQueue.h"

/* ************************************************************************** */
/* Local Defines                                                              */
/* ************************************************************************** */

/*
 * Sets up an application dependent transmission timer in ms. Used only when Duty Cycling is off for testing
 */
#define TX_TIMER                        10000

/**
 * Maximum number of events for the event queue.
 * 10 is the safe number for the stack events, however, if application
 * also uses the queue for whatever purposes, this number should be increased.
 */
#define MAX_NUMBER_OF_EVENTS            10

/**
 * Maximum number of retries for CONFIRMED messages before giving up
 */
#define CONFIRMED_MSG_RETRY_COUNTER     3

/* ************************************************************************** */
/* Global Variables                                                           */
/* ************************************************************************** */
using namespace events;

/**
* This event queue is the global event queue for both the
* application and stack. To conserve memory, the stack is designed to run
* in the same thread as the application and the application is responsible for
* providing an event queue to the stack that will be used for ISR deferment as
* well as application information event queuing.
*/
static EventQueue ev_queue(MAX_NUMBER_OF_EVENTS *EVENTS_EVENT_SIZE);

/**
 * Event handler.
 *
 * This will be passed to the LoRaWAN stack to queue events for the
 * application which in turn drive the application.
 */
static void lora_event_handler(lorawan_event_t event);

/**
 * Constructing Mbed LoRaWANInterface and passing it the radio object from lora_radio_helper.
 */

static SX1276_LoRaRadio radio(MBED_CONF_APP_LORA_SPI_MOSI,
							   MBED_CONF_APP_LORA_SPI_MISO,
							   MBED_CONF_APP_LORA_SPI_SCLK,
							   MBED_CONF_APP_LORA_CS,
							   MBED_CONF_APP_LORA_RESET,
							   MBED_CONF_APP_LORA_DIO0,
							   MBED_CONF_APP_LORA_DIO1,
							   MBED_CONF_APP_LORA_DIO2,
							   MBED_CONF_APP_LORA_DIO3,
							   MBED_CONF_APP_LORA_DIO4,
							   MBED_CONF_APP_LORA_DIO5,
							   MBED_CONF_APP_LORA_RF_SWITCH_CTL1,
							   MBED_CONF_APP_LORA_RF_SWITCH_CTL2,
							   MBED_CONF_APP_LORA_TXCTL,
							   MBED_CONF_APP_LORA_RXCTL,
							   MBED_CONF_APP_LORA_ANT_SWITCH,
							   MBED_CONF_APP_LORA_PWR_AMP_CTL,
							   MBED_CONF_APP_LORA_TCXO);

static LoRaWANInterface lorawan(radio);

/**
 * Application specific callbacks
 */
static lorawan_app_callbacks_t callbacks;

static apdu_ctx_t ctx;



/* ************************************************************************** */
/* Public Functions                                                           */
/* ************************************************************************** */

#define ERROR_STATUS -1

FileHandle *mbed::mbed_override_console(int)
{
    static UARTSerial uart(MBED_CONF_TARGET_UART_TX, MBED_CONF_TARGET_UART_RX);
    return &uart;
}

int main(void)
{
	printf("lets go!\n");
	DigitalOut led(MBED_CONF_TARGET_LED,1);

	if(MBED_SUCCESS != connect(&ctx)) {
		printf("Can't connect to SE050 \r\n");
		return ERROR_STATUS;
	}


    // stores the status of a call to LoRaWAN protocol
    lorawan_status_t retcode;

    // Initialize LoRaWAN stack
    if (lorawan.initialize(&ev_queue) != LORAWAN_STATUS_OK) {
        printf("\r\n LoRa initialization failed! \r\n");
        return ERROR_STATUS;
    }

    printf("\r\n Mbed LoRaWANStack initialized \r\n");

    // prepare application callbacks
    callbacks.events = mbed::callback(lora_event_handler);
    lorawan.add_app_callbacks(&callbacks);

    // Set number of retries in case of CONFIRMED messages
    if (lorawan.set_confirmed_msg_retries(CONFIRMED_MSG_RETRY_COUNTER)
            != LORAWAN_STATUS_OK) {
        printf("\r\n set_confirmed_msg_retries failed! \r\n\r\n");
        return ERROR_STATUS;
    }

    printf("\r\n CONFIRMED message retries : %d \r\n",
           CONFIRMED_MSG_RETRY_COUNTER);

    // Enable adaptive data rate
    if (lorawan.enable_adaptive_datarate() != LORAWAN_STATUS_OK) {
        printf("\r\n enable_adaptive_datarate failed! \r\n");
        return ERROR_STATUS;
    }

    printf("\r\n Adaptive data  rate (ADR) - Enabled \r\n");

    retcode = lorawan.connect();

    if (retcode == LORAWAN_STATUS_OK ||
            retcode == LORAWAN_STATUS_CONNECT_IN_PROGRESS) {
    } else {
        printf("\r\n Connection error, code = %d \r\n", retcode);
        return ERROR_STATUS;
    }

    printf("\r\n Connection - In Progress ...\r\n");

    // make your event queue dispatching events forever
    ev_queue.dispatch_forever();
	
    printf("Leaving event dispatcher \r\n");
	return ERROR_STATUS;
}

/**
 * Sends a message to the Network Server
 */
static void send_message()
{
	uint16_t retcode;
	attestation_t attestation = {0};
	uint16_t temperature;
	uint8_t tx_buffer[256];
	uint8_t packet_len;

	
    printf("Read SE050 attested PmodTMP3 temperature\r\n");
    /* ------------------------------------------------------------------------------------------------------------------- */
    getTemp(&ctx, &temperature, &attestation);
	printf("Temperature: %x\n", temperature);
	printByteArray("ChipId", &attestation.chipId[0], 18);
	printByteArray("Random", &attestation.outrandom[0], 16);
	printByteArray("TimeStamp", &attestation.timeStamp[0], 12);
	printByteArray("Signature", &attestation.signature.p_data[0], attestation.signature.len);
	printByteArray("Data", &ctx.out.p_data[0], ctx.out.len);
	printf("SW = %x", ctx.sw);

	packet_len = 0;
	memcpy(&tx_buffer[packet_len], &attestation.data.p_data[0], attestation.data.len);
	packet_len += attestation.data.len;
	memcpy(&tx_buffer[packet_len], &attestation.timeStamp[0], 12);
	packet_len += 12;
	memcpy(&tx_buffer[packet_len], &attestation.outrandom[0], 16);
	packet_len += 16;
	memcpy(&tx_buffer[packet_len], &attestation.chipId[0], 18);
	packet_len += 18;
	memcpy(&tx_buffer[packet_len], &attestation.signature.p_data[0], attestation.signature.len);
	packet_len += attestation.signature.len;

    retcode = lorawan.send(MBED_CONF_LORA_APP_PORT, tx_buffer, packet_len,
                           MSG_UNCONFIRMED_FLAG);

    if (retcode < 0) {
        retcode == LORAWAN_STATUS_WOULD_BLOCK ? printf("send - WOULD BLOCK\r\n")
        : printf("\r\n send() - Error code %d \r\n", retcode);

        if (retcode == LORAWAN_STATUS_WOULD_BLOCK) {
            //retry in 3 seconds
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                ev_queue.call_in(3000, send_message);
            }
        }
        return;
    }
    printf("\r\n %d bytes scheduled for transmission \r\n", retcode);
    memset(tx_buffer, 0, sizeof(tx_buffer));
    return;

exit:
	printf("\r\n Transmission failed \r\n");
	memset(tx_buffer, 0, sizeof(tx_buffer));
	return;
}

/**
 * Receive a message from the Network Server
 */
static void receive_message()
{
	uint8_t rx_buffer[64];
    uint8_t port;
    int flags;
    int16_t retcode = lorawan.receive(rx_buffer, sizeof(rx_buffer), port, flags);

    if (retcode < 0) {
        printf("\r\n receive() - Error code %d \r\n", retcode);
        return;
    }

    printf(" RX Data on port %u (%d bytes): ", port, retcode);
    for (uint8_t i = 0; i < retcode; i++) {
        printf("%02x ", rx_buffer[i]);
    }
    printf("\r\n");
    
    memset(rx_buffer, 0, sizeof(rx_buffer));
}

/**
 * Event handler
 */
static void lora_event_handler(lorawan_event_t event)
{
    switch (event) {
        case CONNECTED:
            printf("\r\n Connection - Successful \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            } else {
                ev_queue.call_every(TX_TIMER, send_message);
            }

            break;
        case DISCONNECTED:
            ev_queue.break_dispatch();
            printf("\r\n Disconnected Successfully \r\n");
            break;
        case TX_DONE:
            printf("\r\n Message Sent to Network Server \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case TX_TIMEOUT:
        case TX_ERROR:
        case TX_CRYPTO_ERROR:
        case TX_SCHEDULING_ERROR:
            printf("\r\n Transmission Error - EventCode = %d \r\n", event);
            // try again
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        case RX_DONE:
            printf("\r\n Received message from Network Server \r\n");
            receive_message();
            break;
        case RX_TIMEOUT:
        case RX_ERROR:
            printf("\r\n Error in reception - Code = %d \r\n", event);
            break;
        case JOIN_FAILURE:
            printf("\r\n OTAA Failed - Check Keys \r\n");
            break;
        case UPLINK_REQUIRED:
            printf("\r\n Uplink required by NS \r\n");
            if (MBED_CONF_LORA_DUTY_CYCLE_ON) {
                send_message();
            }
            break;
        default:
            MBED_ASSERT("Unknown Event");
    }
}

// EOF
