//---------------------------------------------------------------------------
// RS485NT.C
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
// Description:
// ------------
// This file was developed for Integrity Instruments as a Generic
// Windows NT RS485 driver. This is a Kernel-Mode driver.
//
//
// Application Interface:
// ----------------------
// CreateFile () - Establishes an open channel to this driver
// WriteFile ()  - Transmits a buffer of Data via RS485 by asserting
//                 RTS during trasnmit and deasserting RTS upon
//                 transmitt complete of the final character
//
//                 *CAUTION* WriteFile discards unread receive buffer contents!
//
// ReadFile ()   - Returns the current received character buffer
//
// See the sample User mode API in Q_TEST.C
//
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//
// Include files 
//
#include "NTDDK.H"
#include "COM8250.H"
#include "RS485NT.H"
#include "RS485IOC.H"

//-------------------------------------------------------------------------------------------------
//
// Define the driver names
// 
#define NT_DEVICE_NAME	    L"\\Device\\RS485NT"
#define DOS_DEVICE_NAME     L"\\DosDevices\\RS485NT"

#define RS_DbgPrint(a) DbgPrint(a)

char sVersionString[] = "RS485NT Device Driver built: " "("__DATE__", "__TIME__")";
char sCopyright[] = "Copyright © 2022 Anthony A. Kempka. All rights reserved.";

//-------------------------------------------------------------------------------------------------
//
// Declare forward function references and PREfast labels
//

DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
__drv_dispatchType(IRP_MJ_READ)
__drv_dispatchType(IRP_MJ_WRITE)
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH  DispatchRoutine;

DRIVER_UNLOAD  UnloadDriver;

KSERVICE_ROUTINE RS485_Isr;

IO_DPC_ROUTINE RS485_Dpc_Routine;

NTSTATUS GetConfiguration (IN PRS485NT_DEVICE_EXTENSION DeviceExtension,
                           IN PUNICODE_STRING RegistryPath);

