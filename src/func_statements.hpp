#pragma once
class func_statements:public statement{
public:
	func_statements(statement*parent,tokenizer&st):statement{parent,nullptr}{
		ws=st.next_token();
		assert(st.is_next_char_block_open());
		while(true){
			if(st.is_eos())throw 1;
			if(st.is_next_char_block_close())break;
			up_token t=st.next_token();
			up_statement stmt=create_statement(t->name(),parent,move(t),st);
			statements.push_back(move(stmt));
		}
	}
	void compile(toc&tc)override{
		for(auto&s:statements){
			s->compile(tc);
		}
	}
	void source_to_stdout()override{
		ws->source_to_stdout();
		printf("{");
		for(auto&s:statements){
			s->source_to_stdout();
		}
		printf("}");
	}

private:
	up_token ws;
	vup_statement statements;
};
using up_func_statements=unique_ptr<func_statements>;
