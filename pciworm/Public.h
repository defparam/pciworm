/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    driver and application

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_pciworm,
    0x5fb65a4e,0x46ad,0x45db,0xa3,0x04,0xdc,0x0c,0x45,0xba,0x8e,0x98);
// {5fb65a4e-46ad-45db-a304-dc0c45ba8e98}
