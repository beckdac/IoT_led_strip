# IoT_led_strip
Internet of things RGB led strip with AVR (atmega328) and ESP8266
---

Support commands
```
	RESET		resets the device
	FACTORY		installs a basic 43 color rainbow progression program
	STOP		stop executing the current program
	PROGRAM		enter programming mode
	RUN			begin executing the current program
	LENGTH <steps>
				stores the total number of steps in a new program to the EEPROM
				the device must be in PROGRAM mode
	STEP <step> <red> <green> <blue> <delay in ms>
				stores the color and time delay in milliseconds for a given
				step to the EEPROM
	RGB	<red> <green> <blue>
				stop the current program and display the specified color
  RED <red>
        stop the current program and change the red channel PWM
  GREEN <green>
        stop the current program and change the green channel PWM
  BLUE <blue>
        stop the current program and change the blue channel PWM
	OFF			stop the current program and disable the LED strip
	LHZ			display the current light to frequench value in Hz
	LHZEN		set the threshold value for the program to begin execution
				in HZ; the LHZ must be below this number to execute a program
	DUMP		output the current program in PROGRAM mode statements
	STATUS		output the values of status registers and other settings
```

---

Assumes the ESP is running my transparent bridge firmware: https://github.com/beckdac/ESP8266-transparent-bridge

Burn an optiboot for 38400 baud (which works with internal oscillator).  Put this in your Makefile for optiboot in similar sections (from http://forum.arduino.cc/index.php?topic=124879.0):
```
atmega328_384_8.name=ATmega328 Optiboot @ 38,400baud w/ 8MHz Int. RC Osc.

atmega328_384_8.upload.protocol=arduino
atmega328_384_8.upload.maximum_size=30720
atmega328_384_8.upload.speed=38400

atmega328_384_8.bootloader.low_fuses=0xE2
atmega328_384_8.bootloader.high_fuses=0xDE
atmega328_384_8.bootloader.extended_fuses=0x05
atmega328_384_8.bootloader.path=optiboot
atmega328_384_8.bootloader.file=optiboot_atmega328_384_8.hex
atmega328_384_8.bootloader.unlock_bits=0x3F
atmega328_384_8.bootloader.lock_bits=0x0F

atmega328_384_8.build.mcu=atmega328p
atmega328_384_8.build.f_cpu=8000000L
atmega328_384_8.build.core=arduino
atmega328_384_8.build.variant=standard
```

Then run
```
   make atmega328_384_8
```

Only use ft232u in the below avrdude examples for fuses and bootloader burning if you have the FTDI cable configured as I do (derived from this many year old post http://exmrclean.blogspot.com/2009/05/burning-avr-boot-loader-with-usb-ttl.html).  You probably want ft232r.  Here is the config for the ft232u which goes in your avrdude.conf (assumes avrdude 6.1) if you are still using the old school ft232u pin configuration:

```
programmer
  id    = "ft232u";
  desc  = "FT232R USB Cable Synchronous BitBang";
  type  = "ftdi_syncbb";
  connection_type = usb;
  miso  = 1;  # RxD
  sck   = 2;  # RTS
  mosi  = 0;  # TxD
  reset = 3;  # DTR
;
```

If your avrdude has the broken ftdi_syncbb (won't chip erase), apply the patch from this post:
https://lists.nongnu.org/archive/html/avrdude-dev/2013-09/msg00160.html

AVR 8Mhz internal oscillator, fuses: Low E2 High DC Ex 07
Set with:
```
   lfuse=E2
   hfuse=DC
   efuse=07
   baud=1200
   sudo avrdude -c ft232u -p m328p -P ft0 -F -U lfuse:w:0x$lfuse:m -U hfuse:w:0x$hfuse:m -U efuse:w:0x$efuse:m -B $baud
```

Burn optiboot bootloader with:
```
   file=~/optiboot/optiboot/bootloaders/optiboot/optiboot_atmega328_384_8.hex
   sudo avrdude -c ft232u -p m328p -P ft0 -F -e -U flash:w:${file}:i DONE
```

Send application to chip over wifi+ESP8266:
```
   ip=192.168.1.23 # replace with your own
   file=led_strip.hex
   ./reset_and_upload $ip led_strip.hex 
```

Example interaction:
```
telnet led_strip
Trying 192.168.1.23...
Connected to led_strip.lan.
Escape character is '^]'.

PROGRAM
OK
LENGTH 4
program steps = 4
OK
STEP 0 255 0 0 1000
program step  = 0
program rgb   = 255	0	0
program delay = 1000
OK
STEP 1 200 200 200 1000
program step  = 1
program rgb   = 200	200	200
program delay = 1000
OK
STEP 2 0 0 255 1000
program step  = 2
program rgb   = 0	0	255
program delay = 1000
OK
STEP 3 0 0 0 1000
program step  = 3
program rgb   = 0	0	0
program delay = 1000
OK
RUN
OK
DUMP

PROGRAM
LENGTH 4
STEP	0	255	0	0	1000
STEP	1	200	200	200	1000
STEP	2	0	0	255	1000
STEP	3	0	0	0	1000
RUN

OK
STATUS
state: run
command overflows: 0
RX buffer overflows: 0
TX buffer overflows: 0
light frequency:	15
light frequency enable threshold:	100
OK
^]
telnet> c
Connection closed.
```


---

The *programs* directory contains partially formatted files for upload.  They do not contain the LENGTH header element.  The shell script *program* (relies on awk and expect) will take one the the *.prg* files in *programs*, add the LENGTH and send it over the wifi to an led_strip host (name should be supplied).  E.g.
```
./program led_strip.lan programs/winter.prg
```

If you want to slow down a program or just use linear interprolation between colors to smooth color transitions, there is the *interpolate_program* bash script (also relies on awk) that will do just that.  Takes the name of a program and the number of interpolated points per step transition to insert.  E.g. to add 4 transitions between each color for winter.prg
```
./interpolate_program programs/winter.prg 4
```

Which would produce something like:

```
STEP	29	141	180	750
STEP	58	143	182	750
STEP	87	145	185	750
STEP	116	147	187	750
STEP	146	150	190	750
STEP	151	157	192	750
STEP	156	165	195	750
STEP	161	172	197	750
STEP	166	180	200	750
STEP	149	182	197	750
STEP	133	185	195	750
STEP	116	187	192	750
STEP	100	190	190	750
STEP	85	167	202	750
STEP	70	145	215	750
STEP	55	122	227	750
STEP	40	100	240	750
```

The intended use is in the following pattern:

```
./interpolate_program programs/winter.prg 10 > /tmp/a
./program led_strip.lan /tmp/a
```

