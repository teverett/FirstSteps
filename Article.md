Introduction
---
Warning: Writing an Operating System can be a long and frustrating challenge - even for experienced programmers. You will find yourself trying to debug errors for hours, and then see the error right in front of your face. Writing an Operating System is a hard and worthy challenge, but don't be frightened off.

You may think that making an OS kernel is easy if you've ever programmed before. Think again. You don't have an established API or library to program with; You have to start from scratch. In this tutorial, we are going to learn about bootloaders and linkers and the basics of programming a simple Operating System. If you have never programmed before, I suggest you read some tutorials and try a few smaller projects first.

Details
---
The programming language that I prefer is C. C is very portable and can be compiled to run on almost any platform, and is easy to compile and run. I am going to cut to the chase and jump right into development. Our very first step will be printing to screen. You may think you're over your head, but trust me, this isn't too painful =). Open up your favorite text editor or IDE, and create a new file called video.c. This file will contain all of our functions for writing to the screen; at least for now.

Code
---
Our first function will be the classic putc. Putc is an abbreviation for put-char or character. Before you start actually writing the code, there are a few things you should keep in mind: Video memory is located at 0xB8000, and there are 16 foreground and 16 background colors. We are going to be using gray text on a black background for now, so that is 0x7. The code for each character is always stored in an even array vidmem[0] and the color fr each character is stored directly after the character vidmem[1].

<pre>
/* Simple putc */
int x, y; /* Our global 'x' and 'y' */
char color; /* Our global color attribute */
void putc( unsigned char c )
{
  char *vidmem = (char*)0xB8000; /* pointer to video memory */
  int pos = ( y * 2 ) + x; /* Get the position */
  vidmem[pos]   = c; /* print the character */
  vidmem[pos++] = color; /* Set the color attribute */
}
</pre>

Our next function will be puts. This function writes a message to the screen, and relies on the putc function to print each character.

<pre>
int puts( char *message )
{
  int length;
  while(*message)
  {
    putc(*message++);
    length++;
  }
  return length;
}
</pre>

Next we must make a new file: main.c. No, not even operating systems can escape a main() function. This will be the startup function that will be called by the loader.

<pre>
int main( void )
{
  puts("Hello, world!"); /* Print our welcome message */
  for(;;); /* Keep the OS running */
}
</pre>

Down & Dirty with Assembly
---
If you thought any of that was hard, wait 'til you see whats next... Assembly. Ah, yes that dreaded thing that pulled itself out of the abyss. -- Wait, assembly is not that bad. It just takes time to adjust to. Create a new file, loader.asm, or whatever you want to call it.

<pre>
; Loader.asm
bits 32
extern main
global start

start:
  call main  ; Call our kernel's main() function
  cli        ; Stop interrupts (thats another article?)
  hlt        ; Stop all instructions
</pre>  

That certainly wasn't too bad, but that code is for a custom bootloader that's not multiboot compliant. But enough for now.

Compiling & Running
---
If you don't already have gcc , ld or nasm, now is the time to get them. You can either get the djgpp package or the cygwin package if you are using Windows. Here are the commands you need to use to compile you homebrew OS:

<pre>
gcc -ffreestanding -fno-builtin -nostdlib -c *.c (thats main.c and video.c)
nasm -f aout loader.asm -o loader.o
ld -Ttext 0x1000 -o kernel.bin loader.o main.o video.o 
</pre>

And you're done! Wait, I lied. How do you intend to test it? I would recommend virtual machine software, such as Bochs or Qemu. They allow you to test your OS inside of you host operating system, be it linux or Windows. Still, I'm getting ahead of myself, we need something that will actually interface with the BIOS (Basic Input Output System) and load our kernel (BIOS->Bootloader->Loader->Kernel). Here is the code, and comments for my simple bootloader:

<pre>
; Fat12 Bootloader

[BITS 16]                       ; We need 16-bit intructions for Real mode

[ORG 0x7C00]                    ; The BIOS loads the boot sector into memory location 0x7C00

jmp word load_kernel            ; Load the OS Kernel

;----------Fat 12 Header junk----------;

        db "ONYXDISK"           ; OEM Label String
        dw 512                  ; Bytes per sector
        db 1                    ; Sectors per FAT cluster
        dw 36                   ; Resered sector count
        db 2                    ; number of FATs
        dw 224                  ; Root dir entries
        dw 2880                 ; Total Sectors
        db 240                  ; Double sided, 18 sectors per track
        dw 9                    ; Sectors per FAT
        dw 18                   ; Sectors per track
        dw 2                    ; Head count (double sided)
        dd 0                    ; Hidden sector count 
        
drive db 0                      ; Used to store boot device
        
;----------Bootsector Code----------;
  
