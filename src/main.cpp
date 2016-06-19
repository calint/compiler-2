#include<vector>
#include<memory>
using namespace std;
#include "call_exit.hpp"
#include "call_print.hpp"
#include "call_read.hpp"
#include"tokenizer.hpp"
#include"statement.hpp"
#include"def.hpp"
using ustatementp=std::unique_ptr<statement>;
#include<vector>
using vustatementp=std::vector<ustatementp>;
int main(){
	tokenizer br{R"(
    print(hello)
    read(hello)
    print(hello)
    print(info)
    exit()

    def hello{hello world\n}
    def info{compiler to nasm for linux kernel\n}
)"};
	vustatementp statements;
	while(true){
		if(br.is_eos())break;
		utokenp tk=br.next_token();
		ustatementp stmt;
		if(tk->is_name("exit")){
			stmt=make_unique<call_exit>(move(tk),br);
		}else if(tk->is_name("print")){
			stmt=make_unique<call_print>(move(tk),br);
		}else if(tk->is_name("def")){
			stmt=make_unique<def>(move(tk),br);
		}else if(tk->is_name("read")){
			stmt=make_unique<call_read>(move(tk),br);
		}else{
			stmt=make_unique<statement>(move(tk));
		}
		statements.push_back(move(stmt));
	}
//	printf(">>>> source:\n");
//	for(auto&s:statements){
//		s->source_to_stdout();
//	}
//	printf(">>>> compiled:\n");
	printf("section .text\nglobal _start\n_start:\n");
	toc tc;
	try{
		for(auto&s:statements)s->compile(tc);
		for(auto&s:statements)s->link(tc);
	}catch(...){
		return 1;
	}
//	tc.print_to_stdout();
	return 0;
}
