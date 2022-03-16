# Windows RS485 device driver source code (Visual Studio 2019)

This driver is based on the original Windows NT / Windows 2000 driver [source code](https://github.com/anthonykempka/RS485-Windows-Driver/tree/main/original-NT-driver)

Some things to note:
- This driver is known as *legacy* and does not follow the standard W2K PnP, WDM, KMDF, etc. driver models.
- Locally test-signed versions of the driver .SYS and .INF files are provided
  - [Debug build](https://github.com/anthonykempka/RS485-Windows-Driver/tree/main/RS485-VS2019/Win32/Debug/RS485-VS2019)
  - [Release build](https://github.com/anthonykempka/RS485-Windows-Driver/tree/main/RS485-VS2019/Win32/Release/RS485-VS2019)
- Known issues
  - x64 is will not compile correctly and won't be fixed.




