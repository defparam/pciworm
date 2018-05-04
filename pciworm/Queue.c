/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    User-mode Driver Framework 2

--*/

#include "driver.h"
#include "queue.tmh"

NTSTATUS
pciwormQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
	PQUEUE_CONTEXT queueContext;
    WDF_IO_QUEUE_CONFIG queueConfig;
	WDF_OBJECT_ATTRIBUTES queueAttributes;

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
		WdfIoQueueDispatchSequential
        );

	queueConfig.EvtIoDeviceControl = pciwormEvtIoDeviceControl;
	queueConfig.EvtIoStop = pciwormEvtIoStop;

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_CONTEXT);



    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
				 &queueAttributes,
                 &queue
                 );

    if(!NT_SUCCESS(status)) {
        return status;
    }

	// Get our Driver Context memory from the returned Queue handle
	queueContext = QueueGetContext(queue);
	queueContext->Device = Device;

	return status;
}


VOID
pciwormEvtIoDeviceControl(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ size_t OutputBufferLength,
	_In_ size_t InputBufferLength,
	_In_ ULONG IoControlCode
)
/*++
Routine Description:
This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.
Arguments:
Queue -  Handle to the framework queue object that is associated with the
I/O request.
Request - Handle to a framework request object.
OutputBufferLength - Size of the output buffer in bytes
InputBufferLength - Size of the input buffer in bytes
IoControlCode - I/O control code.
Return Value:
VOID
--*/
{
	DbgPrint(
		"Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode 0x%x\n",
		Queue, Request, (int)OutputBufferLength, (int)InputBufferLength, IoControlCode);

	PQUEUE_CONTEXT queueContext;
	PDEVICE_CONTEXT deviceContext;
	ULONG64 addr = 0;
	ULONG64 val = 0;
	PCIE_MMIO64_REQ * reqIn;
	PCIE_MMIO64_REQ * reqOut;
	size_t reqInsz;
	size_t reqOutsz;
	ULONG64 FPGA_UPADDRESS;
	ULONG64 FPGA_UPWRITEDATA;
	ULONG64 FPGA_UPREADDATA;
	ULONG64 FPGA_CTRL;
	ULONG64 FPGA_STATUS;

	queueContext = QueueGetContext(Queue);

	switch (IoControlCode) {
	case IOCTL_PCIE_MMIOREAD64:

		WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(PCIE_MMIO64_REQ),
			&reqIn,
			&reqInsz
		);

		WdfRequestRetrieveOutputBuffer(
			Request,
			sizeof(PCIE_MMIO64_REQ),
			&reqOut,
			&reqOutsz
		);

		if ((reqInsz < sizeof(PCIE_MMIO64_REQ)) || (reqOutsz < sizeof(PCIE_MMIO64_REQ))) { // check to make sure we have enough buffer space
			WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_TOO_SMALL, 0);
			break;
		}
		deviceContext = DeviceGetContext(queueContext->Device);
		addr = (ULONG64)deviceContext->paddr + reqIn->ADDR;
		reqOut->ADDR = reqIn->ADDR;
		WDF_READ_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)addr, &reqOut->DATA, 1);
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(PCIE_MMIO64_REQ));
		break;
	case IOCTL_PCIE_MMIOWRITE64:

		WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(PCIE_MMIO64_REQ),
			&reqIn,
			&reqInsz
		);
		if (reqInsz < sizeof(PCIE_MMIO64_REQ)) { // check to make sure we have enough buffer space
			WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_TOO_SMALL, 0);
			break;
		}
		deviceContext = DeviceGetContext(queueContext->Device);
		addr = (ULONG64)deviceContext->paddr + reqIn->ADDR;
		WDF_WRITE_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)addr, &reqIn->DATA, 1);
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
		break;

	case IOCTL_PCIE2MEM_MMIOREAD64:
		WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(PCIE_MMIO64_REQ),
			&reqIn,
			&reqInsz
		);
		WdfRequestRetrieveOutputBuffer(
			Request,
			sizeof(PCIE_MMIO64_REQ),
			&reqOut,
			&reqOutsz
		);
		if ((reqInsz < sizeof(PCIE_MMIO64_REQ)) || (reqOutsz < sizeof(PCIE_MMIO64_REQ))) { // check to make sure we have enough buffer space
			WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_TOO_SMALL, 0);
			break;
		}
		deviceContext = DeviceGetContext(queueContext->Device);

		FPGA_UPADDRESS = (ULONG64)deviceContext->paddr + 0x10;
		FPGA_UPWRITEDATA = (ULONG64)deviceContext->paddr + 0x18;
		FPGA_UPREADDATA = (ULONG64)deviceContext->paddr + 0x20;
		FPGA_CTRL = (ULONG64)deviceContext->paddr + 0x28;
		FPGA_STATUS = (ULONG64)deviceContext->paddr + 0x30;
		
		// Set the Physical memory address in the FPGA
		WDF_WRITE_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_UPADDRESS, &reqIn->ADDR, 1);
		val = 0x2;
		// Issue a upstream read command to the FPGA
		WDF_WRITE_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_CTRL, &val, 1);

		// Spin loop until in_progress bit equals 0
		WDF_READ_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_STATUS, &val, 1);
		while (val & 0x2) {
			WDF_READ_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_STATUS, &val, 1);
		}

		// Report back the status of the upstream TLP read operation
		reqOut->sts = (val & 0x7);
		reqOut->ADDR = reqIn->ADDR;
		// Report back the read data
		WDF_READ_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_UPREADDATA, &reqOut->DATA, 1);
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, sizeof(PCIE_MMIO64_REQ));
		break;
	case IOCTL_PCIE2MEM_MMIOWRITE64:
		WdfRequestRetrieveInputBuffer(
			Request,
			sizeof(PCIE_MMIO64_REQ),
			&reqIn,
			&reqInsz
		);
		if (reqInsz < sizeof(PCIE_MMIO64_REQ)) { // check to make sure we have enough buffer space
			WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_TOO_SMALL, 0);
			break;
		}
		deviceContext = DeviceGetContext(queueContext->Device);


		FPGA_UPADDRESS = (ULONG64)deviceContext->paddr + 0x10;
		FPGA_UPWRITEDATA = (ULONG64)deviceContext->paddr + 0x18;
		FPGA_UPREADDATA = (ULONG64)deviceContext->paddr + 0x20;
		FPGA_CTRL = (ULONG64)deviceContext->paddr + 0x28;
		FPGA_STATUS = (ULONG64)deviceContext->paddr + 0x30;


		WDF_WRITE_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_UPADDRESS, &reqIn->ADDR, 1);
		WDF_WRITE_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_UPWRITEDATA, &reqIn->DATA, 1);
		val = 0x1;
		// Issue a upstream write command to the FPGA
		WDF_WRITE_REGISTER_BUFFER_ULONG64(queueContext->Device, (PULONG64)FPGA_CTRL, &val, 1);

		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
		break;


		break;
	default:
		WdfRequestCompleteWithInformation(Request, STATUS_INVALID_PARAMETER, 0);
		break;
	}
	return;
}

