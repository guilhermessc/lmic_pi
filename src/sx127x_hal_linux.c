/*
 * Copyright (c) 2014-2016 IBM Corporation.
 * All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of the <organization> nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _POSIX_C_SOURCE 199309L

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>

#include "sx127x_hal.h"
#include "sx127x.h"

// Using BCM_GPIO
const struct lmic_pinmap pins = {
	.nss = 25,
	.rxtx = 24, // Not connected on RFM92/RFM95
	.rst = 17,  // Needed on RFM92/RFM95
	.dio = {4,23,24},
};

int fd;

// -----------------------------------------------------------------------------
// I/O

void hal_io_init (void) {
	wiringPiSetup();
	knot_hal_gpio_setup();
	knot_hal_gpio_pin_mode(pins.nss, OUTPUT);
	knot_hal_gpio_pin_mode(pins.rxtx, OUTPUT);
	knot_hal_gpio_pin_mode(pins.rst, OUTPUT);
	knot_hal_gpio_pin_mode(pins.dio[0], INPUT);
	knot_hal_gpio_pin_mode(pins.dio[1], INPUT);
	knot_hal_gpio_pin_mode(pins.dio[2], INPUT);
}

// val == 1  => tx 1
void hal_pin_rxtx (uint8_t val) {
	printf("*** debbug hal_pin_rxtx\n");
	knot_hal_gpio_digital_write(pins.rxtx, val);
	printf("*** debbug hal_pin_rxtx DONE!\n");
}

// set radio RST pin to given value (or keep floating!)
void hal_pin_rst (uint8_t val) {
	printf("*** debbug hal_pin_rst\n");
	if(val == 0 || val == 1) { // drive pin
		knot_hal_gpio_pin_mode(pins.rst, OUTPUT);
		knot_hal_gpio_digital_write(pins.rst, val);
	//knot_hal_gpio_digital_write(0, val==0?LOW:HIGH);
	} else { // keep pin floating
		knot_hal_gpio_pin_mode(pins.rst, INPUT);
	}
	printf("*** debbug hal_pin_rst DONE!\n");
}

static bool dio_states[3] = {0};

void hal_io_check(void) {
	printf("HAL_IO_CHECK\n");
	uint8_t i;
	for (i = 0; i < 3; ++i) {
		if (dio_states[i] != knot_hal_gpio_digital_read(pins.dio[i])) {
			dio_states[i] = !dio_states[i];
			if (dio_states[i]) {
				//radio_irq_handler(i);
			}
		}
	}
}

// -----------------------------------------------------------------------------
// SPI
//
static int spifd;

static void hal_spi_init (void) {
	// spifd = wiringPiSPISetup(0, 10000000);
	spifd = spi_init("/dev/spidev0.0");
}

void hal_pin_nss (uint8_t val) {
	knot_hal_gpio_digital_write(pins.nss, val);
}

// perform SPI transaction with radio
uint8_t hal_spi (uint8_t out) {
	// uint8_t res = wiringPiSPIDataRW(0, &out, 1);
	int res = spi_transfer(spifd, NULL, 0, &out, 1);
	return out;
}


// -----------------------------------------------------------------------------
// TIME

struct timespec tstart={0,0};
static void hal_time_init () {
	int res=clock_gettime(CLOCK_MONOTONIC_RAW, &tstart);
	tstart.tv_nsec=0; //Makes difference calculations in hal_ticks() easier
}

uint32_t hal_ticks (void) {
	// LMIC requires ticks to be 15.5μs - 100 μs long
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	ts.tv_sec-=tstart.tv_sec;
	uint64_t ticks=ts.tv_sec*(1000000/US_PER_OSTICK)+ts.tv_nsec/
							(1000*US_PER_OSTICK);
//    fprintf(stderr, "%d hal_ticks()=%d\n", sizeof(time_t), ticks);
	return (uint32_t)ticks;
}

// Returns the number of ticks until time.
static uint32_t delta_time(uint32_t time) {
	uint32_t t = hal_ticks( );
	int32_t d = time - t;
	//fprintf(stderr, "deltatime(%d)=%d (%d)\n", time, d, t);
	if (d<=5) {
		return 0;
	} else {
		return (uint32_t)d;
	}
}

void hal_waitUntil (uint32_t time) {
	uint32_t now=hal_ticks();
	uint32_t delta = delta_time(time);
	//fprintf(stderr, "waitUntil(%d) delta=%d\n", time, delta);
	int32_t t=time-now;
	if (delta==0)
	return;
	if (t>0) {
		//fprintf(stderr, "delay(%d)\n", t*US_PER_OSTICK/1000);
		delay(t*US_PER_OSTICK/1000);
		return;
	}
}

// check and rewind for target time
uint8_t hal_checkTimer (uint32_t time) {
	// No need to schedule wakeup, since we're not sleeping
	//fprintf(stderr, "hal_checkTimer(%d):%d (%d)\n", time,
					//delta_time(time), hal_ticks());
	return delta_time(time) <= 0;
}

static uint64_t irqlevel = 0;

void IRQ0(void) {
	//  fprintf(stderr, "IRQ0 %d\n", irqlevel);
	if (irqlevel==0) {
		printf("IRQO\n");
		//radio_irq_handler(0);
		return;
	}
}

void IRQ1(void) {
	if (irqlevel==0){
	printf("IRQ1\n");
	//radio_irq_handler(1);
	}
}

void IRQ2(void) {
	if (irqlevel==0){
		printf("IRQ2\n");
		//radio_irq_handler(2);
	}
}

void hal_disableIRQs (void) {
//    cli();
	irqlevel++;
//    fprintf(stderr, "disableIRQs(%d)\n", irqlevel);
}

void hal_enableIRQs (void) {
	if(--irqlevel == 0) {
//		fprintf(stderr, "enableIRQs(%d)\n", irqlevel);
//		sei();

		// Instead of using proper interrupts (which are a bit tricky
		// and/or not available on all pins on AVR), just poll the pin
		// values. Since os_runloop disables and re-enables interrupts,
		// putting this here makes sure we check at least once every
		// loop.
		//
		// As an additional bonus, this prevents the can of worms that
		// we would otherwise get for running SPI transfers inside ISRs
		//hal_io_check();
	}
}

void hal_sleep (void) {
	// Not implemented
}

void hal_failed (void) {
	//fprintf(stderr, "FAILURE\n");
	//fprintf(stderr, "%s:%d\n",file, line);
	hal_disableIRQs();
	while(1);
}

void hal_init(void) {
	fd=wiringPiSetup();
	hal_io_init();
	// configure radio SPI
	hal_spi_init();
	// configure timer and interrupt handler
	hal_time_init();
	/*
	wiringPiISR(pins.dio[0], INT_EDGE_RISING, IRQ0);
	wiringPiISR(pins.dio[1], INT_EDGE_RISING, IRQ1);
	wiringPiISR(pins.dio[2], INT_EDGE_RISING, IRQ2);
	*/
}