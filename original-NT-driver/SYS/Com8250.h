//-------------------------------------------------------------------------------------------------
// COM8250.H
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
// 8250/16650 UART definitions
//
// Revision History:
// -----------------
// A. Kempka    08/20/1997   Original.
// A. Kempka    11/26/2011   Updated for WDK and PREfast
// A. Kempka    03/02/2022   Publish on GitHub and include BSD 3-Clause License
//  
//-------------------------------------------------------------------------------------------------

#ifndef _COMINC
#define _COMINC

//
// Macro definitions
//

#define COM1                    1       // COM1 PC serial port
#define COM2                    2       // COM2 PC serial port

#define COM1_BASE_ADDRESS       0x3F8   // Base address of the COM1 PC
#define COM2_BASE_ADDRESS       0x2F8   // Base address of the COM2 PC

#define RX_REGISTER_8250        0       // 8250 Receiver Buffer Register
#define TX_REGISTER_8250        0       // 8250 Transmit Buffer Register
#define IER_8250                1       // 8250 Interrupt Enable Register
#define IIR_8250                2       // 8250 Interrupt Id Register
#define LCR_8250                3       // 8250 Line Control Register
#define MCR_8250                4       // 8250 Modem Control Register
#define LSR_8250                5       // 8250 Line Status Register
#define MSR_8250                6       // 8250 Modem Status Register
#define DIVISOR_REGISTER_8250   0       // 8250 16-bit Baud Rate Divisor


//***************************************************************************
//
// Interrupt Enable Register -- IER defines
//

#define IER_ENABLE_RX_DATA_READY_IRQ    0x01    // Enables the Receiver Data
                                                // Ready interrupt when Or'd
                                                // with the IER

#define IER_DISABLE_RX_DATA_READY_IRQ   0xFE    // Disables the Receiver Data
                                                // Ready interrupt when
                                                // And'd with the IER

#define IER_ENABLE_TX_BE_IRQ            0x02    // Enables the Transmitter
                                                // Buffer Empty interrupt
                                                // when Or'd with the IER

#define IER_DISABLE_TX_BE_IRQ           0xFD    // Disables the Transmitter
                                                // Buffer Empty interrupt
                                                // when And'd with the IER

#define IER_ENABLE_RX_ERROR_IRQ         0x04    // Enables the Receiver Line
                                                // Status interrupt when
                                                // Or'd with the IER

#define IER_DISABLE_RX_ERROR_IRQ        0xFB    // Disables the Receiver Line
                                                // Status interrupt when
                                                // And'd with the IER

#define IER_ENABLE_MODEM_STATUS_IRQ     0x08    // Enables the MODEM Status
                                                // interrupt when Or'd with
                                                // the IER

#define IER_DISABLE_MODEM_STATUS_IRQ    0xF7    // Disables the MODEM Status
                                                // interrupt when And'd with
                                                // the IER


//***************************************************************************
//
// Interrupt ID Register -- IIR defines
//

#define IIR_INTERRUPT_MASK              0x07    // Masks off 3 interrupt pending
                                                // bits when And'd with the
                                                // IIR

#define IIR_NO_INTERRUPT_PENDING        0x01    // This defines the Interrupt
                                                // Identification register
                                                // pattern when no UART
                                                // interrupts are pending.

#define IIR_MODEM_STATUS_IRQ_PENDING    0x00    // Interrupt pending pattern for
                                                // MODEM Status interrupt.

#define IIR_TX_HBE_IRQ_PENDING          0x02    // Interrupt pending pattern for
                                                // Transmitter Buffer Empty

#define IIR_RX_DATA_READY_IRQ_PENDING   0x04    // Interrupt pending pattern for
                                                // Receiver Buffer Full

#define IIR_RX_ERROR_IRQ_PENDING        0x06    // Interrupt pending pattern for
                                                // Receiver Line Status


//***************************************************************************
//
// Line Control Register -- LCR defines
//

#define LCR_FIVE_BITS_PER_WORD          0x00    // Sets serial word length to 5
                                                // bits when Or'd with the LCR

#define LCR_SIX_BITS_PER_WORD           0x01    // Sets serial word length to 6
                                                // bits when Or'd with the LCR 

#define LCR_SEVEN_BITS_PER_WORD         0x02    // Sets serial word length to 7
                                                // bits when Or'd with the LCR 

#define LCR_EIGHT_BITS_PER_WORD         0x03    // Sets serial word length to 8
                                                // bits when Or'd with the LCR

#define LCR_ONE_STOP_BIT 0x00               // Sets the number of stop bits
                                            // to 1 when Or'd with the LCR

#define LCR_TWO_STOP_BITS 0x04              // Sets the number of stop bits
                                            // to 2 when Or'd with the LCR

#define LCR_NO_PARITY 0x00                  // Disables parity when Or'd
                                            // with the LCR

#define LCR_ODD_PARITY 0x08                 // Enables odd parity when Or'd
                                            // with the LCR

#define LCR_EVEN_PARITY 0x18                // Enables even parity when Or'd
                                            // with the LCR

