# Makefile for GPIO User Space

CC=arm-linux-gnueabihf-gcc
SOURCES= lcd.c
EXECUTABLE = lcd_test 

lcd_test: lcd_test.c
	$(CC) -std=gnu99 -g -o $@ $^ 

.PHONY: clean

clean:
	rm -f *.o *~ $(EXECUTABLE)

