#pragma once
#include"statement.hpp"
#include<vector>
using allocs=std::vector<const char*>;
class statement_print:public statement{
public:
	statement_print(utokenp t,tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())throw 1;
		tk=st.next_token();
		if(!st.is_next_char_expression_close())throw 2;
	}
	void print_source()override{
		statement::print_source();
		printf("(");
		tk->source_to_stdout();
		printf(")");
	}
	void compile(toc&tc)override{
		allocs regs{"edx","ecx","ebx","eax"};
		printf("  mov edx,%s.len\n",tk->get_name());// length of text
		printf("  mov ecx,%s\n",tk->get_name());// pointer to text
		printf("  mov ebx,1\n");// file descriptor (stdout)
		printf("  mov eax,4\n");// system call number (sys_write)
		printf("  int 0x80\n");// call kernel
		// allocs.free()
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
