# IoT_led_strip
Internet of things RGB led strip with AVR (atmega328) and ESP8266
---

Assumes the ESP is running my transparent bridge firmware: https://github.com/beckdac/ESP8266-transparent-bridge

AVR 8Mhz internal oscillator, fuses: Low E2 High DC Ex 07
Set with
   lfuse=E2
   hfuse=DC
   efuse=07
   baud=1200
   sudo avrdude -c ft232u -p m328p -P ft0 -F -U lfuse:w:0x$lfuse:m -U hfuse:w:0x$hfuse:m -U efuse:w:0x$efuse:m -B $baud

Burn optiboot bootloader with:
   file=~/optiboot/optiboot/bootloaders/optiboot/optiboot_atmega328_pro_8MHz.hex
   sudo avrdude -c ft232u -p m328p -P ft0 -F -D -U flash:w:${file}:i DONE

Send application to chip:
   file=led_strip.hex
   ./reset_and_upload 192.168.1.23 led_strip.hex 
