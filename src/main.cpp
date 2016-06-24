#include <stddef.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "statement.hpp"
#include "stmt_assign_var.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

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
#include"call_asm_jmp.hpp"
#include"call_asm_jne.hpp"
#include"stmt_if.hpp"
#include"call_asm_cmove.hpp"
#include"call_asm_cmovne.hpp"
#include"call_asm_or.hpp"
#include"call_asm_and.hpp"
#include"stmt_def_var.hpp"
#include"stmt_def_field.hpp"
#include"stmt_def_func.hpp"
#include"stmt_def_class.hpp"
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
inline unique_ptr<statement>create_call(const char*funcname,statement*parent,unique_ptr<token>tk,tokenizer&t){
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
	if(!strcmp("jmp",funcname))return make_unique<call_asm_jmp>(parent,move(tk),t);
	if(!strcmp("jne",funcname))return make_unique<call_asm_jne>(parent,move(tk),t);
	if(!strcmp("if",funcname))return make_unique<stmt_if>(parent,move(tk),t);
	if(!strcmp("cmove",funcname))return make_unique<call_asm_cmove>(parent,move(tk),t);
	if(!strcmp("cmovne",funcname))return make_unique<call_asm_cmovne>(parent,move(tk),t);
	if(!strcmp("or",funcname))return make_unique<call_asm_or>(parent,move(tk),t);
	if(!strcmp("and",funcname))return make_unique<call_asm_and>(parent,move(tk),t);
	return make_unique<stmt_call>(parent,move(tk),t);
}

inline unique_ptr<statement>read_next_statement(statement*parent,tokenizer&t){
	unique_ptr<class token>tk=t.next_token();
	if(tk->is_name(""))return make_unique<statement>(parent,move(tk));
	if(tk->is_name("var"))return make_unique<stmt_def_var>(parent,move(tk),t);
	if(t.is_next_char('='))return make_unique<stmt_assign_var>(parent,move(tk),t);
	if(tk->is_name("field"))return make_unique<stmt_def_field>(parent,move(tk),t);
	if(tk->is_name("func"))return make_unique<stmt_def_func>(parent,move(tk),t);
	if(t.is_next_char('{'))return make_unique<stmt_def_class>(parent,move(tk),t);
	if(tk->is_name(""))return make_unique<statement>(parent,move(tk));

	const char*funcname=tk->name();
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
	if(!strcmp("jmp",funcname))return make_unique<call_asm_jmp>(parent,move(tk),t);
	if(!strcmp("jne",funcname))return make_unique<call_asm_jne>(parent,move(tk),t);
	if(!strcmp("if",funcname))return make_unique<stmt_if>(parent,move(tk),t);
	if(!strcmp("cmove",funcname))return make_unique<call_asm_cmove>(parent,move(tk),t);
	if(!strcmp("cmovne",funcname))return make_unique<call_asm_cmovne>(parent,move(tk),t);
	if(!strcmp("or",funcname))return make_unique<call_asm_or>(parent,move(tk),t);
	if(!strcmp("and",funcname))return make_unique<call_asm_and>(parent,move(tk),t);
	return make_unique<stmt_call>(parent,move(tk),t);

}
