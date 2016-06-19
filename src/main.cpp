#include<vector>
using namespace std;
#include"string_tokenizer.hpp"
#include"statement.hpp"
#include"statement_exit.hpp"
#include"statement_print.hpp"
int main(){
	string_tokenizer br{R"(
    print()
    exit()
)"};

	vector<unique_ptr<statement>>statements;
	while(true){
		if(br.is_eos())break;
		unique_ptr<token>tk=br.next_token();
		if(tk->is_name("exit")){
			unique_ptr<statement_exit>stmt=make_unique<statement_exit>(move(tk),br);
			statements.push_back(move(stmt));
		}else if(tk->is_name("print")){
			unique_ptr<statement_print>stmt=make_unique<statement_print>(move(tk),br);
			statements.push_back(move(stmt));
		}else{
			unique_ptr<statement>stmt=make_unique<statement>(move(tk));
			statements.push_back(move(stmt));
		}
	}
	for(auto&s:statements){
		s->source_to_stdout();
	}
//	puts("");
//	for(auto&s:tokens){
//		s.to_stdout2();
//	}
	printf("section .text\nglobal _start\n_start:\n");
	for(auto&s:statements){
		s->to_asm();
	}
	return 0;
}


//string_tokenizer br{R"(
//print("hello world!")
//
//mov     edx,msg.len                         ;message length
//mov     ecx,msg                             ;message to write
//mov     ebx,1                               ;file descriptor (stdout)
//mov     eax,4                               ;system call number (sys_write)
//int     0x80                                ;call kernel
//
//exit()
//mov     eax,1
//int     0x80
//
//msg     db  'Hello, world!',0xa
//msg.len equ $-msg
//)"};
