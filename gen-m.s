true equ 1
false equ 0
section .data
align 4
section .bss
align 4
stk resd 1024
stk.end:
section .text
align 4
bits 64
global _start
_start:
mov rsp,stk.end
mov rbp,rsp
jmp main
assert:
   push rbp
   mov rbp, rsp
   if_8_8:
   cmp_8_8:
   cmp byte[rbp+16], 0
   jne if_8_5_end
   if_8_8_code:  ; opt1
       mov rdi, 1
       mov rax, 60
       syscall
     exit_8_17_end:
   if_8_5_end:
   pop rbp
   ret
main:
   mov dword[rbp-4], 10
     movsx r15, dword[rbp-4]
     mov qword[rbp-19], r15
     mov dword[rbp-11], 2
     movsx r15, dword[rbp-11]
     movsx r14, dword[rbp-4]
     imul r15, r14
     mov dword[rbp-11], r15d
     mov word[rbp-7], 12
     mov byte[rbp-5], 13
   sub rsp, 19
     cmp_20_12:
     cmp qword[rbp-19], 10
     jne false_20_12
     true_20_12:  ; opt1
     mov r15, 1
     jmp end_20_12
     false_20_12:
     mov r15, 0
     end_20_12:
   push r15
   call assert
   add rsp, 27
   sub rsp, 19
     cmp_21_12:
     cmp dword[rbp-11], 20
     jne false_21_12
     true_21_12:  ; opt1
     mov r15, 1
     jmp end_21_12
     false_21_12:
     mov r15, 0
     end_21_12:
   push r15
   call assert
   add rsp, 27
   sub rsp, 19
     cmp_22_12:
     cmp word[rbp-7], 12
     jne false_22_12
     true_22_12:  ; opt1
     mov r15, 1
     jmp end_22_12
     false_22_12:
     mov r15, 0
     end_22_12:
   push r15
   call assert
   add rsp, 27
   sub rsp, 19
     cmp_23_12:
     cmp byte[rbp-5], 13
     jne false_23_12
     true_23_12:  ; opt1
     mov r15, 1
     jmp end_23_12
     false_23_12:
     mov r15, 0
     end_23_12:
   push r15
   call assert
   add rsp, 27
     mov r15, qword[rbp-19]
     mov qword[rbp-34], r15
     mov r15d, dword[rbp-11]
     mov dword[rbp-26], r15d
     mov r15w, word[rbp-7]
     mov word[rbp-22], r15w
     mov r15b, byte[rbp-5]
     mov byte[rbp-20], r15b
       movsx r15, dword[rbp-4]
       mov qword[rbp-65], r15
       movsx r15, dword[rbp-4]
       add qword[rbp-65], r15
       mov dword[rbp-57], 2
       mov word[rbp-53], 3
       mov byte[rbp-51], 4
       mov qword[rbp-50], 5
       mov dword[rbp-42], 6
       mov word[rbp-38], 7
       mov byte[rbp-36], 8
     mov byte[rbp-35], true
   sub rsp, 65
     cmp_28_12:
     cmp qword[rbp-65], 20
     jne false_28_12
     true_28_12:  ; opt1
     mov r15, 1
     jmp end_28_12
     false_28_12:
     mov r15, 0
     end_28_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_29_12:
     cmp dword[rbp-57], 2
     jne false_29_12
     true_29_12:  ; opt1
     mov r15, 1
     jmp end_29_12
     false_29_12:
     mov r15, 0
     end_29_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_30_12:
     cmp word[rbp-53], 3
     jne false_30_12
     true_30_12:  ; opt1
     mov r15, 1
     jmp end_30_12
     false_30_12:
     mov r15, 0
     end_30_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_31_12:
     cmp byte[rbp-51], 4
     jne false_31_12
     true_31_12:  ; opt1
     mov r15, 1
     jmp end_31_12
     false_31_12:
     mov r15, 0
     end_31_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_32_12:
     cmp qword[rbp-50], 5
     jne false_32_12
     true_32_12:  ; opt1
     mov r15, 1
     jmp end_32_12
     false_32_12:
     mov r15, 0
     end_32_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_33_12:
     cmp dword[rbp-42], 6
     jne false_33_12
     true_33_12:  ; opt1
     mov r15, 1
     jmp end_33_12
     false_33_12:
     mov r15, 0
     end_33_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_34_12:
     cmp word[rbp-38], 7
     jne false_34_12
     true_34_12:  ; opt1
     mov r15, 1
     jmp end_34_12
     false_34_12:
     mov r15, 0
     end_34_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_35_12:
     cmp byte[rbp-36], 8
     jne false_35_12
     true_35_12:  ; opt1
     mov r15, 1
     jmp end_35_12
     false_35_12:
     mov r15, 0
     end_35_12:
   push r15
   call assert
   add rsp, 73
   sub rsp, 65
     cmp_36_12:
     cmp byte[rbp-35], true
     jne false_36_12
     true_36_12:  ; opt1
     mov r15, 1
     jmp end_36_12
     false_36_12:
     mov r15, 0
     end_36_12:
   push r15
   call assert
   add rsp, 73
     mov qword[rbp-80], -1
     mov dword[rbp-72], -2
     mov word[rbp-68], -3
     mov byte[rbp-66], -4
     mov r15, qword[rbp-80]
     mov qword[rbp-65], r15
     mov r15d, dword[rbp-72]
     mov dword[rbp-57], r15d
     mov r15w, word[rbp-68]
     mov word[rbp-53], r15w
     mov r15b, byte[rbp-66]
     mov byte[rbp-51], r15b
   sub rsp, 80
     cmp_40_12:
     cmp qword[rbp-65], -1
     jne false_40_12
     true_40_12:  ; opt1
     mov r15, 1
     jmp end_40_12
     false_40_12:
     mov r15, 0
     end_40_12:
   push r15
   call assert
   add rsp, 88
   sub rsp, 80
     cmp_41_12:
     cmp dword[rbp-57], -2
     jne false_41_12
     true_41_12:  ; opt1
     mov r15, 1
     jmp end_41_12
     false_41_12:
     mov r15, 0
     end_41_12:
   push r15
   call assert
   add rsp, 88
   sub rsp, 80
     cmp_42_12:
     cmp word[rbp-53], -3
     jne false_42_12
     true_42_12:  ; opt1
     mov r15, 1
     jmp end_42_12
     false_42_12:
     mov r15, 0
     end_42_12:
   push r15
   call assert
   add rsp, 88
   sub rsp, 80
     cmp_43_12:
     cmp byte[rbp-51], -4
     jne false_43_12
     true_43_12:  ; opt1
     mov r15, 1
     jmp end_43_12
     false_43_12:
     mov r15, 0
     end_43_12:
   push r15
   call assert
   add rsp, 88
     mov qword[rbp-95], -11
     mov dword[rbp-87], -22
     mov word[rbp-83], -33
     mov byte[rbp-81], -44
       mov r15, qword[rbp-80]
       mov qword[rbp-126], r15
       mov r15d, dword[rbp-72]
       mov dword[rbp-118], r15d
       mov r15w, word[rbp-68]
       mov word[rbp-114], r15w
       mov r15b, byte[rbp-66]
       mov byte[rbp-112], r15b
       mov r15, qword[rbp-95]
       mov qword[rbp-111], r15
       mov r15d, dword[rbp-87]
       mov dword[rbp-103], r15d
       mov r15w, word[rbp-83]
       mov word[rbp-99], r15w
       mov r15b, byte[rbp-81]
       mov byte[rbp-97], r15b
     cmp_46_31:
     cmp dword[rbp-4], 10
     jne false_46_31
     true_46_31:  ; opt1
     mov byte[rbp-96], 1
     jmp end_46_31
     false_46_31:
     mov byte[rbp-96], 0
     end_46_31:
   sub rsp, 126
     cmp_47_12:
     cmp qword[rbp-126], -1
     jne false_47_12
     true_47_12:  ; opt1
     mov r15, 1
     jmp end_47_12
     false_47_12:
     mov r15, 0
     end_47_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_48_12:
     cmp dword[rbp-118], -2
     jne false_48_12
     true_48_12:  ; opt1
     mov r15, 1
     jmp end_48_12
     false_48_12:
     mov r15, 0
     end_48_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_49_12:
     cmp word[rbp-114], -3
     jne false_49_12
     true_49_12:  ; opt1
     mov r15, 1
     jmp end_49_12
     false_49_12:
     mov r15, 0
     end_49_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_50_12:
     cmp byte[rbp-112], -4
     jne false_50_12
     true_50_12:  ; opt1
     mov r15, 1
     jmp end_50_12
     false_50_12:
     mov r15, 0
     end_50_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_51_12:
     cmp qword[rbp-111], -11
     jne false_51_12
     true_51_12:  ; opt1
     mov r15, 1
     jmp end_51_12
     false_51_12:
     mov r15, 0
     end_51_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_52_12:
     cmp dword[rbp-103], -22
     jne false_52_12
     true_52_12:  ; opt1
     mov r15, 1
     jmp end_52_12
     false_52_12:
     mov r15, 0
     end_52_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_53_12:
     cmp word[rbp-99], -33
     jne false_53_12
     true_53_12:  ; opt1
     mov r15, 1
     jmp end_53_12
     false_53_12:
     mov r15, 0
     end_53_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_54_12:
     cmp byte[rbp-97], -44
     jne false_54_12
     true_54_12:  ; opt1
     mov r15, 1
     jmp end_54_12
     false_54_12:
     mov r15, 0
     end_54_12:
   push r15
   call assert
   add rsp, 134
   sub rsp, 126
     cmp_55_12:
     cmp byte[rbp-96], true
     jne false_55_12
     true_55_12:  ; opt1
     mov r15, 1
     jmp end_55_12
     false_55_12:
     mov r15, 0
     end_55_12:
   push r15
   call assert
   add rsp, 134
       mov r15, qword[rbp-126]
       mov qword[rbp-157], r15
       mov r15d, dword[rbp-118]
       mov dword[rbp-149], r15d
       mov r15w, word[rbp-114]
       mov word[rbp-145], r15w
       mov r15b, byte[rbp-112]
       mov byte[rbp-143], r15b
       mov r15, qword[rbp-111]
       mov qword[rbp-142], r15
       mov r15d, dword[rbp-103]
       mov dword[rbp-134], r15d
       mov r15w, word[rbp-99]
       mov word[rbp-130], r15w
       mov r15b, byte[rbp-97]
       mov byte[rbp-128], r15b
     mov r15b, byte[rbp-96]
     mov byte[rbp-127], r15b
   sub rsp, 157
     cmp_58_12:
     cmp qword[rbp-157], -1
     jne false_58_12
     true_58_12:  ; opt1
     mov r15, 1
     jmp end_58_12
     false_58_12:
     mov r15, 0
     end_58_12:
   push r15
   call assert
   add rsp, 165
   sub rsp, 157
     cmp_59_12:
     cmp dword[rbp-149], -2
     jne false_59_12
     true_59_12:  ; opt1
     mov r15, 1
     jmp end_59_12
     false_59_12:
     mov r15, 0
     end_59_12:
   push r15
   call assert
   add rsp, 165
   sub rsp, 157
     cmp_60_12:
     cmp word[rbp-145], -3
     jne false_60_12
     true_60_12:  ; opt1
     mov r15, 1
     jmp end_60_12
     false_60_12:
     mov r15, 0
     end_60_12:
   push r15
   call assert
   add rsp, 165
   sub rsp, 157
     cmp_61_12:
     cmp byte[rbp-143], -4
     jne false_61_12
     true_61_12:  ; opt1
     mov r15, 1
     jmp end_61_12
     false_61_12:
     mov r15, 0
     end_61_12:
   push r15
   call assert
   add rsp, 165
mov rax, 60
mov rdi, 0
syscall
