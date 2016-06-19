#include<vector>
using namespace std;
#include"tokenizer.hpp"
#include"statement.hpp"
#include"statement_exit.hpp"
#include"statement_print.hpp"
#include"statement_def.hpp"
using ustatementp=unique_ptr<statement>;
using vustatementp=vector<unique_ptr<statement>>;
int main(){
	tokenizer br{R"(
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
			stmt=make_unique<statement_exit>(move(tk),br);
		}else if(tk->is_name("print")){
			stmt=make_unique<statement_print>(move(tk),br);
		}else if(tk->is_name("def")){
			stmt=make_unique<statement_def>(move(tk),br);
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
		for(auto&s:statements)s->compile_to_stdout(tc);
		for(auto&s:statements)s->link(tc);
	}catch(...){
		return 1;
	}
//	tc.print_to_stdout();
	return 0;
}
