nasm -f elf32 kernel.asm -o kasm.o
g++ -m32 -c kernel.c -o kc.o -ffreestanding -Wno-write-strings -O0 -Wall -Wextra -fno-exceptions -fno-rtti  

gcc -m32 -c include/screen.c -o obj/screen.o -ffreestanding  
gcc -m32 -c include/cstring.c -o obj/cstring.o -ffreestanding  
gcc -m32 -c include/system.c -o obj/system.o -ffreestanding  
g++ -m32 -c include/utility.c -o obj/utility.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
gcc -m32 -c include/descriptor_tables.c -o obj/descr_tbls.o -ffreestanding  
gcc -m32 -c include/isr.c -o obj/isr.o -ffreestanding  
gcc -m32 -c include/pic.c -o obj/pic.o -ffreestanding  
g++ -m32 -c include/pit.c -o obj/pit.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/kb.c -o obj/kb.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
gcc -m32 -c include/parallel.c -o obj/parallel.o -ffreestanding  
g++ -m32 -c include/timer.c -o obj/timer.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/PCI.c -o obj/PCI.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/AHCI.c -o obj/AHCI.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  

g++ -m32 -c include/SerialPort.cpp -o obj/SerialPort.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/vector.cpp -o obj/vector.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/string.cpp -o obj/string.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  

g++ -m32 -c include/OrderedArray.cpp -o obj/OrderedArray.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/HeapManager.cpp -o obj/HeapManager.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  
g++ -m32 -c include/Memory.cpp -o obj/Memory.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings  

g++ -m32 -c CPPEnv.cpp -o CPPEnv.o -ffreestanding -O0 -Wall -Wextra -fno-exceptions -fno-rtti -Wno-write-strings
g++ -m32 -c include/MemoryManager.cpp -o obj/MManager.o -ffreestanding -Wno-write-strings -O0 -Wall -Wextra -fno-exceptions -fno-rtti

#ld -m elf_i386 -T link.ld -o 
ld -o kernel.out -Tlink.ld kc.o kasm.o obj/system.o obj/cstring.o obj/screen.o obj/utility.o obj/descr_tbls.o obj/isr.o obj/pic.o obj/pit.o obj/kb.o obj/parallel.o obj/PCI.o obj/timer.o obj/AHCI.o CPPEnv.o obj/MManager.o obj/OrderedArray.o obj/HeapManager.o obj/SerialPort.o obj/vector.o obj/string.o obj/Memory.o

objcopy -O elf32-i386 kernel.out kernel.elf

#rm megis/boot/kernel.bin
#cp kernel.bin megis/boot/kernel.bin

#grub-mkrescue -o MO.iso megis/

#VBoxManage startvm MegisOS

#qemu-system-i386 -kernel kernel.bin
