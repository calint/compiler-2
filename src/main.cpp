#include"decouple.hpp"
#include"tokenizer.hpp"
#include"call_exit.hpp"
#include"call_read.hpp"
#include"call_print.hpp"
#include"data.hpp"
#include"func.hpp"
#include"program.hpp"
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
	tokenizer t{R"(
  read(hello)
  print(hello)
  exit()
  
  data hello {hello world\n}
  data info {compiler to nasm for linux kernel\n}
  func exit(){
  }
  func data.read(ident) {
    read(ident)
  }
)"};

	up_program p=make_unique<program>(t);
//	printf(">>>> source:\n");
//	p->source_to_stdout();
//	printf(">>>> compiled:\n");
	try{p->build();}catch(...){
		return 1;
	}
//	tc.print_to_stdout();
	return 0;
}
up_statement create_statement(const char*funcname,statement*parent,up_token tk,tokenizer&t){
	if(!strcmp("exit",funcname))return make_unique<call_exit>(parent,move(tk),t);
	if(!strcmp("print",funcname))return make_unique<call_print>(parent,move(tk),t);
	if(!strcmp("read",funcname))return make_unique<call_read>(parent,move(tk),t);
	return make_unique<statement>(parent,move(tk));
}
