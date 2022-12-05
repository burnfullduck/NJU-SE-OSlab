section .data
invalid db "Invalid", 0h

section .bss
input: resb 63
num1: resb 63
num2: resb 63
opChar: resb 1
result: resb 63


section .text
global _start
;宏定义pushe为按反序push eax等4个寄存器，pope为顺序pop
%macro pope 0 
    pop eax
    pop ebx
    pop ecx
    pop edx
%endmacro

%macro pushe 0
    push edx
    push ecx
    push ebx
    push eax
%endmacro

quit: 
	mov	eax, 1
    mov	ebx, 0
	int	80h
	ret

strLength:         ;获取字符串长度，输入字符串放在eax里，输出长度放在eax里
    push ebx
    mov ebx, eax
.nextChar:
    cmp BYTE[eax], 0  ;字符串最后一位为0x00，若ebx所存地址指向的值和0相等则字符串到结尾
    jz .out
    inc eax 
    jmp .nextChar
.out:         ;将计算出长度后退出
    sub eax, ebx
    pop ebx
    cmp eax, 0  ;若长度为零，终止并输出invalid
    jz .quit
    ret

.quit:
    mov eax, 4
    mov ebx, 1
    mov ecx, invalid
    mov edx, 8
    int 80h
    call quit

printStr:               ;输出eax中的字符串
    pushe       
    mov ecx, eax
    call strLength
    mov edx, eax
    mov ebx, 1
    mov eax, 4
    int 80h
    pope
    ret

getInputLine:  ;变量和长度分别放在eax和ebx中。之后调用标准读取，将输入放入变量
    pushe
    mov ecx, eax
    mov edx, ebx
    mov ebx, 1
    mov eax, 3
    int 80h
    pope
    ret

carry:  ;进一位即carry位（eax）置一, 加法结果减10
    mov ecx, 1
    sub al, 10
    ret

mulLoop2: ;乘法运算内循环
    push edi
    push ebx
    push edx   ;将用到的寄存器入栈

.loop:
    cmp edi ,num2  ;数字二到头则结束循环
    jl .out

    mov eax, 0
    mov ebx, 0   ;清空al和bl
    add al, BYTE[edi]
    add bl, BYTE[esi]
    sub al, 48
    sub bl, 48
    mul bl  ;al*bl -> eax
    add BYTE[edx], al
    mov al, BYTE[edx]   ;由于是个位数相乘，得到的数必小于等于81
                        ;再加上进位也不会超过两位数
                        ;故只用除一回10即可
    mov ah, 0
    mov bl, 10
    div bl        ;得到个位在ah中,剩下的al是原结果的进位
    mov BYTE[edx], ah

    dec edx
    dec edi
    add BYTE[edx], al
    jmp .loop

.out:
    mov eax, edx  ;将结果指针给eax
    pop edx
    pop ebx
    pop edi
    ret

intParseStr:         ;对eax操作，实际将result中数字转为字符串
    push ebx
    push eax
.loop:
    mov ebx, result
    add ebx, 63
    cmp eax, ebx
    jnl .out
    add BYTE[eax], 48   ;变为字符串
    inc eax
    jmp .loop
.out:
    pop eax
    pop ebx
    ret

_start:
    mov eax, input
    mov ebx, 63
    call getInputLine  ;获得输入

    ;提取两个数以及操作符
    mov eax, input   
    mov ebx, num1    
    mov ecx, num2
    
    cmp BYTE[eax], 113 ;若输入第一个字符是q则终止并退出
    jz .quit
    
.loopChar1:
    cmp BYTE[eax], 43 ;+
    jz .getOpChar
    cmp BYTE[eax], 42 ;*
    jz .getOpChar
    mov dl, BYTE[eax]  ;将字符先赋给dl低8位数据寄存器,在给ebx中的num1，直接赋值报错error: invalid combination of opcode and operands
    mov BYTE[ebx], dl  ;一个字符一个字符地得到num1
    inc ebx
    inc eax
    jmp .loopChar1

.quit:
    call quit

