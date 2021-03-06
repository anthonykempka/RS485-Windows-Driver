# RS485-Windows-Driver
This repository contains custom RS-485 device driver source code and signed driver binaries for Microsoft Windows operating systems. The driver uses the common IBM PC 8250/16650 UART interface which is still found in many PC computers.

The provenance of this device driver harkens back to the late 1990's when I co-founded Device Drivers International. Originally written for Windows NT, this driver has been upated several times for Windows 2000, Windows XP and Server 2003, Windows 7+ and finally when it was converted to use Visual Studio 2019.


## RTS Control
This driver is designed to use the PC UART RTS output signal to "enable" an RS485 multi-drop interface transceiver (aka RTS Control). Whenever data is sent across the RS-485 bus, RTS is enabled and puts the RS-485 into transmit mode, then serial data is sent. When the last bit of the last byte clears the UART, RTS is disable the RS485 interface chip returns to receive mode.

The diagram below depicts the RTS enable during UART data transmission on the RS485 multi-drop bus.

![image](https://user-images.githubusercontent.com/16089554/156234873-a982bdc0-ce50-4eb6-9ef3-6cf5db8eb4ce.png)

## RS-232 to RS-485 Protocol Converter
The driver is designed to use a RS-232 to RS-485 protocol converter such as the one shown below. 

![RS232toRS485-DB9](https://user-images.githubusercontent.com/16089554/156939157-3182213c-da1c-4242-a00f-3ac0fcb0e8ab.png)

