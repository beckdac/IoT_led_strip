# IoT_led_strip
Internet of things RGB led strip with AVR (atmega328) and ESP8266
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
   make atmega328_384_8

Only use ft232u if you have the FTDI cable configured as I do.  You probably want ft232r.  Here is the config for the ft232u which goes in your avrdude.conf (assumes avrdude 6.1):

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
Set with
   lfuse=E2
   hfuse=DC
   efuse=07
   baud=1200
   sudo avrdude -c ft232u -p m328p -P ft0 -F -U lfuse:w:0x$lfuse:m -U hfuse:w:0x$hfuse:m -U efuse:w:0x$efuse:m -B $baud

Burn optiboot bootloader with:
   file=~/optiboot/optiboot/bootloaders/optiboot/optiboot_atmega328_384_8.hex
   sudo avrdude -c ft232u -p m328p -P ft0 -F -e -U flash:w:${file}:i DONE

Send application to chip:
   ip=192.168.1.23 # replace with your own
   file=led_strip.hex
   ./reset_and_upload $ip led_strip.hex 
