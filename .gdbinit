set architecture i386
target remote localhost:1234
symbol-file ./bin/kernel.elf
break *0x0000000000100000
continue
