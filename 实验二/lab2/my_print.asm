global	my_print
	section .text
my_print:
        push    ebp             //
        mov     ebp, esp
        mov     edx, [ebp+12]   //传入的第二个参数
        mov     ecx, [ebp+8]    //传入的第一个参数
        mov     ebx, 1
        mov     eax, 4
        int     80h
        pop     ebp
        ret