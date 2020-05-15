# Trusted Lorawan end-node

## Introduction

This repo is an example of a simple trusted Lorawan end-node sensor.
 
## Requirements

To execute this example, you need either a TSensor board or these pieces of hardware:

- a B-L072Z-LRWAN1 Board from ST.
- an OM-SE050ARD Arduino shield from NXP.
- a Pmod TMP3 sensor from digilent.

You also need to install [mbed-cli](https://os.mbed.com/docs/mbed-os/v5.15/tools/developing-mbed-cli.html) from ARM
and [ARM GCC 9](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm).
Once ARM GCC 9 is installed, you will have to follow instructions provided on this [web page](https://os.mbed.com/docs/mbed-os/v5.15/tools/manual-installation.html)
to properly configure mbed-cli.

## Importing and building the project

Once toolchain is installed, you will have to import and build the repo:
```bash
mbed import https://github.com/mimok/mbed-tsensor-lorawan
cd mbed-tsensor-lorawan
mbed compile -t GCC_ARM -m TSENSOR
```
If your are using the B-L072Z-LRWAN1 board and the SE050 Arduino shield, use the following command:
```bash
mbed import https://github.com/mimok/mbed-tsensor-lorawan
cd mbed-tsensor-lorawan
mbed compile -t GCC_ARM -m TSENSOR_DEV
```

Befor compiling you will have to configure your node address and keys in the `mbed_app.json` file:
```json
			"lora.device-eui": "{ YOUR_DEVICE_EUI }",
			"lora.application-eui": "{ YOUR_APPLICATION_EUI }",
			"lora.application-key": "{ YOUR_APPLICATION_KEY }",
```
There are others parameters that can be configured (e.g. your region, either EU868 or US915).
The full list of parameters are described [here](https://os.mbed.com/teams/mbed-os-examples/code/mbed-os-example-lorawan/).

## Preparing the hardware

If you are using the TSensor board, use female-male wire jumpers to connect SCL, SDA, GND and Vcc pins
from the Pmod module to the SCL, SDA, GND and 3v3 pin of the MIKROBUS connector.

:warning: **DO NOT connect Pmod Vcc pin to TSensor 5v pin**: Pmod must be supplied through 3v3 pin!

If you are using the B-L072Z-LRWAN1 board and the SE050 Arduino shield, plug the shield into the ST Microeletronics board
and use female-female wire jumpers to connect SCL, SDA, GND and Vcc pin from Pmod TMP3 module the corresponding pins 
of connector J11:
- SCL -> IO2
- SDA -> IO1
- Vcc -> VOUT
- GND -> Pin 7 of J11

Do not forget to connect your antenna.

## Setup your Lorawan network

To use this example, you need a Lorawan gateway (or use a public gateway) and you will have to
configure a Lorawan server. You can use the services provided by [The Things Network](https://www.thethingsnetwork.org/).
We will no further detail how to setup your TTN account, there is already a lot of tutorials available on internet.

## Loading the firmware

If you are using the TSensor board, use [STM32 Cube Programmer](https://www.st.com/en/development-tools/stm32cubeprog.html) to program the board thanks to the on-chip flash loader.
You will have to power up (or resetting) the board while pressing the BOOT0 button to allow the board to be detected by STM32 Cube Prog.

If you are using the B-L072Z-LRWAN1 board, copy/paste the generated `.bin` file into the virtual 
drive which appears when you plug the board to your computer using the on-board ST-LINK probe.

## Connecting to the console

If you are using the TSensor board, plug a USB/Serial adapter to the connector J1. If you are using the B-L072Z-LRWAN1 board you will use the ST-LINK virtual COM port.
Identify the right virtual COM port and connect to this port at 115200 baud.

If everything goes right, you should see something on the console which looks like the following log:
```
lets go!
ATR (len=35)
0000a000 00039604 03e800fe 020b03e8
08010000 00006400 000a4a43 4f503420
415450

Major: 03
Minor: 01
Patch: 00
Applet Config: 6fff
Secure Box: 010b

 Mbed LoRaWANStack initialized

 CONFIRMED message retries : 3

 Adaptive data  rate (ADR) - Enabled

 Connection - In Progress ...

 Connection - Successful
Read SE050 attested PmodTMP3 temperature
Temperature: 1c80
ChipId (len=18)
12040050 01dcfd2e 436e19ba 042c0219
5a00

Random (len=16)
10000102 03040506 0708090a 0b0c0d0e

TimeStamp (len=12)
0c000000 95000000 000001e4

Signature (len=72)
48304602 2100a9f5 09511dcd 7d5da48f
c9a0dc5a 300c9fff 4d8ee555 b7fd5d3f
9a46435e f26e0221 00bbaa31 0b72898a
c6905a0a 6359c51f 4c3a8745 4f1b3a4a
8c65284b 70b1894c

Data (len=150)
01418200 0c015a03 5a035a04 5a00021c
80438200 0c000000 95000000 000001e4
60448200 10000102 03040506 0708090a
0b0c0d0e 0f458200 12040050 01dcfd2e
436e19ba 042c0219 5a000046 82004830
46022100 a9f50951 1dcd7d5d a48fc9a0
dc5a300c 9fff4d8e e555b7fd 5d3f9a46
435ef26e 022100bb aa310b72 898ac690
5a0a6359 c51f4c3a 87454f1b 3a4a8c65
284b70b1 894c

SW = 9000
 130 bytes scheduled for transmission
```

## Checking your node traffic in the things network console

Connect to your [console in TTN](https://console.thethingsnetwork.org/applications) and select your application.
Click on de *data* tab to watch you node traffic.

# Root of Trust

## Check chain of certificates

First, use the `mbed-tsensor-vcom` application to retrieve the SE050 ECC certificate located by default at address
0xF0000013. Then download the root and intermediate certificates using links from [AN12436](https://www.nxp.com/docs/en/application-note/AN12436.pdf).
Finally, you can verify the validty of the certificate retrieve from the SE050 chip using the following command:

```bash
openssl verify -verbose -CAfile root.pem -untrusted intermediate.pem certificate.pem
```

## Check integrity of data received from the lorawan sensor
:warning: It is assumed that you are using services from *The Things Network*.
To check the integrity of the received data using the extracted certificate use the `ttn_console_example.py`
Python script located in the `scripts` folder of the `mbed-tsensor-lorawan` repository.
In this script, enter the right application ID and access key by modifying the two following lines (you will find these values in the TTN interface):

```python
app_id = "YOUR_APP_ID"
access_key = "YOUR_ACCES_KEY"
```
Plug your board and run the python. If everything goes right, you should see the following output in the python
console:

```
Received uplink from  board1
temperature: 26.0
valid signature
```