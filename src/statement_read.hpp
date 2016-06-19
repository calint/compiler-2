#pragma once
#include"statement.hpp"
using allocs=vector<const char*>;
class statement_read:public statement{
public:
	statement_read(unique_ptr<token>t,tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())
			throw 1;
		tk=st.next_token();
		if(!st.is_next_char_expression_close())
			throw 2;
	}
	void print_source()override{
		statement::print_source();
		printf("(");
		tk->source_to_stdout();
		printf(")");
	}
	void compile(toc&tc)override{
		allocs all{"eax","edi","esi","edx"};
		printf("  xor eax,eax\n");// 0 (write syscall number)
		printf("  xor edi,edi\n");// 0 (stdin file descriptor)
		printf("  mov esi,%s\n",tk->get_name());// address of the buffer
		printf("  mov edx,%s.len\n",tk->get_name());// size of the buffer
		printf("  syscall\n");// call kernel
		// allocs.free
	}
	void link(toc&tc)override{
		if(!tc.has_def(tk->get_name())){
			printf("%zu:%zu identifier '%s' not found\n",tk->get_nchar(),tk->get_nchar_end(),tk->get_name());
			tc.print_to_stdout();
			throw 1;
		}
	}

private:
	utokenp tk;
};
