# RS485-Windows-Driver
RS485 device driver source code and signed driver for Windows operating systenms.The current version is compatible for Windows 7 and later. The driver uses the common 16650 UART interface. 


## RTS Control
This driver is designed to use the PC UART RTS output signal to "enable" an RS485 multi-drop interface transceiver (aka RTS Control). Whenever data needs to be sent across the RS485 bus, RTS is enabled and puts the RS485 into transmit mode, then data is send. When the last bit of the last byte clears the UART, RTS is disable the RS485 interface chip returns to receive mode.

The diagram below depicts the RTS enable during UART data transmission on the RS485 multi-drop bus.

![image](https://user-images.githubusercontent.com/16089554/156234873-a982bdc0-ce50-4eb6-9ef3-6cf5db8eb4ce.png)