NTSTATUS Initialize_RS485 (IN PRS485NT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS RS485_Write (IN PRS485NT_DEVICE_EXTENSION  deviceExtension, IN PIRP Irp);
NTSTATUS RS485_Read (IN PRS485NT_DEVICE_EXTENSION  deviceExtension, IN PIRP Irp);

BOOLEAN ReportUsage (IN PDRIVER_OBJECT DriverObject,
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PHYSICAL_ADDRESS PortAddress,
                     IN BOOLEAN *ConflictDetected);



//---------------------------------------------------------------------------
//
//  Begin FUNCTIONS
//

//---------------------------------------------------------------------------
// DriverEntry
//
// Description:
//  NT device Driver Entry point
//
// Arguments:
//      DriverObject    - Pointer to this device's driver object
//      RegistryPath    - Pointer to the Unicode regsitry path name
//
// Return Value:
//      NTSTATUS
//
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status, ioConnectStatus;
    UNICODE_STRING uniNtNameString;
    UNICODE_STRING uniWin32NameString;
    KIRQL irql = DEF_IRQ_LINE;
    KAFFINITY Affinity;
    ULONG MappedVector, AddressSpace = 1;
    PRS485NT_DEVICE_EXTENSION extension;
    BOOLEAN ResourceConflict;
    PHYSICAL_ADDRESS InPortAddr, OutPortAddr;

    RS_DbgPrint ("RS485NT: Enter the driver!\n");

    //
    // Create counted string version of our device name.
    //
    RtlInitUnicodeString(&uniNtNameString, NT_DEVICE_NAME);

    //
    // Create the device object, single-thread access (TRUE)
    //
    status = IoCreateDevice(DriverObject, sizeof(RS485NT_DEVICE_EXTENSION),
                            &uniNtNameString, FILE_DEVICE_UNKNOWN, 0,
                            TRUE, &deviceObject);

    if (!NT_SUCCESS (status) ) {
        RS_DbgPrint("RS485NT: IoCreateDevice failed\n");
        return status;
    }
    //
    // Set the FLAGS field
    //
    deviceObject->Flags |= DO_BUFFERED_IO;

    //
    // Get the configuration information from the Registry
    //
    status = GetConfiguration (deviceObject->DeviceExtension, RegistryPath);

    if (!NT_SUCCESS (status) ) {
        RS_DbgPrint("RS485NT: GetConfiguration failed\n");
        return status;
    }

    extension = (PRS485NT_DEVICE_EXTENSION) deviceObject->DeviceExtension;

    //
    // This call will map our IRQ to a system vector. It will also fill
    // in the IRQL (the kernel-defined level at which our ISR will run),
    // and affinity mask (which processors our ISR can run on).
    //
    // We need to do this so that when we connect to the interrupt, we
    // can supply the kernel with this information.
    //
    MappedVector = HalGetInterruptVector(
            Isa,        // Interface type
            0,          // Bus number
            extension->IRQLine,
            extension->IRQLine,
            &irql,      // IRQ level
            &Affinity   // Affinity mask
            );

    //
    // A little known Windows NT fact,
    // If MappedVector==0, then HalGetInterruptVector failed.
    //
    if (MappedVector == 0) {
        RS_DbgPrint("RS485NT: HalGetInterruptVector failed\n");
        return (STATUS_INVALID_PARAMETER);
    }

    //
    // Save off the Irql
    //
    extension->Irql = irql;

    //
    // Translate the base port address to a system mapped address.
    // This will be saved in the device extension after IoCreateDevice,
    // because we use the translated port address to access the ports.
    //
    InPortAddr.LowPart = (ULONG)extension->PortAddress;
    InPortAddr.HighPart = 0;
    if (!HalTranslateBusAddress(Isa, 0, InPortAddr, &AddressSpace, 
                                &OutPortAddr)) {
        RS_DbgPrint("RS485NT: HalTranslateBusAddress failed\n");
        return STATUS_SOME_NOT_MAPPED;
    }


    if ( NT_SUCCESS(status) ) {
        //
        // Create dispatch points for create/open, close, unload, and ioctl
        //
        DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchRoutine;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchRoutine;
        DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRoutine;
        DriverObject->MajorFunction[IRP_MJ_WRITE] = DispatchRoutine;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchRoutine;
        DriverObject->DriverUnload = UnloadDriver;

        //
        // check if resources (ports and interrupt) are available.
        // 
        //    extern PUCHAR *KdComPortInUse;
        //    KdComPhysical = MmGetPhysicalAddress(*KdComPortInUse);
        //    if ((*KdComPortInUse) == (ULongToPtr(OutPortAddr))
        //       {{ // comport in user by debugger }}
        // 
        // NOTE: This is LEGACY and is no longer used. 
        //       But the check above for DEBUGGER conflict should be done.
        /* 
        ReportUsage (DriverObject, deviceObject, OutPortAddr, &ResourceConflict);

        if (ResourceConflict) {
            RS_DbgPrint("RS485NT: Couldn't get resources\n");
            IoDeleteDevice(deviceObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        */
        
        //
        // fill in the device extension
        //
        extension = (PRS485NT_DEVICE_EXTENSION) deviceObject->DeviceExtension;
        extension->DeviceObject = deviceObject;
        extension->PortAddress = (PVOID)OutPortAddr.LowPart;

        //
        // connect the device driver to the IRQ
        //
        ioConnectStatus = IoConnectInterrupt(&extension->InterruptObject,
                                             RS485_Isr,
                                             extension->DeviceObject,
                                             NULL,
                                             MappedVector,
                                             irql,
                                             irql,
                                             Latched,
                                             FALSE,
                                             Affinity,
                                             FALSE);

        if ( !NT_SUCCESS (ioConnectStatus) ) {
            RS_DbgPrint("RS485NT: Couldn't connect interrupt\n");
            IoDeleteDevice(deviceObject);
            return ioConnectStatus;
        }

        RS_DbgPrint("RS485NT: just about ready!\n");

        //
        // Create counted string version of our Win32 device name.
        //
        RtlInitUnicodeString( &uniWin32NameString, DOS_DEVICE_NAME);
    
        //
        // Create a link from our device name to a name in the Win32 namespace.
        //
        status = IoCreateSymbolicLink( &uniWin32NameString, &uniNtNameString );

        if (!NT_SUCCESS(status)) {
            RS_DbgPrint("RS485NT: Couldn't create the symbolic link\n");
            IoDeleteDevice (DriverObject->DeviceObject);
        } else {

            //
            // Setup the Dpc for ISR routine
            //
            IoInitializeDpcRequest (DriverObject->DeviceObject, RS485_Dpc_Routine);

            //
            // Initialize the device (enable IRQ's, hit the hardware)
            //
            Initialize_RS485 (extension);

            RS_DbgPrint("RS485NT: All initialized!\n");
        }

    } else {
        RS_DbgPrint("RS485NT: Couldn't create the device\n");
    }
    return status;
}


