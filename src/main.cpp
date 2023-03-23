#include<cstring>
#include<fstream>
#include<cassert>
#include<sstream>

using namespace std;

#include"stmt_program.hpp"
#include"call_asm_mov.hpp"
#include"call_asm_syscall.hpp"
#include"stmt_loop.hpp"
#include"stmt_if.hpp"

// called from stmt_block to solve circular dependencies with loop, if and calls
inline unique_ptr<statement>create_statement_from_tokenizer(token tk,unary_ops uops,tokenizer&t){
	if(tk.is_name("loop"))          return make_unique<stmt_loop>(move(tk),t);
	if(tk.is_name("if"))              return make_unique<stmt_if>(move(tk),t);
	if(tk.is_name("mov"))        return make_unique<call_asm_mov>(move(tk),t);
	if(tk.is_name("syscall"))return make_unique<call_asm_syscall>(move(tk),t);
	return                      make_unique<stmt_call>(move(tk),move(uops),t);
}

// called from expr_ops_list to solve circular dependencies with calls
inline unique_ptr<statement>create_statement_from_tokenizer(tokenizer&t){
	unary_ops uops{t};
	token tk=t.next_token();
	if(tk.is_name(""))
		throw compiler_error(tk,"unexpected empty expression");

	if(tk.is_name("#")){
		if(!uops.is_empty())
			throw compiler_error(tk,"unexpected comment after unary ops");
		// i.e.  print("hello") # comment
		return make_unique<stmt_comment>(move(tk),t);
	}else if(t.is_peek_char('(')){
		// i.e.  f(...)
		return create_statement_from_tokenizer(move(tk),move(uops),t);
	}else{
		// i.e. 0x80, rax, identifiers
		return make_unique<statement>(move(tk),move(uops));
	}
}

// solves circular reference unary_ops->toc->statement->unary_ops
inline void unary_ops::compile(toc&tc,ostream&os,size_t indent_level,const string&dst_resolved)const{
	size_t i{ops_.size()};
	while(i--){
		tc.indent(os,indent_level,false);
		const char op{ops_[i]};
		if(op=='~'){
			os<<"not "<<dst_resolved<<endl;
		}else if(op=='-'){
			os<<"neg "<<dst_resolved<<endl;
		}else{
			throw"unexpected "+string{__FILE__}+":"+string{__LINE__};
		}
	}
}

// opt1
// example:
//   jmp cmp_13_26
//   cmp_13_26:
// to
//   cmp_13_26:
static string optimize_jumps_1(stringstream&ss){
	stringstream sso;
	const regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
	const regex rxlbl{R"(^\s*(.+):.*$)"};
	smatch match;
	while(true){
		string line1;
		getline(ss,line1);
		if(ss.eof())
			break;

		if(!regex_search(line1,match,rxjmp)){
			sso<<line1<<endl;
			continue;
		}
		const string&jmplbl{match[1]};

		string line2;
		vector<string>comments;
		while(true){// read comments
			getline(ss,line2);
			if(line2.starts_with(';')){
				comments.push_back(line2);
				continue;
			}
			break;
		}

		if(!regex_search(line2,match,rxlbl)){
			sso<<line1<<endl;
			for(const string&s:comments)sso<<s<<endl;
			sso<<line2<<endl;
			continue;
		}

		const string&lbl{match[1]};

		if(jmplbl!=lbl){
			sso<<line1<<endl;
			for(const auto&s:comments)sso<<s<<endl;
			sso<<line2<<endl;
			continue;
		}

		// write the label without the preceding jmp
		for(const string&s:comments)sso<<s<<endl;
		sso<<line2<<"  ; opt1"<<endl;
	}
	return sso.str();
}

