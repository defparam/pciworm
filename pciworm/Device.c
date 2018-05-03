/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    User-mode Driver Framework 2

--*/

#include "driver.h"
#include "device.tmh"
#include <WinUser.h>


NTSTATUS
pciwormEvtDevicePrepareHardware(
	_In_ WDFDEVICE Device,
	_In_ WDFCMRESLIST ResourcesRaw,
	_In_ WDFCMRESLIST ResourcesTranslated
)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG i,j,k;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;
	PDEVICE_CONTEXT deviceContext;
	ULONG64 data = 0;
	ULONG64 addr = 0;
	
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(ResourcesRaw);
	PAGED_CODE();
	for (i = 0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {
		descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);
		if ((descriptor->Type == CmResourceTypeMemory) && (descriptor->u.Memory.Length == 0x1000))
		{
			deviceContext = DeviceGetContext(Device);
			WdfDeviceMapIoSpace(Device,
				descriptor->u.Memory.Start,
				descriptor->u.Memory.Length,
				MmNonCached,
				&deviceContext->paddr);
			deviceContext->PortCount = descriptor->u.Memory.Length;
			deviceContext->PortWasMapped = TRUE;

			addr = (ULONG64)deviceContext->paddr; // weird pointer stuff, i know
			k = 0;
			for (j = 0; j < 0x100; j++) {
				WDF_READ_REGISTER_BUFFER_ULONG64(Device, (PULONG64)addr, &data, 1);
				DbgPrint("(BAR0 0x%04X): 0x%016llx\n", k,data); // ta da
				addr += 8;
				k += 8;
			}
		}
	}
	return status;
}

NTSTATUS
pciwormEvtDeviceReleaseHardware(
	_In_ WDFDEVICE Device,
	_In_ WDFCMRESLIST ResourcesTranslated
) {
	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT deviceContext;
	UNREFERENCED_PARAMETER(Device);
	UNREFERENCED_PARAMETER(ResourcesTranslated);
	PAGED_CODE();
	deviceContext = DeviceGetContext(Device);
	if (deviceContext->PortWasMapped != FALSE) {
		WdfDeviceUnmapIoSpace(Device,
			deviceContext->paddr,
			deviceContext->PortCount
		);
	}
	return status;
}


NTSTATUS
pciwormCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	pnpPowerCallbacks.EvtDevicePrepareHardware = pciwormEvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = pciwormEvtDeviceReleaseHardware;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {
        //
        // Get a pointer to the device context structure that we just associated
        // with the device object. We define this structure in the device.h
        // header file. DeviceGetContext is an inline function generated by
        // using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
        // This function will do the type checking and return the device context.
        // If you pass a wrong object handle it will return NULL and assert if
        // run under framework verifier mode.
        //
        deviceContext = DeviceGetContext(device);

        //
        // Initialize the context.
        //
        deviceContext->PrivateDeviceData = 0;

        //
        // Create a device interface so that applications can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_pciworm,
            NULL // ReferenceString
            );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and any Queues
            //
            status = pciwormQueueInitialize(device);
        }
    }

    return status;
}