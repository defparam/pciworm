# pciworm
An example Windows 10 UMDF driver for the sole purpose of sending MMIO to BAR regions of an FPGA

This is a UMDF example driver that latches onto an FPGA device VendorID 0x1172 (Altera) DeviceID 0xCAFE. I wanted to test MMIOs to my TLP processor in verilog and this example code sends a bunch of MMIO64 reads to bar0 of the PCIe device that is mapped into the system address space.

This driver doesn't do much but scrape BAR0 and print it into the debug log (needs a debugger attached). I will eventually extend this example to drive these MMIOs from a userland application (probably using the queue structure).

This code was partially copied and fully inspired by Alex Ionescu quick blurb about UMDF drivers w/PCIe devices at his talk at Recon Brussels 2017, Getting Physical with USB Type-C (http://alex-ionescu.com/publications/Recon/recon2017-bru.pdf, slide 27-32)

defparam