load_kernel:
        jmp read_disk           ; Load the OS into memory
        jmp get_ram             ; Get ram amount
        jmp enter_pm            ; Enter Protected Mode
        
read_disk:
        mov ah, 0               ; RESET-command
        int 13h                 ; Call interrupt 13h
        mov [drive], dl         ; Store boot disk
        or ah, ah               ; Check for error code
        jnz read_disk           ; Try again if ah != 0
        mov ax, 0
        mov ax, 0                               
        mov es, ax                              
        mov bx, 0x1000          ; Destination address = 0000:1000

        mov ah, 02h             ; READ SECTOR-command
        mov al, 12h             ; Number of sectors to read (0x12 = 18 sectors)
        mov dl, [drive]         ; Load boot disk
        mov ch, 0               ; Cylinder = 0
        mov cl, 2               ; Starting Sector = 3
        mov dh, 0               ; Head = 1
        int 13h                 ; Call interrupt 13h
        or ah, ah               ; Check for error code
        jnz load_kernel         ; Try again if ah != 0
        cli                     ; Disable interrupts, we want to be alone

enter_pm:
        xor ax, ax              ; Clear AX register
        mov ds, ax              ; Set DS-register to 0 - used by lgdt

        lgdt [gdt_desc]         ; Load the GDT descriptor 
        
;----------Entering Protected Mode----------;
                
        mov eax, cr0            ; Copy the contents of CR0 into EAX
        or eax, 1               ; Set bit 0     (0xFE = Real Mode)
        mov cr0, eax            ; Copy the contents of EAX into CR0
        
        jmp 08h:kernel_segments ; Jump to code segment, offset kernel_segments
        

[BITS 32]                       ; We now need 32-bit instructions
kernel_segments:
        mov ax, 10h             ; Save data segment identifyer
        mov ds, ax              ; Move a valid data segment into the data segment register
        mov ss, ax              ; Move a valid data segment into the stack segment register
        mov esp, 090000h        ; Move the stack pointer to 090000h
        
        jmp 08h:0x1000          ; Jump to section 08h (code), offset 01000h
        
;----------Global Descriptor Table----------;

gdt:                            ; Address for the GDT

gdt_null:                       ; Null Segment
        dd 0
        dd 0
        
        
KERNEL_CODE             equ $-gdt
gdt_kernel_code:
        dw 0FFFFh               ; Limit 0xFFFF
        dw 0                    ; Base 0:15
        db 0                    ; Base 16:23
        db 09Ah                 ; Present, Ring 0, Code, Non-conforming, Readable
        db 0CFh                 ; Page-granular
        db 0                    ; Base 24:31

KERNEL_DATA             equ $-gdt
gdt_kernel_data:                        
        dw 0FFFFh               ; Limit 0xFFFF
        dw 0                    ; Base 0:15
        db 0                    ; Base 16:23
        db 092h                 ; Present, Ring 0, Data, Expand-up, Writable
        db 0CFh                 ; Page-granular
        db 0                    ; Base 24:32

gdt_interrupts:
        dw 0FFFFh
        dw 01000h
        db 0
        db 10011110b
        db 11001111b
        db 0

gdt_end:                        ; Used to calculate the size of the GDT

gdt_desc:                       ; The GDT descriptor
        dw gdt_end - gdt - 1    ; Limit (size)
        dd gdt                  ; Address of the GDT

times 510-($-$$) db 0          ; Fill up the file with zeros

dw 0AA55h                       ; Boot sector identifyer
</pre>

Yes I'm sure that is confusing, but here is a short wrap up. When you turn on you computer, it automatically starts into real mode (16 bit mode), where you can use the BIOS to carry out certain functions like printing to the screen and reading files. Real mode has several limitations: you can only access the first 1MB of memory, and there is no protection for data. We (as OS-developers) want to switch to protected mode (32 bit mode), where you have 4GB of memory, and the aforementioned data protection. In protect mode though, you do not have access to the BIOS. Therefore, since we can't rely on the BIOS, we need to write functions for our OS to use. But back to the bootloader. The BIOS will look for a bootsector that is exactly 512 bytes (1 sector) and has 0xAA55 as the boot checksum. Then the BIOS will load our bootsector, which in turn loads our loader, which loads our OS.

Now compile our bootloader using

<pre>
nasm -f bin boot.asm -o boot.bin
</pre>

You should now have the files kernel.bin and boot.bin. If you are using Windows use this command:

<pre>
copy /b boot.bin + kernel.bin os.img
</pre>

Or on linux:
<pre>
dd if=boot.bin of=os.img bs=512 count=1
dd if=kernel.bin of=os.img bs=512 seek=1
</pre>
Then tell your emulator (bochs, qemu) to use that file (os.img) as a floppy disk to boot from, and you're on your way to OS Development.

--Michael 'lazyear' Lazear

Revision 2, 1/5/08