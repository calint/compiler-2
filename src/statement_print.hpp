#pragma once
#include"statement.hpp"
class statement_print:public statement{
public:
	inline statement_print(unique_ptr<token>t,string_tokenizer&st):statement(move(t)){
		if(!st.is_next_char_expression_open())
			throw 1;
		tk=st.next_token();
		if(!st.is_next_char_expression_close())
			throw 2;
	}
	inline void source_to_stdout()override{
		statement::source_to_stdout();
		printf("(");
		tk->to_stdout();
		printf(")");
	}
	inline void compiled_to_stdout()override{
		// alloc edx, ecx, ebx, eax
		printf("    mov     edx,%s.len\n",tk->get_name());// length of text
		printf("    mov     ecx,%s\n",tk->get_name());// pointer to text
		puts("    mov     ebx,1");// file descriptor (stdout)
		puts("    mov     eax,4");// system call number (sys_write)
		puts("    int     0x80");// call kernel
		// free edx, ecx, ebx, eax
	}

private:
	unique_ptr<token>tk;
};
