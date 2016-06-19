#include<vector>
using namespace std;
#include "string_tokenizer.hpp"
int main(){
	string_tokenizer br{R"(
section     .text
global      _start                              ;must be declared for linker (ld)

_start:                                         ;tell linker entry point

    mov     edx,msg.len                             ;message length
    mov     ecx,msg                             ;message to write
    mov     ebx,1                               ;file descriptor (stdout)
    mov     eax,4                               ;system call number (sys_write)
    int     0x80                                ;call kernel

    mov     eax,1                               ;system call number (sys_exit)
    int     0x80                                ;call kernel

section     .data

msg     db  'Hello, world!',0xa                 ;our dear string
msg.len equ $ - msg                             ;length of our dear string
)"};

	vector<token>tokens;
	while(true){
		if(br.is_eos())break;
		tokens.push_back(br.next_token());
	}
	for(auto&s:tokens){
		s.to_stdout();
	}
//	puts("");
//	for(auto&s:tokens){
//		s.to_stdout2();
//	}
//	for(auto&s:tokens){
//		s.to_asm();
//	}
	return 0;
}