#define LCR_MARK_PARITY 0x28                // Enables mark parity when Or'd
                                            // with the LCR

#define LCR_SPACE_PARITY 0x38               // Enables space parity when
                                            // Or'd with the LCR

#define LCR_DISABLE_BREAK 0xBF              // Disables the break condition
                                            // when And'd with the LCR

#define LCR_ENABLE_BREAK 0x40               // Enables the break condition
                                            // when Or'd with the LCR

#define LCR_DISABLE_DIVISOR_LATCH 0x7F      // Disables the divisor latch
                                            // when And'd with the LCR

#define LCR_ENABLE_DIVISOR_LATCH 0x80       // Enables the divisor latch
                                            // when Or'd with the LCR


//***************************************************************************
//
// Modem Control Register -- MCR defines
//

#define MCR_DEACTIVATE_ALL 0x00             // Deactivates all MCR outputs

#define MCR_ACTIVATE_DTR 0x01               // Activates the RS-232 DTR line
                                            // when Or'd with the MCR

#define MCR_DEACTIVATE_DTR 0xFE             // Deactivates the RS-232 DTR
                                            // line when And'd with the MCR

#define MCR_ACTIVATE_RTS 0x02               // Activates the RS-232 RTS line
                                            // when Or'd with the MCR

#define MCR_DEACTIVATE_RTS 0xFD             // Deactivates the RS-232 RTS
                                            // line when And'd with the MCR

#define MCR_ACTIVATE_GP01 0x04              // Activates the general purpose
                                            // output #1 when Or'd with MCR

#define MCR_DEACTIVATE_GP01 0xFB            // Deativates the general
                                            // purpose output #1 when
                                            // And'd with the MCR

#define MCR_ACTIVATE_GP02 0x08              // Activates the general purpose
                                            // output #2 when Or'd with MCR

#define MCR_DEACTIVATE_GP02 0xF7            // Deativates the general
                                            // purpose output #2 when
                                            // And'd with the MCR

#define MCR_ACTIVATE_LOOPBACK 0x10          // Activates the local loopback
                                            // test when Or'd with the MCR

#define MCR_DEACTIVATE_LOOPBACK 0xEF        // Deactivates the local
                                            // loopback test when And'd
                                            // with the MCR


//***************************************************************************
//
// Line Status Register -- LSR defines
//

#define LSR_RX_DATA_READY 0x01              // This bit signals a
                                            // character is ready

#define LSR_RX_OVERRUN_ERROR 0x02           // This bit signals a
                                            // receiver overrun error 

#define LSR_RX_PARITY_ERROR 0x04            // This bit signals a
                                            // parity error

#define LSR_RX_FRAMING_ERROR 0x08           // This bit signals a
                                            // framing error

#define LSR_RX_BREAK_DETECTED 0x10          // This bit signals a break char

#define LSR_TX_BUFFER_EMPTY 0x20            // This bit signals the transmit
                                            // buffer is empty

#define LSR_TX_BOTH_EMPTY 0x40              // This bit signals both the
                                            // transmit buffer and the shift
                                            // register are empty


//***************************************************************************
//
// Modem Status Register -- MSR defines
//

#define MSR_DELTA_CTS 0x01                  // Signals CTS has changed

#define MSR_DELTA_DSR 0x02                  // Signals DSR has changed

#define MSR_DELTA_RI 0x04                   // Signals RI has changed

#define MSR_DELTA_DCD 0x08                  // Signals CDC has changed

#define MSR_CURRENT_CTS 0x10                // Reads current CTS

#define MSR_CURRENT_DSR 0x20                // Reads current DSR

#define MSR_CURRENT_RI 0x40                 // Reads current RI

#define MSR_CURRENT_DCD 0x80                // Reads current DCD


//
// Communication system BAUD rate defines.
//

#define BAUD_RATE_DIVISOR_1200      0x60    // 1200 baud

#define BAUD_RATE_DIVISOR_2400      0x30    // 2400 baud

#define BAUD_RATE_DIVISOR_4800      0x18    // 4800 baud 

#define BAUD_RATE_DIVISOR_9600      0x0C    // 9600 baud 

#define BAUD_RATE_DIVISOR_19200     0x06    // 19200 baud

#define BAUD_RATE_DIVISOR_38400     0x03    // 38400 baud

#define BAUD_RATE_DIVISOR_57600     0x02    // 57600 baud

#define BAUD_RATE_DIVISOR_115200    0x01    // 152000 baud

//
// Communication port type definition
//

typedef struct _COMPORT {
    PUCHAR RBR;     // Receiver Buffer reg.
    PUCHAR TBR;     // Transmitter Buffer reg.
    PUCHAR IER;     // Interrupt Enable reg.
    PUCHAR IIR;     // Interrupt ID reg.
    PUCHAR LCR;     // Line Control register.
    PUCHAR MCR;     // Modem Control register.
    PUCHAR LSR;     // Line Status register.
    PUCHAR MSR;     // Modem Status register.
    PUCHAR BAUD;    // Baud Rate Divisor.
} COMPORT, *PCOM_PORT;

#endif // _COMINC