.getOpChar:
    mov edx, opChar
    mov bl, BYTE[eax]
    mov BYTE[edx], bl  ;得到操作符+/*
    inc eax
.loopChar2:
    cmp BYTE[eax], 32 ;空格
    jz .getOut
    cmp BYTE[eax], 10 ;回车
    jz .getOut
    mov bl, BYTE[eax]  
    mov BYTE[ecx], bl  ;一个字符一个字符地得到num2
    inc ecx
    inc eax
    jmp .loopChar2

.getOut:
    mov ebx, 0
    mov ecx, 0   ;清空寄存器

    cmp Byte[edx], 43;按符号跳转
    jz .add
    jmp .multip

    ;加法
.add:
    mov edx, result ;将edx指向输出字符串的最后一位
    add edx, 63     ;此处不减一以便之后操作先减一再赋值
    mov BYTE[edx], 10

    mov eax, num1
    call strLength
    mov esi, eax
    add esi, num1
    sub esi, 1     ;esi指向num1的第一位

    mov eax, num2
    call strLength
    mov edi, eax
    add edi, num2
    sub edi, 1     ;edi指向num2的第一位
.addLoop:
    cmp esi, num1  ;判断是否num1到头
    jl .endNum1
    cmp edi, num2 
    jl .endNum2

    mov eax, 0  ;清除eax，为之后使用al寄存器做准备
    add al, BYTE[esi]
    sub al, 48     ;将字符转为数字
    add al, BYTE[edi]
    add al, cl   ;cl为进位
    mov ecx, 0    ;cl重置为0

    dec esi
    dec edi
    dec edx  ;分别将num1，2以及result的指针向前移动
    mov BYTE[edx], al   ;赋值给结果

    cmp al, 57    ;判断是否溢出
    jng .addLoop   ;小于等于
    call carry    ;溢出则进位
    mov BYTE[edx], al
    jmp .addLoop

.endNum1:
    cmp edi, num2      ;若num2也到头则结束
    jl .addEnd

    mov eax, 0
    add al, BYTE[edi]           
    add al, cl         
    mov ecx, 0

    dec edi                     
    dec edx                 
    mov BYTE[edx], al

    cmp al, 57
    jng .endNum1
    call carry
    mov BYTE[edx], al
    jmp .endNum1
    
.endNum2:
    cmp esi, num1
    jl .addEnd

    mov eax, 0
    add al, BYTE[esi]           
    add al, cl                 
    mov ecx, 0

    dec esi                     
    dec edx                    
    mov BYTE[edx], al

    cmp al, 57
    jng .endNum2
    call carry
    mov BYTE[edx], al
    jmp .endNum2

.addEnd:
    cmp ecx, 1 ;若有进位则
    jz .endCarry
    jmp .addOutput

.endCarry:
    dec edx
    mov al, 49    ;赋值为1
    mov BYTE[edx], al
    jmp .addOutput

.addOutput:
    mov eax, edx
    call printStr
    call clearResult
    mov eax, 0
    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov esi, 0
    mov edi, 0
    jmp _start

    ;加法到此结束

.multip:   ;同add，后续可优化;懒得优化了
    mov edx, result
    add edx, 63    ;将edx指向result最后一位
    mov BYTE[edx], 10
    dec edx

    mov eax, num1
    call strLength
    mov esi, eax
    add esi, num1
    sub esi, 1     ;esi指向num1的第一位

    mov eax, num2
    call strLength
    mov edi, eax
    add edi, num2
    sub edi, 1     ;edi指向num2的第一位

.mulLoop1:    ;乘法运算外循环
    cmp esi, num1  ;若到头则输出
    jl .outputMul
    call mulLoop2
    dec edx
    dec esi
    jmp .mulLoop1

.outputMul:
    call intParseStr   ;此时eax中应为result的第一位

.mulLoop3:  ;乘法运算最后判断eax是否是字符串开头
    cmp BYTE[eax], 48   ;若eax指不向0则说明为字符串第一位
    jnz .mulOutput
    mov ebx, 0
    add ebx, result
    add ebx, 62        ;ebx指向result最后一位

    cmp eax, ebx
    jz .mulOutput     ;若到最后一位还是0则输出0
    inc eax
    jmp .mulLoop3

.mulOutput:

    call printStr
    call clearResult
    mov eax, 0
    mov ebx, 0
    mov ecx, 0
    mov edx, 0
    mov esi, 0
    mov edi, 0
    
    jmp _start

clearResult:
    mov eax, result
    mov ebx, num1
    mov edx, num2
    mov ecx, input 
    add eax, 63
    add ebx, 63
    add ecx, 63
    add edx, 63
.loop:
    mov BYTE[eax], 0
    dec eax
    mov BYTE[ebx], 0
    dec ebx
    mov BYTE[ecx], 0
    dec ecx
    mov BYTE[edx], 0
    dec edx
    cmp eax, result
    jnz .loop
    ret
