echo off
cd/d "C:\Users\michalis\Documents\Visual Studio 2015\Projects\MeOS\Builds"

del ca.exe

copy "C:\Users\michalis\Documents\visual studio 2015\Projects\ConsoleApplication1\ConsoleApplication1\bin\Debug\ConsoleApplication1.exe" ca.exe
copy "C:\Users\michalis\Documents\Visual Studio 2015\Projects\MeOS\Debug\MeOS.exe" MeOS.exe

cd/d "C:\Users\michalis\Documents\Visual Studio 2015\Projects\MeOS"

"C:\Program Files (x86)\NASM\nasm.exe" -f bin Boot/Boot.asm -o Builds\boot.bin
"C:\Program Files (x86)\NASM\nasm.exe" -f bin Boot/Stage2.asm -o Builds\stage2.sys

Builds\ca.exe "C:\Users\michalis\VirtualBox VMs\MeOS\MeOS.vhd" 256MB Builds\boot.bin Builds\stage2.sys Builds\MeOS.exe

"C:\Program Files\Oracle\VirtualBox\VBoxManage" startvm MeOS