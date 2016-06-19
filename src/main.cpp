#include<vector>
using namespace std;
#include"string_tokenizer.hpp"
#include"statement.hpp"
#include"statement_exit.hpp"
#include"statement_print.hpp"
#include"statement_def.hpp"
int main(){
	string_tokenizer br{R"(
    print(hello)
    print(info)
    exit()

    def hello{ "hello_world" }
    def info{ "compiler_to_nasm_for_linux_kernel" }
)"};

	vector<unique_ptr<statement>>statements;
	while(true){
		if(br.is_eos())break;
		unique_ptr<token>tk=br.next_token();
		if(tk->is_name("exit")){
			unique_ptr<statement_exit>stmt=make_unique<statement_exit>(move(tk),br);
			statements.push_back(move(stmt));
		}else if(tk->is_name("print")){
			unique_ptr<statement_print>stmt=make_unique<statement_print>(move(tk),br);
			statements.push_back(move(stmt));
		}else if(tk->is_name("def")){
			unique_ptr<statement_def>stmt=make_unique<statement_def>(move(tk),br);
			statements.push_back(move(stmt));
		}else{
			unique_ptr<statement>stmt=make_unique<statement>(move(tk));
			statements.push_back(move(stmt));
		}
	}
//	printf(">>>> source:\n");
//	for(auto&s:statements){
//		s->source_to_stdout();
//	}
//	printf(">>>> compiled:\n");
	printf("section .text\nglobal _start\n_start:\n");
	for(auto&s:statements){
		s->compiled_to_stdout();
	}
	return 0;
}
