#include<cstring>
#include<streambuf>
#include<fstream>
#include<memory>
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
	ifstream t(filename);
	string str;
	t.seekg(0,ios::end);
	str.reserve(size_t(t.tellg()));
	t.seekg(0,ios::beg);
	str.assign(istreambuf_iterator<char>(t),istreambuf_iterator<char>());
	return str;
}

static size_t line_number_for_char_index(size_t ix,const char*str,size_t&start_char_in_line){
	size_t lineno{1};
	const char*start_of_line_ptr{str};
	while(ix--){
		if(*str++=='\n'){
			start_of_line_ptr=str;
			lineno++;
		}
	}
	assert(str>=start_of_line_ptr);
	start_char_in_line=size_t(str-start_of_line_ptr);
	return lineno;
}

int main(int argc,char**args){

	auto src=file_read_to_string("prog.baz");

	try{
		stmt_program p(src);

		ofstream fo("diff.baz");

		p.source_to(fo);

		fo.close();

		if(file_read_to_string("prog.baz")!=file_read_to_string("diff.baz"))
			throw string("generated source differs");

		p.build(cout);

	}catch(compiler_error&e){
		size_t start_char_in_line{0};
		auto lineno=line_number_for_char_index(e.start_char,src.c_str(),start_char_in_line);
		cout<<"\n\n **  ["<<lineno<<":"<<start_char_in_line<<"]  "<<e.msg<<": "<<e.ident<<endl;
		return 1;
	}catch(string&s){
		cout<<"\n\n ** exception: "<<s<<endl;
		return 1;
	}catch(...){
		cout<<" *** exception"<<endl;
		return 1;
	}
	return 0;
}


inline unique_ptr<statement>create_call_statement_from_tokenizer(const statement&parent,const token&tk,tokenizer&t){
	const string&func=tk.name();
	if("mov"==func)        return make_unique<call_asm_mov>(parent,move(tk),t);
	if("int"==func)        return make_unique<call_asm_int>(parent,move(tk),t);
	if("xor"==func)        return make_unique<call_asm_xor>(parent,move(tk),t);
	if("syscall"==func)return make_unique<call_asm_syscall>(parent,move(tk),t);
	if("add"==func)        return make_unique<call_asm_add>(parent,move(tk),t);
	if("loop"==func)          return make_unique<stmt_loop>(parent,move(tk),t);
	if("break"==func)        return make_unique<stmt_break>(parent,move(tk),t);
	if("continue"==func)  return make_unique<stmt_continue>(parent,move(tk),t);
	if("tag"==func)        return make_unique<call_asm_tag>(parent,move(tk),t);
	if("cmp"==func)        return make_unique<call_asm_cmp>(parent,move(tk),t);
	if("je"==func)          return make_unique<call_asm_je>(parent,move(tk),t);
	if("jmp"==func)        return make_unique<call_asm_jmp>(parent,move(tk),t);
	if("jne"==func)        return make_unique<call_asm_jne>(parent,move(tk),t);
	if("if"==func)              return make_unique<stmt_if>(parent,move(tk),t);
	if("cmove"==func)    return make_unique<call_asm_cmove>(parent,move(tk),t);
	if("cmovne"==func)  return make_unique<call_asm_cmovne>(parent,move(tk),t);
	if("or"==func)          return make_unique<call_asm_or>(parent,move(tk),t);
	if("and"==func)        return make_unique<call_asm_and>(parent,move(tk),t);
	return                           make_unique<stmt_call>(parent,move(tk),t);
}


inline unique_ptr<statement>create_statement_from_tokenizer(const statement&parent,tokenizer&t){
	auto tk=t.next_token();
	if(tk.is_name("//"))return make_unique<stmt_comment>(parent,move(tk),t);// ie    print("hello") // comment
	if(!t.is_peek_char('('))return make_unique<statement>(parent,move(tk));// ie  0x80
	return create_call_statement_from_tokenizer(parent,move(tk),t); // ie  f(...)
}

//inline unique_ptr<expression>create_expression_from_tokenizer(const statement&parent,tokenizer&t){
//}

