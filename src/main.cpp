#include<cstring>
#include<fstream>
#include<cassert>

using namespace std;

#include"stmt_program.hpp"
#include"call_asm_mov.hpp"
#include"call_asm_int.hpp"
#include"call_asm_xor.hpp"
#include"call_asm_syscall.hpp"
#include"stmt_loop.hpp"
#include"stmt_break.hpp"
#include"stmt_continue.hpp"
#include"stmt_if.hpp"

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
	auto src_file_name=argc==1?"prog.baz":args[1];
	auto src=file_read_to_string(src_file_name);

	try{
		stmt_program p{src};

		ofstream fo{"diff.baz"};

		p.source_to(fo);

		fo.close();

		if(file_read_to_string(src_file_name)!=file_read_to_string("diff.baz"))
			throw string("generated source differs");

		p.build(cout);

	}catch(compiler_error&e){
		size_t start_char_in_line{0};
		auto lineno=line_number_for_char_index(e.start_char,src.c_str(),start_char_in_line);
		cout<<"\n"<<src_file_name<<":"<<lineno<<":"<<start_char_in_line<<": "<<e.msg<<" "<<e.ident<<endl;
		return 1;
	}catch(string&s){
		cout<<"\nexception: "<<s<<endl;
		return 1;
	}catch(...){
		cout<<"\nexception"<<endl;
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
	if("loop"==func)          return make_unique<stmt_loop>(parent,move(tk),t);
	if("break"==func)        return make_unique<stmt_break>(parent,move(tk),t);
	if("continue"==func)  return make_unique<stmt_continue>(parent,move(tk),t);
	if("if"==func)              return make_unique<stmt_if>(parent,move(tk),t);
	return                           make_unique<stmt_call>(parent,move(tk),t);
}


inline unique_ptr<statement>create_statement_from_tokenizer(const statement&parent,tokenizer&t){
	auto tk=t.next_token();
	if(tk.is_name("#"))return make_unique<stmt_comment>(parent,move(tk),t);// i.e.  print("hello") # comment
	if(t.is_peek_char('('))return create_call_statement_from_tokenizer(parent,move(tk),t); // ie  f(...)
	return make_unique<statement>(parent,move(tk));// ie  0x80
}

