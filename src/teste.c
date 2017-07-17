#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "sx127x.h"
#include "sx127x_hal.h"

#define TX_INTERVAL 2000

	uint8_t buffer_tx[50] = {0};
	uint8_t buffer_rx[50] = {0};
	size_t len_tx, len_rx;

static void txfunc (const char *str){

	len_tx = 0;
	while (*str){
		buffer_tx[len_tx++] = *str++;
	}

	printf("ENVIANDO\n");
	radio_tx(buffer_tx, len_tx);
}

static void rxfunc (){

	len_rx = 0;
	for (int i = 0; i<50; i++){
		buffer_rx[i] = 0;
	}

	LMIC.rxtime = os_getTime();
	printf("RECEBENDO\n");

	radio_rx(RXMODE_SCAN);
	sleep(3);

	radio_irq_handler(0, buffer_rx, &len_rx);

	printf("%s\n", buffer_rx);
}

int main (){

	printf("Iniciando\n");

	LMIC.freq	= 902300000;
	LMIC.txpow	= 27;

	LMIC.sf		= SF9;
	LMIC.bw		= BW125;
	LMIC.cr		= 0;
	LMIC.ih		= 0;
	LMIC.noCRC	= 1;

	hal_init();
printf("*********** hal_init done! ***********\n");

	radio_init();
	printf("Iniciado\n");

	printf("SF:%d; BW: %d; CR: %d; Ih: %d; \n", LMIC.sf, LMIC.bw, LMIC.cr, LMIC.ih);

	int i;
	while(1){

		printf("Enviar - 1 \nReceber - 2\n");
		scanf("%d", &i);

		if (i == 1){
			txfunc("Hey, ta chegando!!");
		}
		if (i == 2){
			rxfunc();
		}
	}

	return 0;
}
