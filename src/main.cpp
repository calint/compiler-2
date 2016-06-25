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
#include"call_asm_jmp.hpp"
#include"call_asm_jne.hpp"
#include"stmt_if.hpp"
#include"call_asm_cmove.hpp"
#include"call_asm_cmovne.hpp"
#include"call_asm_or.hpp"
#include"call_asm_and.hpp"
#include"stmt_comment.hpp"
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

	auto src=file_read_to_string("prog.baz");

	try{

		auto p=stmt_program(src);

		auto fo=ofstream("diff.baz");

		p.source_to(fo);

		fo.close();

		if(file_read_to_string("prog.baz")!=file_read_to_string("diff.baz"))
			throw "generated source differs";

		p.build(cout);

	}catch(compiler_error&e){
		auto lineno=line_number_for_char_index(e.start_char,src.c_str());
		cout<<" *** error at "<<lineno<<":"<<e.start_char<<".."<<e.end_char<<"  "<<e.msg<<": "<<e.ident<<endl;
		return 1;
	}catch(string&s){
		cout<<" *** exception: "<<s<<endl;
		return 1;
	}
//	catch(...){
//		cout<<" *** exception"<<endl;
//		return 1;
//	}
	return 0;
}
inline up_statement create_call_statement_from_tokenizer(const string&func_name,statement&parent,const token&tk,tokenizer&t){
	if("mov"==func_name)return make_unique<call_asm_mov>(parent,tk,t);
	if("int"==func_name)return make_unique<call_asm_int>(parent,tk,t);
	if("xor"==func_name)return make_unique<call_asm_xor>(parent,tk,t);
	if("syscall"==func_name)return make_unique<call_asm_syscall>(parent,tk,t);
	if("add"==func_name)return make_unique<call_asm_add>(parent,tk,t);
	if("loop"==func_name)return make_unique<stmt_loop>(parent,tk,t);
	if("break"==func_name)return make_unique<stmt_break>(parent,tk,t);
	if("continue"==func_name)return make_unique<stmt_continue>(parent,tk,t);
	if("tag"==func_name)return make_unique<call_asm_tag>(parent,tk,t);
	if("cmp"==func_name)return make_unique<call_asm_cmp>(parent,tk,t);
	if("je"==func_name)return make_unique<call_asm_je>(parent,tk,t);
	if("jmp"==func_name)return make_unique<call_asm_jmp>(parent,tk,t);
	if("jne"==func_name)return make_unique<call_asm_jne>(parent,tk,t);
	if("if"==func_name)return make_unique<stmt_if>(parent,tk,t);
	if("cmove"==func_name)return make_unique<call_asm_cmove>(parent,tk,t);
	if("cmovne"==func_name)return make_unique<call_asm_cmovne>(parent,tk,t);
	if("or"==func_name)return make_unique<call_asm_or>(parent,tk,t);
	if("and"==func_name)return make_unique<call_asm_and>(parent,tk,t);
	return make_unique<stmt_call>(parent,move(tk),t);
}
inline up_statement create_statement_from_tokenizer(statement&parent,tokenizer&t){
	auto tk=t.next_token();
	if(tk.is_name("//"))return make_unique<stmt_comment>(parent,tk,t);// ie    print("hello") // comment
	if(!t.is_peek_char('('))return make_unique<statement>(parent,tk);// ie  0x80
	return create_call_statement_from_tokenizer(tk.name(),parent,tk,t); // ie  f(...)
}