//---------------------------------------------------------------------------
// RS485_Isr
//
// Description:
//  This is our 'C' Isr routine to handle RS485 transmit and recieve
//
// Arguments:
//      Interrupt   - Pointer to our interrupt object
//      Context     - Pointer to our device object
//
// Return Value:
//      TRUE        - If this was our ISR (assumed)
//
BOOLEAN RS485_Isr (IN PKINTERRUPT Interrupt, IN OUT PVOID Context)
{
    PDEVICE_OBJECT DeviceObject;
    PRS485NT_DEVICE_EXTENSION DeviceExtension;
    UCHAR   ch;

    //
    // Get the Device Object and obtain our Extension
    //
    DeviceObject = Context;
    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Bump the interrupt count
    //
    DeviceExtension->InterruptCount++;
    
    RS_DbgPrint ("RS485NT: ISR!\n");

    //
    // For the 8250 series UART, we must spin and handle ALL interrupts
    // before returning
    //
    ch = READ_PORT_UCHAR (DeviceExtension->ComPort.IIR);

    while ((ch & IIR_INTERRUPT_MASK) != IIR_NO_INTERRUPT_PENDING) {
        switch (ch & IIR_INTERRUPT_MASK) {

            case IIR_RX_ERROR_IRQ_PENDING:      // 1st priority interrupt
                RS_DbgPrint ("RS485NT: ISR RX Error!\n");
                ch = READ_PORT_UCHAR (DeviceExtension->ComPort.LSR);
                DeviceExtension->RcvError++;
                break;

            case IIR_RX_DATA_READY_IRQ_PENDING: // 2nd priority int

                RS_DbgPrint ("RS485NT: ISR RX Data!\n");

                //
                // Read the UART receive register and stuff byte into buffer
                //
                ch = READ_PORT_UCHAR (DeviceExtension->ComPort.RBR);

                //
                // Check for end of buffer
                //
                if (DeviceExtension->RcvBufferPosition+1 < 
                    DeviceExtension->RcvBufferEnd) {

                   *DeviceExtension->RcvBufferPosition = ch;
                    DeviceExtension->RcvBufferPosition++;
                    DeviceExtension->RcvBufferCount++;
                }

                //
                // Get the current system time
                //
                KeQuerySystemTime (&DeviceExtension->LastQuerySystemTime);

                break;
            
            case IIR_TX_HBE_IRQ_PENDING:        // 3rd priority interrupt

                RS_DbgPrint ("RS485NT: ISR TX Data!\n");

                //
                // Is this the last byte sent?
                //
                if (DeviceExtension->XmitBufferCount == 0) {

                    //
                    // Wait for the entire character to be sent out the UART
                    //
                    ch = READ_PORT_UCHAR (DeviceExtension->ComPort.LSR);

                    while ( (ch & LSR_TX_BOTH_EMPTY) != LSR_TX_BOTH_EMPTY) {
                        ch = READ_PORT_UCHAR (DeviceExtension->ComPort.LSR);
                    }

                    //
                    // De-assert RTS
                    //
                    ch = READ_PORT_UCHAR (DeviceExtension->ComPort.MCR) & 
                         MCR_DEACTIVATE_RTS;
                    WRITE_PORT_UCHAR (DeviceExtension->ComPort.MCR, ch);

                    //
                    // Clear the Rcv buffer info, a reply is emminent
                    //
                    DeviceExtension->RcvBufferCount = 0;
                    DeviceExtension->RcvBufferPosition = DeviceExtension->RcvBuffer;

                    //
                    // Schedule the DPC (where the Xmit done event is set)
                    //
                    IoRequestDpc (DeviceObject, DeviceObject->CurrentIrp, NULL);

                } else {

                    //
                    // Send the next byte
                    //

                    WRITE_PORT_UCHAR (DeviceExtension->ComPort.TBR,
                                      *DeviceExtension->XmitBufferPosition);

                    DeviceExtension->XmitBufferPosition++;
                    DeviceExtension->XmitBufferCount--;
                }

                //
                // Get the current system time
                //
                KeQuerySystemTime (&DeviceExtension->LastQuerySystemTime);

                break;

            case IIR_MODEM_STATUS_IRQ_PENDING:  // 4th priority interrupt
                RS_DbgPrint ("RS485NT: ISR Modem Status!\n");
                ch = READ_PORT_UCHAR (DeviceExtension->ComPort.MSR);
                break;

            default:
                break;
        }
        ch = READ_PORT_UCHAR (DeviceExtension->ComPort.IIR);    // Read the IIR again for the next loop
    }

    //
    // Return TRUE to signify this was our interrupt and we serviced it.
    //
    return TRUE;
}


//---------------------------------------------------------------------------
// RS485_Dpc_Routine
//
// Description:
//  This DPC for ISR is issued by RS485_Isr to complete Transmit processing
//  by setting the XmitDone event.
//
// Arguments:
//      Dpc             - not used
//      DeviceObject    - Pointer to the Device object
//      Irp             - not used
//      Context         - not used
//
// Return Value:
//      none
//
VOID RS485_Dpc_Routine (IN PKDPC Dpc, IN PDEVICE_OBJECT DeviceObject, 
                        IN PIRP Irp, IN PVOID Context)
{
    PRS485NT_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    //
    // Set the Xmit Done event
    //
    KeSetEvent (&DeviceExtension->XmitDone, 0, FALSE);

    RS_DbgPrint ("RS485NT: Dpc Routine KeSetEvent\n");
    return;
}


