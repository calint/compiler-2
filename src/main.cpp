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
#include"stmt_break.hpp"
#include"stmt_continue.hpp"
#include"stmt_if.hpp"
#include"stmt_return.hpp"

static string file_read_to_string(const char *filename){
	ifstream t(filename);
	string str;
	t.seekg(0,ios::end);
	str.reserve(size_t(t.tellg()));
	t.seekg(0,ios::beg);
	str.assign(istreambuf_iterator<char>(t),istreambuf_iterator<char>());
	return str;
}

static string trim(string s){
	size_t start=0;
	size_t end=s.length()-1;
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
	string token;
	istringstream tokenStream(s);
	while(getline(tokenStream,token,delimiter)){
		tokens.push_back(token);
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
	while(true){
		string line1;
		getline(ss,line1);
		if(ss.eof())
			break;
		string jmp=trim(line1);
		if(!jmp.starts_with("jmp")){
			sso<<line1<<endl;
			continue;
		}
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
		string lbl=trim(line2);
		if(!lbl.ends_with(':')){
			sso<<line1<<endl;
			for(const auto&s:comments)sso<<s<<endl;
			sso<<line2<<endl;
			continue;
		}
		lbl.pop_back();
		vector<string>jxx_split=split(jmp,' ');
		if(jxx_split[1]!=lbl){
			sso<<line1<<endl;
			for(const auto&s:comments)sso<<s<<endl;
			sso<<line2<<endl;
			continue;
		}
		// write the label without the preceding jmp
		for(const auto&s:comments)sso<<s<<endl;
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
		string jxx=trim(line1);
		if(!jxx.starts_with("j")){
			sso<<line1<<endl;
			continue;
		}
		string line2;
		getline(ss,line2);
		string jmp=trim(line2);
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
		string lbl=trim(line3);
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
		const string ws=line1.substr(0,line1.find_first_not_of(" \t\n\r\f\v"));
		sso<<ws<<jxx_inv<<" "<<jmp_split[1]<<"  ; opt2"<<endl;
		for(const auto&s:comments)sso<<s<<endl;
		sso<<line3<<endl;
	}
	return sso.str();
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
//		stringstream ss1;
//		p.build(ss1);
//		string pass1=optimize_jumps_1(ss1);
//		stringstream ss2{pass1};
//		string pass2=optimize_jumps_2(ss2);
//		cout<<pass2<<endl;
		p.build(cout);
	}catch(compiler_error&e){
		size_t start_char_in_line{0};
		auto lineno=toc::line_number_for_char_index(e.start_char,src.c_str(),start_char_in_line);
		cout<<"\n"<<src_file_name<<":"<<lineno<<":"<<start_char_in_line<<": "<<e.msg<<endl;
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

// called from stmt_block to solve circular dependencies with loop, if and calls
inline unique_ptr<statement>create_statement_from_tokenizer(const statement&parent,const token&tk,tokenizer&t){
	const string&func=tk.name();
	if("mov"==func)        return make_unique<call_asm_mov>(parent,move(tk),t);
	if("int"==func)        return make_unique<call_asm_int>(parent,move(tk),t);
	if("xor"==func)        return make_unique<call_asm_xor>(parent,move(tk),t);
	if("syscall"==func)return make_unique<call_asm_syscall>(parent,move(tk),t);
	if("loop"==func)          return make_unique<stmt_loop>(parent,move(tk),t);
	if("if"==func)              return make_unique<stmt_if>(parent,move(tk),t);
	return                           make_unique<stmt_call>(parent,move(tk),t);
}

// called from expr_ops_list to solve circular dependencies with calls
inline unique_ptr<statement>create_statement_from_tokenizer(const statement&parent,tokenizer&t){
	const token&tk=t.next_token();
	if(tk.is_name("#"))return make_unique<stmt_comment>(parent,move(tk),t);// i.e.  print("hello") # comment
	if(t.is_peek_char('('))return create_statement_from_tokenizer(parent,move(tk),t); // i.e.  f(...)
	return make_unique<statement>(parent,move(tk));// i.e. 0x80, rax, identifiers
}

