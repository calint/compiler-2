#include<fstream>
#include<memory>
#include<cstring>
using namespace std;
#include"stmt_program.hpp"
#include"compiler_error.hpp"
#include"call_asm_mov.hpp"
#include"call_asm_int.hpp"
#include"call_asm_xor.hpp"
#include"call_asm_syscall.hpp"
#include"call_asm_add.hpp"
#include"stmt_loop.hpp"
#include"stmt_break.hpp"
#include"stmt_continue.hpp"
#include"call_asm_cmp.hpp"
#include"call_asm_je.hpp"
#include"call_asm_tag.hpp"
static string file_read_to_string(const char *filename){
	FILE*f=fopen(filename,"rb");
	if(!f)throw "cannot open file";
	string s;
	fseek(f,0,SEEK_END);
	s.resize(size_t(ftell(f)));
	rewind(f);
	fread(&s[0],1,s.size(),f);
	fclose(f);
	return(s);
}

int main(int argc,char**args){
	string s=file_read_to_string("prog.clare");
	tokenizer t{s.data()};
	up_program p;// to keep valid in case of exception
	try{
		p=make_unique<stmt_program>(t);
		ofstream fo("diff.clare");
		p->source_to(fo);
		fo.close();
		if(file_read_to_string("prog.clare")!=file_read_to_string("diff.clare"))throw "generated source differs";

		p->build(cout);
	}catch(compiler_error&e){
		cout<<" *** error at "<<e.start_char<<":"<<e.end_char<<"  "<<e.msg<<": "<<e.ident.get()<<endl;
		return 1;
	}catch(const char*msg){
		cout<<" *** exception: "<<msg<<endl;
		return 1;
	}catch(...){
		cout<<" *** exception"<<endl;
		return 1;
	}
	return 0;
}
inline up_statement create_call(const char*funcname,statement*parent,unique_ptr<token>tk,tokenizer&t){
	if(!strcmp("mov",funcname))return make_unique<call_asm_mov>(parent,move(tk),t);
	if(!strcmp("int",funcname))return make_unique<call_asm_int>(parent,move(tk),t);
	if(!strcmp("xor",funcname))return make_unique<call_asm_xor>(parent,move(tk),t);
	if(!strcmp("syscall",funcname))return make_unique<call_asm_syscall>(parent,move(tk),t);
	if(!strcmp("add",funcname))return make_unique<call_asm_add>(parent,move(tk),t);
	if(!strcmp("loop",funcname))return make_unique<stmt_loop>(parent,move(tk),t);
	if(!strcmp("break",funcname))return make_unique<stmt_break>(parent,move(tk),t);
	if(!strcmp("continue",funcname))return make_unique<stmt_continue>(parent,move(tk),t);
	if(!strcmp("tag",funcname))return make_unique<call_asm_tag>(parent,move(tk),t);
	if(!strcmp("cmp",funcname))return make_unique<call_asm_cmp>(parent,move(tk),t);
	if(!strcmp("je",funcname))return make_unique<call_asm_je>(parent,move(tk),t);
	return make_unique<stmt_call>(parent,move(tk),t);
}
