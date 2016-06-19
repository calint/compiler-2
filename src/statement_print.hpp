#pragma once
#include"statement.hpp"
class statement_print:public statement{
public:
	statement_print(unique_ptr<token>t,tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())
			throw 1;
		tk=st.next_token();
		if(!st.is_next_char_expression_close())
			throw 2;
	}
	void source_to_stdout()override{
		statement::source_to_stdout();
		printf("(");
		tk->source_to_stdout();
		printf(")");
	}
	void compile_to_stdout(toc&tc)override{
		// alloc edx, ecx, ebx, eax
		printf("    mov     edx,%s.len\n",tk->get_name());// length of text
		printf("    mov     ecx,%s\n",tk->get_name());// pointer to text
		puts("    mov     ebx,1");// file descriptor (stdout)
		puts("    mov     eax,4");// system call number (sys_write)
		puts("    int     0x80");// call kernel
		// free edx, ecx, ebx, eax
	}
	void link(toc&tc)override{
		if(!tc.has_def(tk->get_name())){
			printf("%zu:%zu identifier '%s' not found\n",tk->get_nchar(),tk->get_nchar_end(),tk->get_name());
			tc.print_to_stdout();
			throw 1;
		}
	}


private:
	unique_ptr<token>tk;
};