//---------------------------------------------------------------------------
// ReportUsage
//
// Description:
//  This routine registers (reports) the I/O and IRQ usage for this driver.
//
// Arguments:
//      DriverObject    - Pointer to the driver object
//      DeviceObject    - Pointer to the Device object
//      PortAddress     - Address of I/O port used
//      ConflictDetected - TRUE if a resource conflict was detected.
//
// Return Value:
//      TRUE    - If a Resource conflict was detected
//      FALSE   - If no conflict was detected
//
BOOLEAN ReportUsage(IN PDRIVER_OBJECT DriverObject,
                    IN PDEVICE_OBJECT DeviceObject,
                    IN PHYSICAL_ADDRESS PortAddress,
                    IN BOOLEAN *ConflictDetected)
{
    PRS485NT_DEVICE_EXTENSION extension;

    ULONG sizeOfResourceList;
    PCM_RESOURCE_LIST resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR nextFrd;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;

    extension = (PRS485NT_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    //
    // The size of the resource list is going to be one full descriptor
    // which already has one partial descriptor included, plus another
    // partial descriptor. One partial descriptor will be for the
    // interrupt, and the other for the port addresses.
    //
    sizeOfResourceList = sizeof(CM_FULL_RESOURCE_DESCRIPTOR);

    //
    // The full resource descriptor already contains one
    // partial.	Make room for one more.
    //
    // It will hold the irq "prd", and the port "prd".
    //    ("prd" = partial resource descriptor)
    //
    sizeOfResourceList += sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    //
    // Now we increment the length of the resource list by field offset
    // of the first frd.   This will give us the length of what preceeds
    // the first frd in the resource list.
    //   (frd = full resource descriptor)
    //
    sizeOfResourceList += FIELD_OFFSET(CM_RESOURCE_LIST, List[0]);

    resourceList = ExAllocatePoolWithTag(PagedPool, sizeOfResourceList, MEMORY_TAG);

    if (!resourceList) {
        return FALSE;
    }

    //
    // Zero out the list
    //
    RtlZeroMemory(resourceList, sizeOfResourceList);

    resourceList->Count = 1;
    nextFrd = &resourceList->List[0];

    nextFrd->InterfaceType = Isa;
    nextFrd->BusNumber = 0;

    //
    // We are going to report port addresses and interrupt
    //
    nextFrd->PartialResourceList.Count = 2;

    //
    // Now fill in the port data.  We don't wish to share
    // this port range with anyone.
    //
    // Note: the port address we pass in is the one we got
    // back from HalTranslateBusAddress.
    //
    partial = &nextFrd->PartialResourceList.PartialDescriptors[0];

    partial->Type = CmResourceTypePort;
    partial->ShareDisposition = CmResourceShareDriverExclusive;
    partial->Flags = CM_RESOURCE_PORT_IO;
    partial->u.Port.Start = PortAddress;
    partial->u.Port.Length = DEF_PORT_RANGE;

    partial++;

    //
    // Now fill in the irq stuff.
    //
    // Note: for IoReportResourceUsage, the Interrupt.Level and
    // Interrupt.Vector are bus-specific level and vector, just
    // as we passed in to HalGetInterruptVector, not the mapped
    // system vector we got back from HalGetInterruptVector.
    //
    partial->Type = CmResourceTypeInterrupt;
    partial->u.Interrupt.Level = extension->IRQLine;
    partial->u.Interrupt.Vector = extension->IRQLine;
    partial->ShareDisposition = CmResourceShareDriverExclusive;
    partial->Flags = CM_RESOURCE_INTERRUPT_LATCHED;

    IoReportResourceForDetection (
        DriverObject,
        resourceList,
        sizeOfResourceList,
        NULL,
        NULL,
        0,
        ConflictDetected);

    //
    // The above routine sets the BOOLEAN parameter ConflictDetected
    // to TRUE if a conflict was detected.
    //
    ExFreePool(resourceList);

    return (*ConflictDetected);
}


//---------------------------------------------------------------------------
//
//
// Routine Description:
// 
//     Process the IRPs sent to this device.
// 
// Arguments:
// 
//     DeviceObject - pointer to a device object
// 
//     Irp          - pointer to an I/O Request Packet
// 
// Return Value:
// 
// 
NTSTATUS DispatchRoutine (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION  irpStack;
    PRS485NT_DEVICE_EXTENSION   deviceExtension;
    PVOID               ioBuffer;
    ULONG               inputBufferLength;
    ULONG               outputBufferLength;
    ULONG               ioControlCode;
    NTSTATUS            ntStatus;

    LARGE_INTEGER       CurrentSystemTime;
    LARGE_INTEGER       ElapsedTime;
    
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // Get a pointer to the current location in the Irp. This is where
    //     the function codes and parameters are located.
    //

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //
    // Get a pointer to the device extension
    //

    deviceExtension = DeviceObject->DeviceExtension;

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuffer           = Irp->AssociatedIrp.SystemBuffer;
    inputBufferLength  = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (irpStack->MajorFunction) {
        case IRP_MJ_CREATE:
        {    
            RS_DbgPrint ("RS485NT: IRP_MJ_CREATE\n");
            break;
        }

        case IRP_MJ_CLOSE:
        {
            RS_DbgPrint ("RS485NT: IRP_MJ_CLOSE\n");
            break;
        }

        case IRP_MJ_READ:
        {
            RS_DbgPrint ("RS485NT: IRP_MJ_READ\n");
            RS485_Read (deviceExtension, Irp);
            break;
        }

        case IRP_MJ_WRITE:
        {
            RS_DbgPrint ("RS485NT: IRP_MJ_WRITE\n");
            RS485_Write (deviceExtension, Irp);
            break;
        }

        case IRP_MJ_DEVICE_CONTROL:
        {
            RS_DbgPrint ("RS485NT: IRP_MJ_DEVICE_CONTROL - ");
    
            ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    
            switch (ioControlCode)
            {
                case IOCTL_RS485NT_HELLO:
                {
                    RS_DbgPrint ("HELLO\n");
                            
                    //
                    // Some app is saying hello
                    //

                    break;
                }

                case IOCTL_RS485NT_GET_RCV_COUNT:
                {
                    RS_DbgPrint ("GET_RCV_COUNT\n");
                    if (outputBufferLength >= 4) {
                        //
                        // Return the current receive buffer count
                        //

                        *(ULONG *)ioBuffer = deviceExtension->RcvBufferCount;

                        Irp->IoStatus.Information = 4;
                    }
                    break;
                }

                case IOCTL_RS485NT_LAST_RCVD_TIME:
                {
                    RS_DbgPrint ("LAST_RCVD_TIME\n");
                    if (outputBufferLength >= 8) {

                        //
                        // Get the current system time and convert to Milliseconds
                        //

                        KeQuerySystemTime (&CurrentSystemTime);
                        ElapsedTime.QuadPart = CurrentSystemTime.QuadPart - 
                                               deviceExtension->LastQuerySystemTime.QuadPart;
                        ElapsedTime.QuadPart /= 10000;

                        RtlMoveMemory (ioBuffer, &ElapsedTime, 8);
                        Irp->IoStatus.Information = 8;
                    }

                    break;
                }

                default:
                {
                    RS_DbgPrint ("RS485NT: Unknown IRP_MJ_DEVICE_CONTROL\n");
                    Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    break;
                }
    
            }
    
            break;
        }
        default:
        {
            RS_DbgPrint ("RS485NT: Unhandled IRP_MJ function\n");
            Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
    }

    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    ntStatus = Irp->IoStatus.Status;

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    RS_DbgPrint ("RS485NT: DisptachRoutine exit.\n");

    //
    // We never have pending operation so always return the status code.
    //

    return ntStatus;
}


//---------------------------------------------------------------------------
// UnloadDriver
//
// Description:
//     Free all the allocated resources, etc.
//
// Arguments:
//     DriverObject - pointer to a driver object
// 
// Return Value:
//      None
// 
VOID UnloadDriver (IN PDRIVER_OBJECT DriverObject)
{
    WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\RS485NT";
    UNICODE_STRING         deviceLinkUnicodeString;
    PRS485NT_DEVICE_EXTENSION extension;

    extension = DriverObject->DeviceObject->DeviceExtension;

    //
    // Deactivate all of the MCR interrupt sources.
    //
    WRITE_PORT_UCHAR (extension->ComPort.MCR, MCR_DEACTIVATE_ALL);

    //
    // Free any resources
    //
    IoDisconnectInterrupt (extension->InterruptObject);

    //
    // Delete the symbolic link
    //
    RtlInitUnicodeString (&deviceLinkUnicodeString, deviceLinkBuffer);

    IoDeleteSymbolicLink (&deviceLinkUnicodeString);

    //
    // Delete the device object
    //
    IoDeleteDevice (DriverObject->DeviceObject);

    RS_DbgPrint ("RS485NT: Unloaded\n");
    return;
}


//---------------------------------------------------------------------------
//  GetConfiguration
//
// Description:
//      Obtains driver configuration information from the Registry.
//
// Arguments:
//      DeviceExtension - Pointer to the device extension.
//      RegistryPath    - Pointer to the null-terminated Unicode name of the
//                        registry path for this driver.
//
// Return Value:
//      NTSTATUS
// 
NTSTATUS GetConfiguration(IN PRS485NT_DEVICE_EXTENSION DeviceExtension,
                          IN PUNICODE_STRING RegistryPath)
{
    PRTL_QUERY_REGISTRY_TABLE parameters = NULL;
    UNICODE_STRING parametersPath;

    ULONG notThereDefault = 1234567;
    ULONG PortAddressDefault;
    ULONG IRQLineDefault;
    ULONG BaudRateDefault;
    ULONG BufferSizeDefault;

    NTSTATUS status = STATUS_SUCCESS;
    PWSTR path = NULL;
    USHORT queriesPlusOne = 5;

    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it.
    //
    path = RegistryPath->Buffer;

    //
    // Allocate the Rtl query table.
    //
    parameters = ExAllocatePoolWithTag (PagedPool, (sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne), MEMORY_TAG);

    if (!parameters) {
        RS_DbgPrint("RS485NT: ExAllocatePool failed for Rtl in GetConfiguration\n");
        status = STATUS_UNSUCCESSFUL;
    } else {

        RtlZeroMemory(parameters, sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlusOne);

        //
        // Form a path to this driver's Parameters subkey.
        //
        RtlInitUnicodeString(&parametersPath, NULL);

        parametersPath.MaximumLength = RegistryPath->Length + sizeof(L"\\Parameters");

        parametersPath.Buffer = ExAllocatePoolWithTag (PagedPool, parametersPath.MaximumLength, MEMORY_TAG);

        if (!parametersPath.Buffer) {
            RS_DbgPrint("RS485NT: ExAllocatePool failed for Path in GetConfiguration\n");
            status = STATUS_UNSUCCESSFUL;
        }
    }

    if (NT_SUCCESS(status)) {

        //
        // Form the parameters path.
        //
        RtlZeroMemory(parametersPath.Buffer, parametersPath.MaximumLength);
        RtlAppendUnicodeToString(&parametersPath, path);
        RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

        //
        // Gather all of the "user specified" information from
        // the registry.
        //
        parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[0].Name = L"Port Address";
        parameters[0].EntryContext =  &PortAddressDefault;
        parameters[0].DefaultType = REG_DWORD;
        parameters[0].DefaultData = &notThereDefault;
        parameters[0].DefaultLength = sizeof(ULONG);

        parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[1].Name = L"IRQ Line";
        parameters[1].EntryContext = &IRQLineDefault;
        parameters[1].DefaultType = REG_DWORD;
        parameters[1].DefaultData = &notThereDefault;
        parameters[1].DefaultLength = sizeof(ULONG);

        parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[2].Name = L"Baud Rate";
        parameters[2].EntryContext = &BaudRateDefault;
        parameters[2].DefaultType = REG_DWORD;
        parameters[2].DefaultData = &notThereDefault;
        parameters[2].DefaultLength = sizeof(ULONG);

        parameters[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
        parameters[3].Name = L"Buffer Size";
        parameters[3].EntryContext = &BufferSizeDefault;
        parameters[3].DefaultType = REG_DWORD;
        parameters[3].DefaultData = &notThereDefault;
        parameters[3].DefaultLength = sizeof(ULONG);

        status = RtlQueryRegistryValues(
                     RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                     parametersPath.Buffer,
                     parameters,
                     NULL,
                     NULL);

        if (!NT_SUCCESS(status)) {
            RS_DbgPrint("RS485NT: RtlQueryRegistryValues failed\n");
        }

        status = STATUS_SUCCESS;
    }

    //
    // Go ahead and assign driver defaults.
    //
    if (PortAddressDefault == notThereDefault) {
        DeviceExtension->PortAddress = (PUCHAR) DEF_PORT_ADDRESS;
    } else {
        DeviceExtension->PortAddress = (PVOID) PortAddressDefault;
    }

    if (IRQLineDefault == notThereDefault) {
        DeviceExtension->IRQLine = DEF_IRQ_LINE;
    } else {
        DeviceExtension->IRQLine = (KIRQL) IRQLineDefault;
    }

    if (BaudRateDefault == notThereDefault) {
        DeviceExtension->BaudRate = DEF_BAUD_RATE;
    } else {
        DeviceExtension->BaudRate = BaudRateDefault;
    }

    if (BufferSizeDefault == notThereDefault) {
        DeviceExtension->BufferSize = DEF_BUFFER_SIZE;
    } else {
        DeviceExtension->BufferSize = BufferSizeDefault;
    }

    //
    // Free the allocated memory before returning.
    //

    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);
    return (status);
}


//---------------------------------------------------------------------------
// Initialize_RS485
//
// Description:
//  Initializes all data structures and hardware necessary for
//  driver execution
//
// Arguments:
//      DeviceExtension - Pointer to the device extension.
//
// Return Value:
//      NSTATUS
//  
NTSTATUS Initialize_RS485 (IN PRS485NT_DEVICE_EXTENSION DeviceExtension)
{
    UCHAR       ch, Divisor;
    NTSTATUS    status = STATUS_SUCCESS;

    //
    // Initialize all of the 8250 register addresses
    //
    DeviceExtension->ComPort.RBR = DeviceExtension->PortAddress + RX_REGISTER_8250;
    DeviceExtension->ComPort.TBR = DeviceExtension->PortAddress + TX_REGISTER_8250;
    DeviceExtension->ComPort.IER = DeviceExtension->PortAddress + IER_8250;
    DeviceExtension->ComPort.IIR = DeviceExtension->PortAddress + IIR_8250;
    DeviceExtension->ComPort.LCR = DeviceExtension->PortAddress + LCR_8250;
    DeviceExtension->ComPort.MCR = DeviceExtension->PortAddress + MCR_8250;
    DeviceExtension->ComPort.LSR = DeviceExtension->PortAddress + LSR_8250;
    DeviceExtension->ComPort.MSR = DeviceExtension->PortAddress + MSR_8250;
    DeviceExtension->ComPort.BAUD = DeviceExtension->PortAddress + DIVISOR_REGISTER_8250;

    //
    // Initialize any Events
    //
    KeInitializeEvent (&DeviceExtension->XmitDone, SynchronizationEvent, FALSE);

    //
    // Allocate memory for the Transmit and Receive data buffers
    //
    DeviceExtension->RcvBuffer = ExAllocatePoolWithTag (NonPagedPool, DeviceExtension->BufferSize, MEMORY_TAG);

    if (DeviceExtension->RcvBuffer == NULL) {
        RS_DbgPrint("RS485NT: ExAllocatePool failed for RcvBuffer\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        //
        // Setup buffer pointers and counts
        //

        DeviceExtension->RcvBufferPosition = DeviceExtension->RcvBuffer;
        DeviceExtension->RcvBufferEnd = DeviceExtension->RcvBuffer + 
                                        (DeviceExtension->BufferSize - 1);
        DeviceExtension->RcvBufferCount = 0;

    }

    if (NT_SUCCESS(status)) {
        DeviceExtension->XmitBuffer = ExAllocatePoolWithTag (NonPagedPool, DeviceExtension->BufferSize, MEMORY_TAG);
        if (DeviceExtension->RcvBuffer == NULL) {
            RS_DbgPrint("RS485NT: ExAllocatePool failed for XmitBuffer\n");
            status = STATUS_INSUFFICIENT_RESOURCES;
        } else {

            //
            // Setup buffer pointers and counts
            //
            DeviceExtension->XmitBufferPosition = DeviceExtension->XmitBuffer;
            DeviceExtension->XmitBufferEnd = DeviceExtension->XmitBuffer + 
                                            (DeviceExtension->BufferSize - 1);
            DeviceExtension->XmitBufferCount = 0;
        }
    }

    //
    // Clear the interrupt/error Count and get current system time
    //
    DeviceExtension->InterruptCount = 0;
    DeviceExtension->RcvError = 0;
    KeQuerySystemTime (&DeviceExtension->LastQuerySystemTime);

    //
    // Determine the UART divisor value
    //
    switch (DeviceExtension->BaudRate) {
        case 1200:
            Divisor = BAUD_RATE_DIVISOR_1200;
            break;
        case 2400:
            Divisor = BAUD_RATE_DIVISOR_2400;
            break;
        case 4800:
            Divisor = BAUD_RATE_DIVISOR_4800;
            break;
        case 9600:
            Divisor = BAUD_RATE_DIVISOR_9600;
            break;
        case 19200:
            Divisor = BAUD_RATE_DIVISOR_19200;
            break;
        case 38400:
            Divisor = BAUD_RATE_DIVISOR_38400;
            break;
        case 57600:
            Divisor = BAUD_RATE_DIVISOR_57600;
            break;
        case 115200:
            Divisor = BAUD_RATE_DIVISOR_115200;
            break;
        default:
            Divisor = BAUD_RATE_DIVISOR_19200;
            break;
    }

    //
    // Set the baud rate to the divisor value.
    //
    ch = ((READ_PORT_UCHAR (DeviceExtension->ComPort.LCR)) | LCR_ENABLE_DIVISOR_LATCH);
    WRITE_PORT_UCHAR (DeviceExtension->ComPort.LCR, ch);

    ch = READ_PORT_UCHAR (DeviceExtension->ComPort.LCR);
    WRITE_PORT_UCHAR (DeviceExtension->ComPort.BAUD, Divisor);

    ch = ((READ_PORT_UCHAR (DeviceExtension->ComPort.LCR)) & LCR_DISABLE_DIVISOR_LATCH);
    WRITE_PORT_UCHAR (DeviceExtension->ComPort.LCR, ch);

    //
    // The data format = 1 start bit, 8 data bits, 1 stop bit, no parity.
    //
    ch = (LCR_EIGHT_BITS_PER_WORD | LCR_ONE_STOP_BIT | LCR_NO_PARITY);
    WRITE_PORT_UCHAR (DeviceExtension->ComPort.LCR, ch);

    //
    // Enable all UART interrupts on the IBM PC by asserting the GP02 general
    // purpose output. Clear all other MCR bits. Activate DTR for RS485 use.
    //
    ch = MCR_ACTIVATE_GP02 | MCR_ACTIVATE_DTR;
    WRITE_PORT_UCHAR (DeviceExtension->ComPort.MCR, ch);

    //
    // Enable Specific Interrupts
    //
    ch = (IER_ENABLE_RX_DATA_READY_IRQ | IER_ENABLE_TX_BE_IRQ | IER_ENABLE_RX_ERROR_IRQ);
    WRITE_PORT_UCHAR (DeviceExtension->ComPort.IER, ch);

    return status;
}


//---------------------------------------------------------------------------
// RS485_Write 
//
// Description:
//  Called by DispatchRoutine in response to a Write request.
//
// Arguments:
//      DeviceExtension - The device extension strtucture
//      Irp             - The Irp associated with this IO
//
// Return Value:
//      NTSTATUS
//
NTSTATUS RS485_Write (IN PRS485NT_DEVICE_EXTENSION  DeviceExtension, IN PIRP Irp)
{
    ULONG   Length;
    UCHAR   ch;

    Length = IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length;
    Irp->IoStatus.Information = 0L;

    //
    // Check for a zero length write.
    //

    if (Length) {

        if (Length >= DeviceExtension->BufferSize) {

            //
            // Not enough room in the buffer
            //
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        } else {

            //
            // Clear the Transmit complete event
            //
            KeClearEvent (&DeviceExtension->XmitDone);

            //
            // Copy the buffer into the DeviceExtension
            //
            RtlMoveMemory (DeviceExtension->XmitBuffer, 
                           Irp->AssociatedIrp.SystemBuffer, Length);

            DeviceExtension->XmitBufferCount = Length;
            DeviceExtension->XmitBufferPosition = DeviceExtension->XmitBuffer;

            //
            // Assert RTS
            //
            ch = READ_PORT_UCHAR (DeviceExtension->ComPort.MCR) | MCR_ACTIVATE_RTS;
            WRITE_PORT_UCHAR (DeviceExtension->ComPort.MCR, ch);

            //
            // Kick start the UART by jamming one byte out
            //
            WRITE_PORT_UCHAR (DeviceExtension->ComPort.TBR, 
                              *DeviceExtension->XmitBufferPosition);

            DeviceExtension->XmitBufferPosition++;
            DeviceExtension->XmitBufferCount--;

            //
            // Wait for the complete buffer to be sent
            //
            RS_DbgPrint ("RS485NT: Write KeWaitForSingleObject\n");
            KeWaitForSingleObject (&DeviceExtension->XmitDone, Executive, KernelMode, FALSE, NULL);

            //
            // Set the number of bytes written
            //
            Irp->IoStatus.Information = Length;
        }

    } else {
        //
        // Nothing to write, so return SUCCESS (and do nothing!)
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}


//---------------------------------------------------------------------------
// RS485_Read
//
// Description:
//  Called by DispatchRoutine in response to a Read request.
//
// Arguments:
//      DeviceExtension - The device extension strtucture
//      Irp             - The Irp associated with this IO
//
// Return Value:
//      NTSTATUS
//
NTSTATUS RS485_Read (IN PRS485NT_DEVICE_EXTENSION  DeviceExtension, IN PIRP Irp)
{
    ULONG   Length;
    KIRQL   OldIrql;
    
    Length = IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length;
    Irp->IoStatus.Information = 0L;

    //
    // Check for a zero length read.
    //
    if (Length) {

        //
        // Read in the minimum amount (User buffer or Device Extension buffer)
        //

        if (Length > DeviceExtension->RcvBufferCount) {
            Length = DeviceExtension->RcvBufferCount;
        }

        //
        // Synchronize - LOCK
        //
        KeRaiseIrql ((KIRQL)(DeviceExtension->Irql+1), &OldIrql);

        //
        // Copy the buffer from the DeviceExtension
        //
        RtlMoveMemory (Irp->AssociatedIrp.SystemBuffer, 
                       DeviceExtension->RcvBuffer, Length);

        //
        // Clear the Rcv buffer info
        //
        DeviceExtension->RcvBufferCount = 0;
        DeviceExtension->RcvBufferPosition = DeviceExtension->RcvBuffer;

        //
        // Synchronize - UNLOCK
        //
        KeLowerIrql (OldIrql);

        //
        // Set the number of bytes actually read
        //
        Irp->IoStatus.Information = Length;

    } else {
        //
        // Nothing to read, so return SUCCESS (and do nothing!)
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}
