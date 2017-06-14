### Application-specific constants

APP_NAME := teste

### Constant symbols

CC := $(CROSS_COMPILE)g++
AR := $(CROSS_COMPILE)ar

CFLAGS := -O2 -Wall -Wextra -std=c99 -lrt -Iinc -I.

OBJDIR = obj
INCLUDES = $(wildcard inc/*.h)
OBJ = $(wildcard obj/*.o)

### General build targets

all: $(APP_NAME)

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(APP_NAME)

### Sub-modules compilation

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: src/%.c $(INCLUDES) | $(OBJDIR)
	$(CC) -c $(CFLAGS) $< -o $@


### Main program assembly

$(APP_NAME): $(OBJDIR)/gpio_sysfs.o $(OBJDIR)/$(APP_NAME).o $(OBJDIR)/spi_linux.o $(OBJDIR)/aes.o $(OBJDIR)/hal.o $(OBJDIR)/lmic.o $(OBJDIR)/oslmic.o $(OBJDIR)/radio.o
	$(CC) $< $(OBJDIR)/gpio_sysfs.o $(OBJDIR)/spi_linux.o $(OBJDIR)/aes.o $(OBJDIR)/hal.o $(OBJDIR)/lmic.o $(OBJDIR)/oslmic.o $(OBJDIR)/radio.o -lwiringPi -o $@

### EOF
