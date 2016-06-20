#pragma once
class func_statements:public statement{public:
	func_statements(statement*parent,tokenizer&t):statement{parent,t.next_token()}{
		assert(t.is_next_char_block_open());
		while(true){
			if(t.is_eos())throw 1;
			if(t.is_next_char_block_close())break;
			up_token tkn=t.next_token();
			up_statement stmt=create_statement(tkn->name(),parent,move(tkn),t);
			statements.push_back(move(stmt));
		}
	}
	void compile(toc&tc)override{
		for(auto&s:statements)s->compile(tc);
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		printf("{");
		for(auto&s:statements)s->source_to_stdout();
		printf("}");
	}

private:
	vup_statement statements;
};
using up_func_statements=unique_ptr<func_statements>;
