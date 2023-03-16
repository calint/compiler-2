#include<cstring>
#include<fstream>
#include<cassert>
#include<sstream>

using namespace std;

#include"stmt_program.hpp"
#include"call_asm_mov.hpp"
#include"call_asm_int.hpp"
#include"call_asm_xor.hpp"
#include"call_asm_syscall.hpp"
#include"stmt_loop.hpp"
#include"stmt_if.hpp"

// called from stmt_block to solve circular dependencies with loop, if and calls
inline unique_ptr<statement>create_statement_from_tokenizer(token tk,tokenizer&t){
	if(tk.is_name("loop"))          return make_unique<stmt_loop>(move(tk),t);
	if(tk.is_name("if"))              return make_unique<stmt_if>(move(tk),t);
	if(tk.is_name("mov"))        return make_unique<call_asm_mov>(move(tk),t);
	if(tk.is_name("int"))        return make_unique<call_asm_int>(move(tk),t);
	if(tk.is_name("xor"))        return make_unique<call_asm_xor>(move(tk),t);
	if(tk.is_name("syscall"))return make_unique<call_asm_syscall>(move(tk),t);
	return                                 make_unique<stmt_call>(move(tk),t);
}

// called from expr_ops_list to solve circular dependencies with calls
inline unique_ptr<statement>create_statement_from_tokenizer(tokenizer&t){
	token tk=t.next_token();
	if(tk.is_name("#"))return make_unique<stmt_comment>(move(tk),t);// i.e.  print("hello") # comment
	if(t.is_peek_char('('))return create_statement_from_tokenizer(move(tk),t); // i.e.  f(...)
	return make_unique<statement>(move(tk));// i.e. 0x80, rax, identifiers
}

static string read_file_to_string(const char *filename){
	ifstream t{filename};
	if(!t.is_open())
		throw "cannot open file '"+string{filename}+"'";
	string str;
	t.seekg(0,ios::end);
	str.reserve(size_t(t.tellg()));
	t.seekg(0,ios::beg);
	str.assign(istreambuf_iterator<char>(t),istreambuf_iterator<char>());
	return str;
}

static string trim(string s){
	size_t start{0};
	size_t end{s.length()-1};
	while(start<end&&(s[start]==' '||s[start]=='\t')){
		start++;
	}
	while(end>start&&(s[end]==' '||s[end]=='\t')){
		end--;
	}
	return s.substr(start,end-start+1);
}

static vector<string>split(const string&s,char delimiter){
	vector<string>tokens;
	string tk;
	istringstream ts{s};
	while(getline(ts,tk,delimiter)){
		tokens.push_back(tk);
	}
	return tokens;
}

// example:
//   jmp cmp_13_26
//   cmp_13_26:
// to
//   cmp_13_26:
static string optimize_jumps_1(stringstream&ss){
	stringstream sso;
	regex rxjmp{R"(^\s*jmp\s+(.+)\s*$)"};
	regex rxlbl{R"(^\s*(.+):.*$)"};
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

// example:
//   jne cmp_14_26
//   jmp if_14_8_code
//   cmp_14_26:
// to
//   je if_14_8_code
//   cmp_14_26:
static string optimize_jumps_2(stringstream&ss){
	stringstream sso;
	while(true){
		string line1;
		getline(ss,line1);
		if(ss.eof())
			break;

		const string&jxx{trim(line1)};
		if(!jxx.starts_with("j")){
			sso<<line1<<endl;
			continue;
		}

		string line2;
		getline(ss,line2);
		const string&jmp{trim(line2)};
		if(!jmp.starts_with("jmp")){
			sso<<line1<<endl;
			sso<<line2<<endl;
			continue;
		}

		string line3;
		vector<string>comments;
		while(true){// read comments
			getline(ss,line3);
			if(line3.starts_with(';')){
				comments.push_back(line3);
				continue;
			}
			break;
		}
		
		string lbl{trim(line3)};
		if(!lbl.ends_with(':')){
			sso<<line1<<endl;
			sso<<line2<<endl;
			for(const auto&s:comments)sso<<s<<endl;
			sso<<line3<<endl;
			continue;
		}

		lbl.pop_back();
		vector<string>jxx_split=split(jxx,' ');
		if(jxx_split[1]!=lbl){
			sso<<line1<<endl;
			sso<<line2<<endl;
			for(const auto&s:comments)sso<<s<<endl;
			sso<<line3<<endl;
			continue;
		}
		//   jne cmp_14_26
		//   jmp if_14_8_code
		//   cmp_14_26:
		vector<string>jmp_split=split(jmp,' ');
		string jxx_inv;
		if(jxx_split[0]=="jne"){
			jxx_inv="je";
		}else if(jxx_split[0]=="je"){
			jxx_inv="jne";
		}else if(jxx_split[0]=="jg"){
			jxx_inv="jle";
		}else if(jxx_split[0]=="jge"){
			jxx_inv="jl";
		}else if(jxx_split[0]=="jl"){
			jxx_inv="jge";
		}else if(jxx_split[0]=="jle"){
			jxx_inv="jg";
		}else{
			sso<<line1<<endl;
			sso<<line2<<endl;
			for(const auto&s:comments)sso<<s<<endl;
			sso<<line3<<endl;
			continue;
		}
		//   je if_14_8_code
		//   cmp_14_26:
		const string&ws{line1.substr(0,line1.find_first_not_of(" \t\n\r\f\v"))};
		sso<<ws<<jxx_inv<<" "<<jmp_split[1]<<"  ; opt2"<<endl;
		for(const auto&s:comments)sso<<s<<endl;
		sso<<line3<<endl;
	}
	return sso.str();
}

int main(int argc,char*args[]){
	const char*src_file_name=argc==1?"prog.baz":args[1];
	string src=read_file_to_string(src_file_name);
	try{
		stmt_program p{src};
		ofstream fo{"diff.baz"};
		p.source_to(fo);
		fo.close();
		if(read_file_to_string(src_file_name)!=read_file_to_string("diff.baz"))
			throw string("generated source differs");
		// p.build(cout);
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
