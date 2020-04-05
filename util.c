/*
 * util.c
 *
 *  Created on: 23 mars 2020
 *      Author: mimok
 */

#include <stdint.h>
#include <stdio.h>

void printByteArray(const char *arrayName, uint8_t *buffer, int len)
{
	printf("%s (len=%d)\n", arrayName, len);
	for(int i=1; i<=len; i++) {
		if(i%16==0)
			printf("\n");
		else if (i%4==0)
			printf(" ");
		printf("%02x",buffer[i]);
	}
	printf("\n");
}
