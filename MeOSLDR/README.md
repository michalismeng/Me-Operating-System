# The Kernel Loader

This is the second stage 32-bit loader used to place the kernel executable into the main memory. 
This image contains a lot of duplicate code from the main kernel project 
(mainly the AHCI SATA driver and the virtual memory routines) which it uses to setup the basic kernel execution environment.
