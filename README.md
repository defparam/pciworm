# pciworm
An example Windows 10 UMDF driver for the sole purpose of sending MMIO to BAR regions of an FPGA

This is a UMDF example driver that latches onto an FPGA device VendorID 0x1172 (Altera) DeviceID 0xCAFE. I wanted to test MMIOs to my TLP processor in verilog and this example code sends a bunch of MMIO64 reads to bar0 of the PCIe device that is mapped into the system address space.

The latest version of this code shows how to write a userland app to open the UMDF driver device and send 4 different types of IOCTLs.

IOCTLs:
1) fpgaRead64 - This IOCTL from the app tells the driver to read from a specific BAR0 address of the FPGA
2) fpgaWrite64 - This IOCTL from the app tells the driver to write to a specific BAR0 address of the FPGA
3) memRead64 - This IOCTL from the app tells the driver to instruct the FPGA to perform an upstream read of physical memory
4) memWrite64 - This IOCTL from the app tells the driver to instruct the FPGA to perform an upstream write of physical memory

This code was partially copied and fully inspired by Alex Ionescu quick blurb about UMDF drivers w/PCIe devices at his talk at Recon Brussels 2017, Getting Physical with USB Type-C (http://alex-ionescu.com/publications/Recon/recon2017-bru.pdf, slide 27-32)

defparam
