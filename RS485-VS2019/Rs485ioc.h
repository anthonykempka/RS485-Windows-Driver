//-------------------------------------------------------------------------------------------------
// RS485IOC.H
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
// RS485NT driver IOCTL definitions. Shared between kernel mode device driver and user mode application
//
// Revision History:
// -----------------
// A. Kempka    08/20/1997   Original.
// A. Kempka    11/26/2011   Updated for WDK and PREfast
// A. Kempka    03/02/2022   Publish on GitHub and include BSD 3-Clause License
//  
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------
//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use by customers.
//

#define FILE_DEVICE_RS485DRV  0x00009000

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define RS485DRV_IOCTL_INDEX  0x900

//
// The RS485NT device driver IOCTLs
//

#define IOCTL_RS485NT_HELLO CTL_CODE(FILE_DEVICE_RS485DRV, RS485DRV_IOCTL_INDEX, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RS485NT_GET_RCV_COUNT CTL_CODE(FILE_DEVICE_RS485DRV, RS485DRV_IOCTL_INDEX+1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_RS485NT_LAST_RCVD_TIME CTL_CODE(FILE_DEVICE_RS485DRV, RS485DRV_IOCTL_INDEX+2, METHOD_BUFFERED, FILE_ANY_ACCESS)

