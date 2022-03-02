//-------------------------------------------------------------------------------------------------
// Q_TEST.C
//
// Author:  Anthony A. Kempka
// 
// BSD 3-Clause License
// 
// Copyright (c) 2022, Anthony Kempka
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
//
// Description:
// ------------
// This program will attempt to open a channel to the driver RS485NT and
// make sure iterative ReadFile() and WriteFile() calls.
//
// Environment:
// ------------
// This is a WIN32 console application that can be compiled for
//    a) x86 32-bit user mode
//    b) x64 64-bit user mode
//
// Launch a WDK build environment and use the following build command:
//    build -ceZ
//
// Revision History:
// -----------------
// A. Kempka    08/20/1997   Original (for Device Drivers International)
// A. Kempka    11/26/2011   Updated for WDK and updated drivers for 
//                           Windows XP, Vista and Windows 7 (x86 & x64)
// A. Kempka    03/02/2022   Publish on GitHub and include BSD 3-Clause License
//
//-------------------------------------------------------------------------------------------------

// System includes
#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <time.h>
#include <wincon.h>

// IOCTL definitions
#include <WinIoCtl.h>

// for all of the THCAR _t stuff (to allow compiling for both Unicode/Ansi)
#include <tchar.h>

// String safe functions
#include <Strsafe.h>

// Local includes
#include "RS485IOC.H"        // IOCTL codes

char sVersionString[] = "RS485NT Test Application built: " "("__DATE__", "__TIME__")";
char sCopyright[] = "Copyright © 2022 Angthony A. Kempka. All rights reserved.";

///////////////////////////////////////////////////////////////////////////////////////////////////
// DisplayErrors
//
// Prints the formatted text string for a given WINERROR error code
//
// Parameters
// ----------
// sErrorDescription    Used in printing the formatted error string
// dwError              Error code returned by GetLastError()
//
#define ERROR_BUFFER_SIZE    255
void DisplayErrors(_TCHAR *sErrorDescription, DWORD dwError)
{
    DWORD size = ERROR_BUFFER_SIZE;
    WCHAR buffer[ERROR_BUFFER_SIZE];
    
    LPCTSTR   lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dwError,
        0,                      // Default language
        (LPTSTR)&lpMsgBuf,      // buffer,
        0,                      //size 
        NULL);

    // Process any inserts in lpMsgBuf.
    // ...

    // Display the string.
    _tprintf(_T("%s Code(%u) %s\n"), sErrorDescription, dwError, lpMsgBuf);
    
    LocalFree((LPVOID)lpMsgBuf);
    return;
}

//---------------------------------------------------------------------------
//
// Main 
//
// Main processing loop.
// 
int __cdecl main ()
{
    ULONG   i;
    UCHAR   DummyBuffer[100];
    TCHAR   DriverName[] = _T("\\\\.\\RS485NT");
    HANDLE  DriverHandle;
    DWORD   BytesWritten, BytesRead;
    BOOL    status;

    _tprintf(_T("\nQ_Test RS485NT driver test application starting...\n"));
    // Note: string %S (upper case) if UNICODE, otherwise string %s (lower case)
    _tprintf(_T("Build Date: %S, Time: %S\nFile: %S\n\n"), __DATE__, __TIME__, __FILE__);

    //
    // Open a channel to the driver
    _tprintf(_T("Attempting to open device: %s\n"), DriverName);
    DriverHandle = CreateFile (DriverName, GENERIC_READ | GENERIC_WRITE,
                               0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    //
    // Do we have a valid handle? (If not, the driver probably isn't loaded)
    //
    if (DriverHandle == INVALID_HANDLE_VALUE) {
        DisplayErrors(_T("CreateFile failed!"), GetLastError());
        //
        // Exit -- ERROR
        //
        return (1);
    } else {
        _tprintf(_T("CreateFile success!\n\n"));
    }

    //
    // Simple WriteFile, ReadFile 
    //
    memcpy (DummyBuffer, "This is a test!", 15);

    status = WriteFile (DriverHandle, DummyBuffer, 15, &BytesWritten, 0);
    if (status) {
        _tprintf (_T("WriteFile: BytesWritten=%li\n"), BytesWritten);
    } else {
        _tprintf (_T("WriteFile: Error!\n"));
    }

    //
    // Wait for some receive response time
    //
    _tprintf (_T("Waiting for response...\n"));
    Sleep (1000);
    _tprintf (_T("Attempting to read RS-485 data.\n"));
    
    status = ReadFile (DriverHandle, DummyBuffer, 50, &BytesRead, 0);
    if (status) {
        _tprintf (_T("ReadFile: BytesRead=%li\n"), BytesRead);
    } else {
        _tprintf (_T("ReadFile: Error!\n"));
    }

    for (i=0; i<BytesRead; i++) {
        _tprintf (_T("%02X "), DummyBuffer[i]);
    }

    _tprintf (_T("\n"));

    _tprintf (_T("\nWrite and Read sequence complete!\n"));
    _tprintf (_T("Closing driver handle\n"));

    //
    // Close the channel to the driver
    //
    CloseHandle (DriverHandle);

    //
    // Exit -- OK
    //
    return (0);
}
