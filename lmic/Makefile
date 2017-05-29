CC=g++

DEPS=config.h hal.h lmic.h local_hal.h lorabase.h oslmic.h ../spi/spi.h
OBJ=aes.o hal.o lmic.o oslmic.o radio.o ../spi/spi_linux.c

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: $(OBJ)

.PHONY: clean

clean:
	rm *.o
