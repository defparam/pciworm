#include "stdafx.h"
#include "pciworm_if.h"



BOOL
GetDevicePath(
	_In_ LPGUID InterfaceGuid,
	_Out_writes_(BufLen) PWCHAR DevicePath,
	_In_ size_t BufLen
)
{
	CONFIGRET cr = CR_SUCCESS;
	PWSTR deviceInterfaceList = NULL;
	ULONG deviceInterfaceListLength = 0;
	PWSTR nextInterface;
	HRESULT hr = E_FAIL;
	BOOL bRet = TRUE;

	cr = CM_Get_Device_Interface_List_Size(
		&deviceInterfaceListLength,
		InterfaceGuid,
		NULL,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
	if (cr != CR_SUCCESS) {
		printf("Error 0x%x retrieving device interface list size.\n", cr);
		goto clean0;
	}

	if (deviceInterfaceListLength <= 1) {
		bRet = FALSE;
		printf("Error: No active device interfaces found.\n"
			" Is the sample driver loaded?");
		goto clean0;
	}

	deviceInterfaceList = (PWSTR)malloc(deviceInterfaceListLength * sizeof(WCHAR));
	if (deviceInterfaceList == NULL) {
		printf("Error allocating memory for device interface list.\n");
		goto clean0;
	}
	ZeroMemory(deviceInterfaceList, deviceInterfaceListLength * sizeof(WCHAR));

	cr = CM_Get_Device_Interface_List(
		InterfaceGuid,
		NULL,
		deviceInterfaceList,
		deviceInterfaceListLength,
		CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
	if (cr != CR_SUCCESS) {
		printf("Error 0x%x retrieving device interface list.\n", cr);
		goto clean0;
	}

	nextInterface = deviceInterfaceList + wcslen(deviceInterfaceList) + 1;
	if (*nextInterface != UNICODE_NULL) {
		printf("Warning: More than one device interface instance found. \n"
			"Selecting first matching device.\n\n");
	}

	hr = StringCchCopy(DevicePath, BufLen, deviceInterfaceList);
	if (FAILED(hr)) {
		bRet = FALSE;
		printf("Error: StringCchCopy failed with HRESULT 0x%x", hr);
		goto clean0;
	}

clean0:
	if (deviceInterfaceList != NULL) {
		free(deviceInterfaceList);
	}
	if (CR_SUCCESS != cr) {
		bRet = FALSE;
	}

	return bRet;
}




pciworm_if::pciworm_if()
{
	
	GetDevicePath(
		(LPGUID)&GUID_DEVINTERFACE_pciworm,
		G_DevicePath,
		sizeof(G_DevicePath) / sizeof(G_DevicePath[0]));
}
int pciworm_if::open() {
	hDevice = CreateFile(G_DevicePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hDevice == INVALID_HANDLE_VALUE) {
		printf("Failed to open device. Error %d\n", GetLastError());
		return 0;
	}

	printf("Opened device successfully\n");
	return 1;
}
int pciworm_if::close() {
	CloseHandle(hDevice);
	hDevice = INVALID_HANDLE_VALUE;
	printf("Closed device successfully\n");
	return 0;
}




// MMIO Read 64 on PCIe BAR0 of FPGA device
// This function reads FPGA memory
int pciworm_if::fpgaRead64(LONG64 addr, PULONG64 data) {
	PCIE_MMIO64_REQ req;
	BOOL rval;
	DWORD bytesRet;
	req.ADDR = addr;
	rval =
		DeviceIoControl(hDevice,
			IOCTL_PCIE_MMIOREAD64,
			&req,
			sizeof(req),
			&req,
			sizeof(req),
			&bytesRet,
			NULL
		);
	*data = req.DATA;
	return rval;
}

// MMIO Write 64 on PCIe BAR0 of FPGA device
// This function writes FPGA memory
int pciworm_if::fpgaWrite64(LONG64 addr, ULONG64 data) {
	PCIE_MMIO64_REQ req;
	BOOL rval;
	DWORD bytesRet;
	req.ADDR = addr;
	req.DATA = data;
	rval =
		DeviceIoControl(hDevice,
			IOCTL_PCIE_MMIOWRITE64,
			&req,
			sizeof(req),
			NULL,
			NULL,
			&bytesRet,
			NULL
		);
	return rval;
}

// Mem Read 64 on PCIe BAR0 of FPGA device
// This function reads host physical memory via FPGA
int pciworm_if::memRead64(LONG64 addr, PULONG64 data) {
	PCIE_MMIO64_REQ req;
	BOOL rval;
	DWORD bytesRet;
	req.ADDR = addr;
	rval =
		DeviceIoControl(hDevice,
			IOCTL_PCIE2MEM_MMIOREAD64,
			&req,
			sizeof(req),
			&req,
			sizeof(req),
			&bytesRet,
			NULL
		);
	*data = req.DATA;
	return rval;
}

// Mem Write 64 on PCIe BAR0 of FPGA device
// This function writes host physical memory via FPGA
int pciworm_if::memWrite64(LONG64 addr, ULONG64 data) {
	PCIE_MMIO64_REQ req;
	BOOL rval;
	DWORD bytesRet;
	req.ADDR = addr;
	req.DATA = data;
	rval =
		DeviceIoControl(hDevice,
			IOCTL_PCIE2MEM_MMIOWRITE64,
			&req,
			sizeof(req),
			&req,
			sizeof(req),
			&bytesRet,
			NULL
		);
	return rval;
}

