### Application-specific constants

APP_NAME := teste

### Constant symbols

CC := $(CROSS_COMPILE)gcc
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

$(APP_NAME): $(OBJDIR)/$(APP_NAME).o $(OBJDIR)/gpio_sysfs.o $(OBJDIR)/sx127x_hal_linux.o $(OBJDIR)/sx127x.o
	$(CC) $< $ $(OBJDIR)/gpio_sysfs.o $(OBJDIR)/sx127x_hal_linux.o $(OBJDIR)/sx127x.o -lwiringPi -o $@

### EOF
