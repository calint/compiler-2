#include<vector>
#include<memory>
using namespace std;
#include "call_exit.hpp"
#include "call_print.hpp"
#include "call_read.hpp"
#include"tokenizer.hpp"
#include"statement.hpp"
#include"data.hpp"
#include"func.hpp"
using ustatementp=std::unique_ptr<statement>;
#include<vector>
#include"decouple.hpp"
using vustatementp=std::vector<ustatementp>;
int main(){
//	tokenizer br{R"(
//  print(read(hello),hello)
//  exit()
//  data hello{hello world\n}
//  data info{compiler to nasm for linux kernel\n}
//  data console(32KB,0xb8000){"hello"}
//  func exit(){
//     mov(eax,1)
//     int(0x80)
//  }
//  func read(dst):nread{
//    xor(eax)
//    xor(edi)
//    mov(esi,dst)
//    mov(edx,dst.len)
//    syscall
//    ret(edx)
//  }
//  func print(len,src){
//    mov(edx,len)
//    mov(ecx,src)
//    mov(ebx,1)
//    mov(eax,4)
//    int(0x80)
//  }
//)"};
	tokenizer br{R"(
  read(hello)
  print(hello)
  exit()
  
  data hello {hello world\n}
  data info {compiler to nasm for linux kernel\n}
  func buf.write(buf,len,src)
)"};

	vustatementp statements;
	while(true){
		if(br.is_eos())break;
		utokenp tk=br.next_token();
		ustatementp stmt;
		if(tk->is_name("data")){
			stmt=make_unique<data>(nullptr,move(tk),br);
		}else if(tk->is_name("func")){
			stmt=make_unique<func>(nullptr,move(tk),br);
		}else{
			stmt=create_call_func(tk->name(),nullptr,move(tk),br);
		}
		statements.push_back(move(stmt));
	}
	printf(">>>> source:\n");
	for(auto&s:statements){
		s->source_to_stdout();
	}
	printf(">>>> compiled:\n");
	printf("section .text\nglobal _start\n_start:\n");
	toc tc;
	try{
		for(auto&s:statements)s->compile(tc);
		for(auto&s:statements)s->link(tc);
	}catch(...){
		return 1;
	}
//	tc.print_to_stdout();
	return 0;
}
std::unique_ptr<statement>create_call_func(const char*funcname,statement*parent,utokenp tk,tokenizer&br){
	if(!strcmp("exit",funcname))return make_unique<call_exit>(parent,move(tk),br);
	if(!strcmp("print",funcname))return make_unique<call_print>(parent,move(tk),br);
	if(!strcmp("read",funcname))return make_unique<call_read>(parent,move(tk),br);
	return make_unique<statement>(parent,move(tk));
}
