## lcd_kernel

This code acts as a simple driver for the Optrex LCD running on the phytec board.  It runs using GPIO pins on the board, but the board can easily be swapped if the gpio pins are changed.


In order to run:

type make (makefile runs)

a kernel object file will be created (lcd.ko) as well as a lot of other different files. Upload the .ko file to the board

To run the module on the board, type:

insmod lcd.ko

If initialized correctly the LCD screen will have a blinking cursor!