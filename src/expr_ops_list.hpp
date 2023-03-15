#pragma once

#include"decouple.hpp"
#include"compiler_error.hpp"
#include"toc.hpp"
#include"expression.hpp"

class expr_ops_list final:public expression{
public:
	inline expr_ops_list(tokenizer&t,const bool in_args=false,const bool enclosed=false,const char list_op='=',const int first_op_precedence=3,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		expression{t.next_whitespace_token()},
		enclosed_{enclosed},
		in_args_{in_args},
		list_op_{list_op}
	{
		int precedence=first_op_precedence;
		// if called in a recursion with a first expression passed
		if(first_expression){
			// put in list
			exps_.push_back(move(first_expression));
		}else{
			// if subexpression
			if(t.is_next_char('(')){
				// recurse
				exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,true));
			}else{
				// add first expression
				exps_.emplace_back(create_statement_from_tokenizer(t));
			}
		}

		while(true){ // +a  +3
			// if parsed in a function call argument list
			if(in_args){ // ? rewrite is_in_bool_expr
				// if in boolean expression exit when an operation is found
				if(t.is_peek_char('<'))break;
				if(t.is_peek_char('='))break;
				if(t.is_peek_char('>'))break;
			}
			// if in a function argument return when ',' or ')' found
			if(in_args and (t.is_peek_char(',') or t.is_peek_char(')')))
				break;

			// if end of subexpression
			if(t.is_peek_char(')')){
				if(enclosed_){
					t.next_char();
				}
				break;
			}

			// next operation
			if(t.is_peek_char('+')){
				ops_.push_back('+');
			}else if(t.is_peek_char('-')){
				ops_.push_back('-');
			}else if(t.is_peek_char('*')){
				ops_.push_back('*');
			}else if(t.is_peek_char('/')){
				ops_.push_back('/');
			}else if(t.is_peek_char('%')){
				ops_.push_back('%');
			}else{
				// no more operations
				break;
			}

			// check if next operation precedence is same or lower
			// if not then a new subexpression is added to the list with the last
			// expression in this list as first expression
			const int next_precedence=precedence_for_op(t.peek_char());
			if(next_precedence>precedence){
				// i.e. =a+b*c+1 where the peeked char is '*'
				// next operation has higher precedence than current
				// list is now =[(=a)(+b)]
				// move last expression (+b) to subexpression
				//   =[(=a) +[(=b)(*c)(+1)]]
				precedence=next_precedence;
				if(!ops_.empty()){
					const int first_op_prec=precedence_for_op(ops_.back());
					ops_.pop_back();
					const char lst_op=ops_.back();
					unique_ptr<statement>prev=move(exps_.back());
					exps_.pop_back();
					exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,false,lst_op,first_op_prec,move(prev)));
					continue;
				}
			}else{
				precedence=next_precedence;
				t.next_char();// read the peeked operator
			}

			if(t.is_next_char('(')){
				exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,true));
				continue;
			}

			unique_ptr<statement>stmt=create_statement_from_tokenizer(t);
			if(stmt->tok().is_blank())
				throw compiler_error(*stmt,"unexpected '"+string{t.peek_char()}+"'");

			exps_.push_back(move(stmt));
		}
	}

	inline void source_to(ostream&os)const override{
		expression::source_to(os);//?

		if(enclosed_)
			os<<"(";

		if(not exps_.empty()){
			exps_[0]->source_to(os);
			const size_t len{exps_.size()};
			for(size_t i{1};i<len;i++){
				os<<ops_[i-1];
				exps_[i]->source_to(os);
			}
		}

		if(enclosed_)
			os<<")";
	}

	inline void compile(toc&tc,ostream&os,const size_t indent_level,const string&dest)const override{
		tc.source_comment(*this,os,indent_level);

		if(exps_.empty()) // ? can this happen?
			throw compiler_error(*this,"expressions is empty");

		// first element is assigned to destination
		const statement&st=*exps_[0].get();
		const string dest_resolved=tc.resolve_ident_to_nasm(*this,dest);
		asm_op(tc,os,indent_level,st,'=',dest,dest_resolved);

		// remaining elements are +,-,*,/
		const size_t len=ops_.size();
		for(size_t i{0};i<len;i++){
			const char op=ops_[i];
			const statement&stm=*exps_[i+1].get();
			asm_op(tc,os,indent_level,stm,op,dest,dest_resolved);
		}
	}

	inline bool is_expression()const override{
		if(exps_.size()==1){
			return exps_[0]->is_expression();
		}
		return true;
	}

	inline const string&identifier()const override{
		return exps_[0]->identifier();
	}

	inline bool is_empty()const override{return exps_.empty();}

private:
	inline static int precedence_for_op(const char ch){
		if(ch=='+'||ch=='-')
			return 1;
		if(ch=='*'||ch=='/')
			return 2;
		throw string(to_string(__LINE__)+" err");
	}

	inline void asm_op(toc&tc,ostream&os,const size_t indent_level,const statement&st,const char op,const string&dest,const string&dest_resolved)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,dest,op,st);
		if(op=='='){
			if(st.is_expression()){
				st.compile(tc,os,indent_level,dest);
				return;
			}
			tc.asm_cmd(st,os,indent_level,"mov",dest_resolved,tc.resolve_ident_to_nasm(st));
			return;
		}
		if(op=='+'){// order1op
			if(st.is_expression()){
				const string r=tc.alloc_scratch_register(st,os,indent_level);
				st.compile(tc,os,indent_level,r);
				tc.asm_cmd(st,os,indent_level,"add",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			tc.asm_cmd(st,os,indent_level,"add",dest_resolved,tc.resolve_ident_to_nasm(st));
			return;
		}
		if(op=='-'){// order1op
			if(st.is_expression()){
				const string r=tc.alloc_scratch_register(st,os,indent_level);
				st.compile(tc,os,indent_level,r);
				tc.asm_cmd(st,os,indent_level,"sub",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			tc.asm_cmd(st,os,indent_level,"sub",dest_resolved,tc.resolve_ident_to_nasm(st));
			return;
		}
		if(op=='*'){// order2op
			if(st.is_expression()){
				const string&r=tc.alloc_scratch_register(st,os,indent_level);
				st.compile(tc,os,indent_level,r);
				// imul destination must be a register
				if(tc.is_identifier_register(dest_resolved)){
					tc.asm_cmd(st,os,indent_level,"imul",dest_resolved,r);
				}else{
					// imul destination is not a register
					tc.asm_cmd(st,os,indent_level,"imul",r,dest_resolved);
					tc.asm_cmd(st,os,indent_level,"mov",dest_resolved,r);
				}
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			// not an expression, either a register or memory location
			// imul destination operand must be register
			if(tc.is_identifier_register(dest_resolved)){
				tc.asm_cmd(st,os,indent_level,"imul",dest_resolved,tc.resolve_ident_to_nasm(st));
				return;
			}
			// imul destination is not a register
			const string&r=tc.alloc_scratch_register(st,os,indent_level);
			tc.asm_cmd(st,os,indent_level,"mov",r,dest_resolved);
			tc.asm_cmd(st,os,indent_level,"imul",r,tc.resolve_ident_to_nasm(st));
			tc.asm_cmd(st,os,indent_level,"mov",dest_resolved,r);
			tc.free_scratch_register(os,indent_level,r);
			return;
		}
	}

	bool enclosed_{false}; //  =(a+b) vs =a+b
	bool in_args_{false}; // foo(a+b)
	char list_op_{0}; // +[...]
	vector<unique_ptr<statement>>exps_;
	vector<char>ops_;
};
