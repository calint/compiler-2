#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "call_exit.hpp"
#include "call_print.hpp"
#include "call_read.hpp"
#include "compiler_error.hpp"
#include "program.hpp"
#include "statement.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

static string file_read_to_string(const char *filename){
	FILE*f=fopen(filename,"rb");
	if(!f)throw 1;
	string s;
	fseek(f,0,SEEK_END);
	s.resize(ftell(f));
	rewind(f);
	fread(&s[0],1,s.size(),f);
	fclose(f);
	return(s);
}

int main(){
	string s=file_read_to_string("prog.clare");
	tokenizer t{s.data()};
	try{
		up_program p=make_unique<program>(t);

//		ofstream fo("diff.clare");
//		p->source_to(fo);
//		fo.close();
//		if(file_read_to_string("prog.clare")!=file_read_to_string("diff.clare")){
//			throw 1;
//		}
		p->build(cout);
	}catch(compiler_error&e){
		cout<<" *** compiler error at "<<e.start_char<<":"<<e.end_char<<"  "<<e.msg<<endl;
		return 1;
	}catch(...){
		return 2;
	}
	return 0;
}
up_statement create_statement(const char*funcname,statement*parent,up_token tk,tokenizer&t){
	if(!strcmp("exit",funcname))return make_unique<call_exit>(parent,move(tk),t);
	if(!strcmp("print",funcname))return make_unique<call_print>(parent,move(tk),t);
	if(!strcmp("read",funcname))return make_unique<call_read>(parent,move(tk),t);
	return make_unique<statement>(parent,move(tk));
}


//try{up_program p=make_unique<program>(t);
////		printf(">>>> source:\n");
////		p->source_to_stdout();

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
//    zero(eax)
//    zero(edi)
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
//tokenizer t{R"(
//read(hello)
//print(hello)
//exit()
//
//file hello {hello world\n}
//file info {compiler to nasm for linux kernel\n}
//func exit(){
//}
//func file.read(ident) {
//}
//)"};
