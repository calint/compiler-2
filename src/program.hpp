#pragma once
class program:public statement{
public:
	program(tokenizer&t):statement{nullptr,nullptr}{
		while(true){
			if(t.is_eos())break;
			up_token tk=t.next_token();
			up_statement stmt;
			if(tk->is_name("file")){
				stmt=make_unique<file>(this,move(tk),t);
			}else if(tk->is_name("func")){
				stmt=make_unique<func>(this,move(tk),t);
			}else{
				stmt=create_statement(tk->name(),this,move(tk),t);
			}
			statements.push_back(move(stmt));
		}
		compile(tc);
		link(tc);
	}
	void compile(toc&tc)override final{
		printf("section .text\nglobal _start\n_start:\n");
		for(auto&s:statements)s->compile(tc);
	}
	void source_to_stdout()override final{
		for(auto&s:statements)s->source_to_stdout();
	}
	const toc&get_toc()const{return tc;}
private:
	vup_statement statements;
	toc tc;
};
using up_program=unique_ptr<program>;
