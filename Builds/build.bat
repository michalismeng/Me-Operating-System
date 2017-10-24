echo off

:: This script is used to assemble the boot files and initialize the two drives required to run the kernel.
:: The kernel image is not produced by this script and is assumed to already exist in the builds folder.

:: Execute this script from inside the Builds folder

:: Required files at this folder level:
:: Compiled images: MeOS.exe, MeOsLDR.exe
:: Utility programs: initrd.exe

:: Paths for the two virtual hard drives
set "vdisk1=path\to\vdisk1\disk_name1.vhd"
set "vdisk2=path\to\vdisk1\disk_name2.vhd"

:: Path for nasm executable
set "nasmpath=path\to\nasmnasm.exe"

:: Go back one folder for the assembly include files to work
cd ..

:: Assemble the two boot files
"%nasmpath%" -f bin Boot\Boot.asm -o Builds\boot.bin
"%nasmpath%" -f bin Boot\Stage2.asm -o Builds\stage2.sys

cd Builds

:: Format and initialize the drives
initrd.exe clear "%vdisk1%"
initrd.exe form "%vdisk1%" 256MB boot.bin stage2.sys MeOsLDR.exe

initrd.exe clear "%vdisk2%"
initrd.exe form "%vdisk2%" 256MB boot.bin
initrd.exe add "%vdisk2%" MeOs.exe