// opt2
// example:
//   jne cmp_14_26
//   jmp if_14_8_code
//   cmp_14_26:
// to
//   je if_14_8_code
//   cmp_14_26:
static string optimize_jumps_2(stringstream&ss){
	stringstream sso;
	const regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
	const regex rxjxx{R"(^\s*(j[a-z][a-z]?)\s+(.+)\s*$)"};
	const regex rxlbl{R"(^\s*(.+):.*$)"};
	const regex rxcomment{R"(^\s*;.*$)"};
	smatch match;

	while(true){
		string line1;
		getline(ss,line1);
		if(ss.eof())
			break;

		if(!regex_search(line1,match,rxjxx)){
			sso<<line1<<endl;
			continue;
		}
		const string&jxx{match[1]};
		const string&jxxlbl{match[2]};

		string line2;
		vector<string>comments2;
		while(true){// read comments
			getline(ss,line2);
			if(regex_match(line2,rxcomment)){
				comments2.push_back(line2);
				continue;
			}
			break;
		}
		if(!regex_search(line2,match,rxjmp)){
			sso<<line1<<endl;
			for(const auto&s:comments2)sso<<s<<endl;
			sso<<line2<<endl;
			continue;
		}
		const string&jmplbl{match[1]};

		string line3;
		vector<string>comments3;
		while(true){// read comments
			getline(ss,line3);
			if(regex_match(line3,rxcomment)){
				comments3.push_back(line3);
				continue;
			}
			break;
		}

		if(!regex_search(line3,match,rxlbl)){
			sso<<line1<<endl;
			for(const auto&s:comments2)sso<<s<<endl;
			sso<<line2<<endl;
			for(const auto&s:comments3)sso<<s<<endl;
			sso<<line3<<endl;
			continue;
		}
		string lbl{match[1]};

		if(jxxlbl!=lbl){
			sso<<line1<<endl;
			for(const auto&s:comments2)sso<<s<<endl;
			sso<<line2<<endl;
			for(const auto&s:comments3)sso<<s<<endl;
			sso<<line3<<endl;
			continue;
		}

		//   jne cmp_14_26
		//   jmp if_14_8_code
		//   cmp_14_26:
		string jxx_inv;
		if(jxx=="jne"){
			jxx_inv="je";
		}else if(jxx=="je"){
			jxx_inv="jne";
		}else if(jxx=="jg"){
			jxx_inv="jle";
		}else if(jxx=="jge"){
			jxx_inv="jl";
		}else if(jxx=="jl"){
			jxx_inv="jge";
		}else if(jxx=="jle"){
			jxx_inv="jg";
		}else{
			sso<<line1<<endl;
			for(const auto&s:comments2)sso<<s<<endl;
			sso<<line2<<endl;
			for(const auto&s:comments3)sso<<s<<endl;
			sso<<line3<<endl;
			continue;
		}
		//   je if_14_8_code
		//   cmp_14_26:
		const string&ws{line1.substr(0,line1.find_first_not_of(" \t\n\r\f\v"))};
		for(const auto&s:comments2)sso<<s<<endl;
		sso<<ws<<jxx_inv<<" "<<jmplbl<<"  ; opt2"<<endl;
		for(const auto&s:comments3)sso<<s<<endl;
		sso<<line3<<endl;
	}
	return sso.str();
}

static string read_file_to_string(const char *file_name){
	ifstream t{file_name};
	if(!t.is_open())
		throw"cannot open file '"+string{file_name}+"'";
	string str;
	t.seekg(0,ios::end);
	str.reserve(size_t(t.tellg()));
	t.seekg(0,ios::beg);
	str.assign(istreambuf_iterator<char>(t),istreambuf_iterator<char>());
	return str;
}

int main(int argc,char*args[]){
	const char*src_file_name=argc==1?"prog.baz":args[1];
	string src;
	try{
		src=read_file_to_string(src_file_name);
		stmt_program p{src};
		ofstream fo{"diff.baz"};
		p.source_to(fo);
		fo.close();
		if(read_file_to_string(src_file_name)!=read_file_to_string("diff.baz"))
			throw"generated source differs. diff "+string{src_file_name}+" diff.baz";

//		 p.build(cout);

		stringstream ss1;
		p.build(ss1);
		const string&pass1{optimize_jumps_1(ss1)};
		stringstream ss2{pass1};
		const string&pass2{optimize_jumps_2(ss2)};
		cout<<pass2<<endl;
	}catch(const compiler_error&e){
		size_t start_char_in_line{0};
		const size_t lineno{toc::line_number_for_char_index(e.start_char,src.c_str(),start_char_in_line)};
		cerr<<"\n"<<src_file_name<<":"<<lineno<<":"<<start_char_in_line<<": "<<e.msg<<endl;
		return 1;
	}catch(const string&s){
		cerr<<"\nexception: "<<s<<endl;
		return 1;
	}catch(...){
		cerr<<"\nexception"<<endl;
		return 1;
	}
	return 0;
}
