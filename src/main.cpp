#include<fstream>
#include<memory>
#include<cstring>
using namespace std;
#include"program.hpp"
#include"compiler_error.hpp"
#include"call_exit.hpp"
#include"call_print.hpp"
#include"call_read.hpp"
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
		p=make_unique<program>(t);

		ofstream fo("diff.clare");
		p->source_to(fo);
		fo.close();
		if(file_read_to_string("prog.clare")!=file_read_to_string("diff.clare"))throw "generated source differs";

		p->build(cout);
	}catch(compiler_error&e){
		cout<<" *** error at "<<e.start_char<<":"<<e.end_char<<"  "<<e.msg<<": "<<e.ident<<endl;
		return 1;
	}catch(const char*msg){
		cout<<" *** exception: "<<msg<<endl;
		return 1;
	}catch(...){
		cout<<" *** exception caught"<<endl;
		return 1;
	}
	return 0;
}
inline up_statement create_statement(const char*funcname,statement*parent,up_token tk,tokenizer&t){
	if(!strcmp("exit",funcname))return make_unique<call_exit>(parent,move(tk),t);
	if(!strcmp("print",funcname))return make_unique<call_print>(parent,move(tk),t);
	if(!strcmp("read",funcname))return make_unique<call_read>(parent,move(tk),t);
	return make_unique<statement>(parent,move(tk));
}
