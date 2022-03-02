//-------------------------------------------------------------------------------------------------
// RS485NT.H
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
// Revision History:
// -----------------
// A. Kempka    08/20/1997   Original.
// A. Kempka    11/26/2011   Updated for WDK and PREfast
// A. Kempka    03/02/2022   Publish on GitHub and include BSD 3-Clause License
//  
//-------------------------------------------------------------------------------------------------

//
// Defaults (COM2)
//
#define DEF_PORT_ADDRESS    0x2F8
#define DEF_PORT_RANGE      0x07
#define DEF_IRQ_LINE        0x03
#define DEF_BAUD_RATE       19200
#define DEF_BUFFER_SIZE     2048

//---------------------------------------------------------------------------
//
// *Note*
// ALL variables and data storage should be placed in the following data structure.
//

typedef struct _RS485NT_DEVICE_EXTENSION {
    PDEVICE_OBJECT  DeviceObject;
    PKINTERRUPT     InterruptObject;
    KIRQL           Irql;
    ULONG           InterruptCount;
    ULONG           RcvError;
    LARGE_INTEGER   LastQuerySystemTime;
    ULONG           ioCtlCode;
    PUCHAR          PortAddress;
    KIRQL           IRQLine;
    ULONG           BaudRate;
    COMPORT         ComPort;
    KEVENT          XmitDone;
    ULONG           BufferSize;
    PUCHAR          XmitBuffer;
    PUCHAR          XmitBufferPosition;
    PUCHAR          XmitBufferEnd;
    ULONG           XmitBufferCount;
    PUCHAR          RcvBuffer;
    PUCHAR          RcvBufferPosition;
    PUCHAR          RcvBufferEnd;
    ULONG           RcvBufferCount;
} RS485NT_DEVICE_EXTENSION, *PRS485NT_DEVICE_EXTENSION;

// ExAllocatePoolWithTag() memory tag definition
#define MEMORY_TAG  '584R'