VOID
pciwormEvtIoStop(
	_In_ WDFQUEUE Queue,
	_In_ WDFREQUEST Request,
	_In_ ULONG ActionFlags
)
/*++
Routine Description:
This event is invoked for a power-managed queue before the device leaves the working state (D0).
Arguments:
Queue -  Handle to the framework queue object that is associated with the
I/O request.
Request - Handle to a framework request object.
ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
that identify the reason that the callback function is being called
and whether the request is cancelable.
Return Value:
VOID
--*/
{
	DbgPrint(
		"%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d",
		Queue, Request, ActionFlags);

	//
	// In most cases, the EvtIoStop callback function completes, cancels, or postpones
	// further processing of the I/O request.
	//
	// Typically, the driver uses the following rules:
	//
	// - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
	//   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
	//   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
	//   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
	//
	//   Before it can call these methods safely, the driver must make sure that
	//   its implementation of EvtIoStop has exclusive access to the request.
	//
	//   In order to do that, the driver must synchronize access to the request
	//   to prevent other threads from manipulating the request concurrently.
	//   The synchronization method you choose will depend on your driver's design.
	//
	//   For example, if the request is held in a shared context, the EvtIoStop callback
	//   might acquire an internal driver lock, take the request from the shared context,
	//   and then release the lock. At this point, the EvtIoStop callback owns the request
	//   and can safely complete or requeue the request.
	//
	// - If the driver has forwarded the I/O request to an I/O target, it either calls
	//   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
	//   further processing of the request and calls WdfRequestStopAcknowledge with
	//   a Requeue value of FALSE.
	//
	// A driver might choose to take no action in EvtIoStop for requests that are
	// guaranteed to complete in a small amount of time.
	//
	// In this case, the framework waits until the specified request is complete
	// before moving the device (or system) to a lower power state or removing the device.
	// Potentially, this inaction can prevent a system from entering its hibernation state
	// or another low system power state. In extreme cases, it can cause the system
	// to crash with bugcheck code 9F.
	//

	return;
}
