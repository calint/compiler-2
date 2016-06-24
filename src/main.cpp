#include <stddef.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "def_block.hpp"
#include "stmt_assign_var.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

using namespace std;
#include"def_program.hpp"
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
#include"call_asm_jmp.hpp"
#include"call_asm_jne.hpp"
#include"stmt_if.hpp"
#include"call_asm_cmove.hpp"
#include"call_asm_cmovne.hpp"
#include"call_asm_or.hpp"
#include"call_asm_and.hpp"
#include"def_var.hpp"
#include"def_field.hpp"
#include"def_func.hpp"
#include"def_class.hpp"
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

static size_t line_number_for_char_index(size_t ix,const char*str){
	size_t lineno{1};
	while(ix--){
		if(*str++=='\n')
			lineno++;
	}
	return lineno;
}
int main(int argc,char**args){
	string s=file_read_to_string("prog.baz");
	tokenizer t{s.data()};
	unique_ptr<def_program>p;// to keep valid in case of exception
	try{
		p=make_unique<def_program>(t);
//		p->source_to(cout);

		ofstream fo("diff.baz");
		p->source_to(fo);
		fo.close();
		if(file_read_to_string("prog.baz")!=file_read_to_string("diff.baz"))
			throw "generated source differs";

		p->build(cout);

		cout<<endl;
	}catch(compiler_error&e){
		const size_t lineno=line_number_for_char_index(e.start_char,s.data());
		cout<<" *** error at "<<lineno<<":"<<e.start_char<<":"<<e.end_char<<"  "<<e.msg<<": "<<e.ident.get()<<endl;
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

inline unique_ptr<statement>read_next_statement(toc&tc,statement*parent,tokenizer&t){
	if(t.is_next_char('{')){
		t.unread();
		return make_unique<def_block>(tc,parent,t.next_token(),t);
	}

	unique_ptr<class token>tkn=t.next_token();
	if(tkn->is_name(""))return make_unique<statement>(tc,parent,move(tkn));
	if(tkn->is_name("class"))return make_unique<def_class>(tc,parent,move(tkn),t);
	if(tkn->is_name("field"))return make_unique<def_field>(tc,parent,move(tkn),t);
	if(tkn->is_name("func"))return make_unique<def_func>(tc,parent,move(tkn),t);
	if(tkn->is_name("var"))return make_unique<def_var>(tc,parent,move(tkn),t);
	if(t.is_next_char('='))return make_unique<stmt_assign_var>(tc,parent,move(tkn),t);

	const char*funcname=tkn->name();
	if(!strcmp("mov",funcname))return make_unique<call_asm_mov>(tc,parent,move(tkn),t);
	if(!strcmp("int",funcname))return make_unique<call_asm_int>(tc,parent,move(tkn),t);
	if(!strcmp("xor",funcname))return make_unique<call_asm_xor>(tc,parent,move(tkn),t);
	if(!strcmp("syscall",funcname))return make_unique<call_asm_syscall>(tc,parent,move(tkn),t);
	if(!strcmp("add",funcname))return make_unique<call_asm_add>(tc,parent,move(tkn),t);
	if(!strcmp("loop",funcname))return make_unique<stmt_loop>(tc,parent,move(tkn),t);
	if(!strcmp("break",funcname))return make_unique<stmt_break>(tc,parent,move(tkn),t);
	if(!strcmp("continue",funcname))return make_unique<stmt_continue>(tc,parent,move(tkn),t);
	if(!strcmp("tag",funcname))return make_unique<call_asm_tag>(tc,parent,move(tkn),t);
	if(!strcmp("cmp",funcname))return make_unique<call_asm_cmp>(tc,parent,move(tkn),t);
	if(!strcmp("je",funcname))return make_unique<call_asm_je>(tc,parent,move(tkn),t);
	if(!strcmp("jmp",funcname))return make_unique<call_asm_jmp>(tc,parent,move(tkn),t);
	if(!strcmp("jne",funcname))return make_unique<call_asm_jne>(tc,parent,move(tkn),t);
	if(!strcmp("if",funcname))return make_unique<stmt_if>(tc,parent,move(tkn),t);
	if(!strcmp("cmove",funcname))return make_unique<call_asm_cmove>(tc,parent,move(tkn),t);
	if(!strcmp("cmovne",funcname))return make_unique<call_asm_cmovne>(tc,parent,move(tkn),t);
	if(!strcmp("or",funcname))return make_unique<call_asm_or>(tc,parent,move(tkn),t);
	if(!strcmp("and",funcname))return make_unique<call_asm_and>(tc,parent,move(tkn),t);

	return make_unique<stmt_call>(tc,parent,move(tkn),t);

}
