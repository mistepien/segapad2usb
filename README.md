![alt text](https://github.com/mistepien/segapad2usb/blob/main/top.svg)
![alt text](https://github.com/mistepien/segapad2usb/blob/main/bottom.svg)

# segapad2usb
Sega MD/Genesis gampepad adapter to USB.

<a href="https://github.com/mistepien/segapad2usb/blob/main/segapad2usb.pdf">Adapter</a> is actually a shield for 
<a href="https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/hardware-overview-pro-micro">Arduino Pro Micro based on ATmega 32U4</a>.

ATmega 32U4 has a hardware support for fullspeed USB, thus <a href="https://wiki.archlinux.org/title/mouse_polling_rate">polling rate 1000Hz</a> is not an issue for that chip.

You can use the <a href="https://github.com/mistepien/segapad2usb/blob/main/gerber.zip">gerber file<a> to order the PCB.

Attached <a href="https://github.com/mistepien/joy2usb/tree/main/firmware">code</a> uses port registers so that is quite efficient. The code is based on <a href="https://github.com/jonthysell/SegaController">SegaController library</a>. That library was changed to use port registers and support HOME button. The code was tested with <a href="https://www.8bitdo.com/m30-2-4g/">M30 2.4G for SEGA Genesis & Mega Drive</a> and <a href="https://retro-bit.com/sega-genesis-6-button-arcade-pad-black.html">SEGA Genesis 6-button Arcade Pad</a>. <a href="https://www.8bitdo.com/m30-2-4g/">M30 2.4G for SEGA Genesis & Mega Drive</a> has a HOME button and it is fully supported by the code.

One footprint is not trivial: DB9 male socket (<b>J1</b>). Officialy the <b>J1</b> footprint (segapad2usb has been designed in <a href="https://www.kicad.org/">KiCad</a>) is "DSUB-9_Male_Horizontal_P2.77x2.84mm_EdgePinOffset9.90mm_Housed_MountingHolesOffset11.32mm" however it was chosen to fit <a href="https://www.tme.eu/pl/en/details/ld09p13a4gx00lf/d-sub-plugs-and-sockets/amphenol-communications-solutions/">LD09P13A4GX00LF Amphenol</a>.

<b>J2</b> is redundant -- you can use it to add some new features like autofire switch or sth.

Sometimes during uploading code to Arduino board you may face some issues -- then you will need to use RESET -- so <b>SW1</b> can be useful if you like to develope your own code.

LED D1 shows whether controller is working in 6-button mode. LED D2 shows is Sega MD/Genesis gampepad connected at all.

BOM:
| Qty	| Reference(s) | Description |
|-----|--------------|-------------|
|2 | D1, D2 | LED 3mm |  
|1 |	J1|	DB9 Male Connector <a href="https://www.tme.eu/pl/en/details/ld09p13a4gx00lf/d-sub-plugs-and-sockets/amphenol-communications-solutions/">LD09P13A4GX00LF Amphenol</a> |
|1 |	J2|	2x4 Pinheader. pitch terminal: 2.54mm |
|1 | R1, R2 | resistors for D1 and D2 (R1 for D1, R2 for D2) |
|1 | SW1 | Tact switch from Arduino breadboard projects |
|1 | U1 |	DIP20 socket (width 15.24mm) + <a href="https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/hardware-overview-pro-micro">Arduino Pro Micro</a> |



