# The Me Operating System

Me Operating System is a Unix-like c/c++ operating system targeting the Intel x86 architecture.

We use the tutorials at [brokenthorn](http://www.brokenthorn.com/Resources/OSDevIndex.html) and [osdev wiki](http://wiki.osdev.org/Main_Page) as a base for the kernel code and build on top of their resources to create a working kernel.

## Table of Contents

* [MeOS features](#features)
* [Project Files Structure](#filestruct)
* [Compiling the kernel files](#compiling)
* [Preparing the virtual machine](#prepvm)
* [Getting Serial Output](#serout)
* [Adding Resources (Add a font for the kernel)](#res)
* [Running the kernel](#runkrnl)
* [Running the kernel without compiling (Jump here if you wish to run the kernel without code modifications)](#runimed)


## MeOS features <a name="features"></a>

Me Operating System is an Intel based OS that can currently boot a 32-bit virtual machine through an ATA hard drive. Below we present a list of the currently available kernel features:

* Virtual memory management
* Memory mapped files
* Unix based virtual file system
* Process-thread scheduler
* VGA linear framebuffer graphics
* Basic network stack
* AHCI SATA driver and FAT32 filesystem support

Due to the kernel being at an early development phase, there is not a straightforward build procedure nor is the code guaranteed to run on every machine - even when configured as described below.


## Project Files Structure <a name="filestruct"></a>

The project structure is important so that the scripts can run correctly without the need for path changes. <br>
The default structure used in this project is the following:

```
.
├── Boot            Bootloader assembly files
├── MeOS            Kernel files
├── Builds          Output directory for compiled kernel images and utility programs
├── MeOSLDR         Kernel loader files (uses SATA driver to load the kernel)
├── Resources       Supplementary files for testing (like fonts, text files, shared libraries...)
└── README.md
```

## Compiling the kernel files <a name="compiling"></a>

The compilation method listed below applies only to Windows systems, using the Visual Studio environment. Unfortunately, there is currently no support for compilation under Linux or other platforms.

### Bootloader - Assembly files

To compile the assembly files we will use the [nasm](http://www.nasm.us/) assembler.
Install the assembler and then execute the following:

```
cd Boot\..
nasm -f bin Boot.asm -o "..\Builds\boot.bin"
nasm -f bin Stage2.asm -o "..\Builds\stage2.sys"
```

### Kernel Loader

Create and configure a Visual Studio project as described in [this](http://www.brokenthorn.com/Resources/OSDevMSVC.html) tutorial. Next, open the project properties window and navigate to
```
Configuration Properties > Linker > Advanced
```
and set
```
Entry Point = ldr_main
Base Address = 0x10000
```

After that, we need to enable [masm](https://msdn.microsoft.com/en-us/library/afzk3475.aspx) for the project assembly file. To do that in Visual Studio 2017 right-click on the project (not on the solution) and navigate to:
```
Build Dependencies > Build Customizations
```
and select masm from the check-list.

Now you can build the project and copy the output executable to the `Builds` directory.


### Kernel

Again create and configure a Visual Studio project as described in [this](http://www.brokenthorn.com/Resources/OSDevMSVC.html) tutorial. Then, open the project properties window and navigate to
```
Configuration Properties > Linker > Advanced
```
and set
```
Entry Point = kmain
Base Address = 0xC0000000
```

Then, enable masm support for this project as described, build the project and copy the output executable to the `Builds` directory.

## Preparing the virtual machine <a name="prepvm"></a>

We will configure the [Oracle VM VirtualBox](https://www.virtualbox.org/) emulator to boot the Me Operating System. 
<br>
Firstly, create a new 32-bit machine and open the Storage settings. Then add a SATA controller and attach two new Virtual Hard Disks that are statically allocated and 256MB in size (this is important for proper disk formatting). We use two drives for development convenience - the bootloader resides in the first one and the actual kernel in the other.

Before continuing make sure you have the following files inside the `Builds` folder:
```
Builds					
├── boot.bin      Boot raw binary image
├── initrd.exe    Utility program to format the two virtual drives
├── MeOS.exe      Kernel image
├── MeOsLDR.exe   Kernel loader image
└── stage2.sys    Second stage loader
```

Now  execute the instruction listed below:
```
cd Builds

initrd.exe form "path\to\vdisk1\name1.vhd" 256MB boot.bin stage2.sys MeOsLDR.exe

initrd.exe clear "path\to\vdisk2\name2.vhd"
initrd.exe form "path\to\vdisk2\name2.vhd" 256MB boot.bin
initrd.exe add "path\to\vdisk2\name2.vhd" MeOs.exe
```

The above script formats the first disk and places the two loaders at the beginning of the drive. Then, it clears the second disk with zeroes, formats it with a valid bootloader (which in the future should display a "not bootable drive" message) and adds a file record of the actual kernel image (using a home-made simple file system).

If you use a different virtual machine, make sure you create the two .vhd virtual drives as explained above.

## Some automation in the procedure

Apart from the two Visual Studio projects, the rest of the build procedure (assembling and formatting the disks) can be automated. For this purpose we have included a `build.bat` batch file in the `Builds` folder that executes the instructions described at the above steps. Just make sure you edit this file's variables to match your virtual drive and nasm paths.

In addition, you can add this script to Visual Studio's post-build events in order to execute it after each succesful build. This way you can build the kernel in a single click!

## Getting Serial Output <a name="serout"></a>

Before launching the operating system, it is useful to run a serial reader program since the kernel constantly outputs serial data about different events that take place.

Firstly, you need to modify the virtual machine settings. In the VirtualBox panel, open the Settings window and navigate to `Serial Ports`. Then enable `Port 1` assigning it a port number (COM1 for Windows) and set the mode to `Host Pipe`. Finally, set the Path/Address to `\\.\pipe\test` (this is an example of the virtual pipe name that is used by SerialDump).

Now you can use a software to connect to the serial pipe and read incoming data.

### SerialDump

You can use the `SerialDump.exe` utility program located in the `Builds` folder. This program outputs the serial data sent by the kernel on a console and in addition, after closing the session it dumps the serial output at some folder.  To run it, execute the following:
```
cd Builds
start "meOsDumper" "SerialDump.exe" dump "path\to\dumps"
```
If you use this software, note that you have to set the VirtualBox Path/Address value to the one provided above. 

### Other Software

You can use any other program to get the serial output (like [putty](http://www.putty.org/)). Make sure to set the serial baud rate to 9600.

## Adding Resources <a name="res"></a>

The kernel comes with some resource/supplementary files that are not needed to boot, but are essential for development and testing. Currently, there are two such files, a full ascii raw font used by the VGA graphics and a text file used for test reading and writing. You can find these files inside the `Resources` folder. 

In order for these files to become accessible to the kernel at runtime, we are currently using a third FAT32 virtual hard drive where these files are stored and loaded when needed. So to use them, create a new 280MB virtual drive and format it with a FAT32 filesystem using a 4096-byte allocation unit (this is imporant for the FAT kernel driver). Then copy the resource files inside the virtual disk and attach it to the VirtualBox SATA controller.


## Running the kernel <a name="runkrnl"></a>

After the configuration procedure you can hit the start button and expect the kernel to boot.<br>
Assuming everything went right you should see a blue screen with a white box almost at its center. In addition, if you have included the kernel resources, a welcome message and a tick+millisecond counter at the bottom should be displayed, like in the picture below.

![Alt boot_image](kernel_boot.png?raw=true "MeOS Boot screen")

## Running the kernel without compiling <a name="runimed"></a>

If you want to run the kernel but you don't want to modify the existing code, you can skip the build procedure and use the binaries provided in the `Builds` folder. These are the executables that match the most recently commited code. After [configuring the virtual machine](#prepvm), run the `build.bat` script located in the `Builds` folder and start your machine.