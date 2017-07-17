/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include "gpio_sysfs.h"

#define BUFFER_MAX 3
#define DIRECTION_MAX 35
#define VALUE_MAX 30
#define HIGHEST_GPIO 28

#define NOT_INITIALIZED 0
#define INITIALIZED_INPUT 1
#define INITIALIZED_OUTPUT 2
#define INITIALIZED 3

static uint8_t initialized_gpio[HIGHEST_GPIO];

static int GPIOExport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open export for writing!\n");
		return -1;
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return 0;
}

static int GPIOUnexport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open unexport for writing!\n");
		return -1;
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return 0;
}

static int GPIODirection(int pin, int dir)
{
	static const char s_directions_str[]  = "in\0out";

	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio %d direction for writing!\n", pin);
		return -1;
	}

	if (-1 == write(fd, &s_directions_str[INPUT == dir ? 0 : 3],
		INPUT == dir ? 2 : 3)) {
		fprintf(stderr, "Failed to set direction!\n");
		return -1;
	}

	close(fd);
	return 0;
}

static int GPIORead(int pin)
{
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio %d value for reading!\n", pin);
		return -1;
	}

	if (-1 == read(fd, value_str, 3)) {
		fprintf(stderr, "Failed to read value!\n");
		return -1;
	}

	close(fd);

	return atoi(value_str);
}

static int GPIOWrite(int pin, int value)
{
	static const char s_values_str[] = "01";

	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		fprintf(stderr, "Failed to open gpio %d value for writing!\n", pin);
		return -1;
	}

	if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
		fprintf(stderr, "Failed to write value!\n");
		return -1;
	}

	close(fd);
	return 0;
}

int knot_hal_gpio_setup(void)
{
	/*
	 * It's wrong to call the set up multiple times this static varaible
	 * avoids unexpected behaviour caused by such mistakes
	 */
printf("go go go\n");
	static int c = 0;
	for (; c<HIGHEST_GPIO; ++c)
		initialized_gpio[c] = NOT_INITIALIZED;
	return 0;
}

void knot_hal_gpio_unmap(void)
{
	int c;
	for (c=0; c<HIGHEST_GPIO; ++c) {
		if (initialized_gpio[c] != NOT_INITIALIZED) {
			GPIOUnexport(c);
			initialized_gpio[c] = NOT_INITIALIZED;
		}
	}
}

int knot_hal_gpio_pin_mode(uint8_t gpio, uint8_t mode)
{
	printf("-> knot_hal_gpio_pin_mode         %d\t\t%d\n", gpio, mode);
	if (gpio > HIGHEST_GPIO) {
		fprintf(stderr, "Cannot initialize gpio: maximum number exceeded\n");
		return -1;
	}

	// if(initialized_gpio[gpio-1] != NOT_INITIALIZED){
	// 	printf("reseting gpio %d\n", gpio);
	// 	if (GPIOUnexport(gpio) == 0)
	// 		initialized_gpio[gpio-1] = NOT_INITIALIZED;
	// }

	// if (GPIOExport((int) gpio)==0 && GPIODirection((int) gpio, (int) mode)==0){
	// 	if (mode == INPUT){
	// 		initialized_gpio[gpio-1] = INITIALIZED_INPUT;
	// 	} else
	// 		initialized_gpio[gpio-1] = INITIALIZED_OUTPUT;
	// } else {
	// 	fprintf(stderr, "Cannot initialize gpio: GPIOExport failed\n");
	// 	return knot_hal_gpio_pin_mode(gpio, mode);
	// 	// return -1;
	// }

	if(initialized_gpio[gpio-1] == NOT_INITIALIZED){
		GPIOExport((int) gpio);
		initialized_gpio[gpio-1] = INITIALIZED;
	}

	if (GPIODirection((int) gpio, (int) mode)==0) {
		if (mode == INPUT){
			initialized_gpio[gpio-1] = INITIALIZED_INPUT;
		} else
			initialized_gpio[gpio-1] = INITIALIZED_OUTPUT;
	} else {
		printf("falha 2\n");
		return -1;
	}

	return 0;
}

void knot_hal_gpio_digital_write(uint8_t gpio, uint8_t value)
{
	if (initialized_gpio[gpio-1] == INITIALIZED_OUTPUT)
		GPIOWrite(gpio, value);
	else{
		fprintf(stderr, "Cannot write: gpio %d not initialized as OUTPUT\n", gpio);
		
		printf("Changing mode and writing\n");
		knot_hal_gpio_pin_mode(gpio, OUTPUT);
		knot_hal_gpio_digital_write(gpio, value);
	}
}

int knot_hal_gpio_digital_read(uint8_t gpio)
{
	if (initialized_gpio[gpio-1] == INITIALIZED_INPUT)
		return (GPIORead((int) gpio) == LOW ? LOW : HIGH);
	fprintf(stderr, "Cannot write: gpio %d not initialized as INPUT\n", gpio);
	printf("Changing mode and reading\n");
	knot_hal_gpio_pin_mode(gpio, INPUT);
	return knot_hal_gpio_digital_read(gpio);
}

int knot_hal_gpio_analog_read(uint8_t gpio)
{
	return 0;
}

void knot_hal_gpio_analog_reference(uint8_t mode)
{

}

void knot_hal_gpio_analog_write(uint8_t gpio, int value)
{

}