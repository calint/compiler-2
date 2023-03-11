#pragma once

#include"decouple.hpp"
#include"compiler_error.hpp"
#include"toc.hpp"
#include"expression.hpp"

class expr_ops_list final:public expression{
public:
	inline expr_ops_list(const statement&parent,tokenizer&t,const bool in_args=false,const bool enclosed=false,const char list_op='=',const int first_op_precedence=3,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		expression{parent,t.next_whitespace_token()},
		enclosed_{enclosed},
		in_args_{in_args},
		precedence_{first_op_precedence},
		list_op_{list_op}
	{
		// if called in a recursion with a first expression passed
		if(first_expression){
			// put in list
			expressions_.push_back(move(first_expression));
		}else{
			// if subexpression
			if(t.is_next_char('(')){
				// recurse
				expressions_.emplace_back(make_unique<expr_ops_list>(*this,t,in_args,true));
			}else{
				// add first expression
				expressions_.emplace_back(create_statement_from_tokenizer(*this,t));
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
			if(next_precedence>precedence_){
				// i.e. =a+b*c+1 where the peeked char is '*'
				// next operation has higher precedence than current
				// list is now =[(=a)(+b)]
				// move last expression (+b) to subexpression
				//   =[(=a) +[(=b)(*c)(+1)]]
				precedence_=next_precedence;
				if(!ops_.empty()){
					const int first_op_prec=precedence_for_op(ops_.back());
					ops_.pop_back();
					const char lst_op=ops_.back();
					unique_ptr<statement>prev=move(expressions_.back());
					expressions_.pop_back();
					expressions_.emplace_back(make_unique<expr_ops_list>(*this,t,in_args,false,lst_op,first_op_prec,move(prev)));
					continue;
				}
			}else{
				precedence_=next_precedence;
				t.next_char();// read the peeked operator
			}

			if(t.is_next_char('(')){
				expressions_.emplace_back(make_unique<expr_ops_list>(*this,t,in_args,true));
				continue;
			}

			unique_ptr<statement>stmt=create_statement_from_tokenizer(*this,t);
			if(stmt->tok().is_blank())
				throw compiler_error(*stmt,"unexpected '"+string{t.peek_char()}+"'");

			expressions_.push_back(move(stmt));
		}
	}

	inline void source_to(ostream&os)const override{
		expression::source_to(os);//?

		if(enclosed_)
			os<<"(";

		if(not expressions_.empty()){
			expressions_[0]->source_to(os);
			const size_t len{expressions_.size()};
			for(size_t i{1};i<len;i++){
				os<<ops_[i-1];
				expressions_[i]->source_to(os);
			}
		}

		if(enclosed_)
			os<<")";
	}

	inline void compile(toc&tc,ostream&os,const size_t indent_level,const string&dest)const override{
		indent(os,indent_level,true);tc.source_comment(os,*this);

		if(expressions_.empty()) // ? can this happen?
			throw compiler_error(*this,"expressions is empty");

		// first element is assigned to destination
		const statement&st=*expressions_[0].get();
		const string dest_resolved=tc.resolve_ident_to_nasm(*this,dest);
		asm_op(tc,os,indent_level,st,'=',dest,dest_resolved);

		// remaining elements are +,-,*,/
		const size_t len=ops_.size();
		for(size_t i{0};i<len;i++){
			const char op=ops_[i];
			const statement&stm=*expressions_[i+1].get();
			asm_op(tc,os,indent_level,stm,op,dest,dest_resolved);
		}
	}

	inline bool is_expression()const override{
		if(expressions_.size()==1){
			return expressions_[0]->is_expression();
		}
		return true;
	}

	inline bool is_ops_list()const override{return true;}

	inline const string&identifier()const override{
		if(expressions_[0]->is_ops_list())
			return expressions_[0]->identifier();

		return expressions_[0]->identifier();
	}

	inline bool alloc_register()const{return!ops_.empty();}

	inline bool is_empty()const override{return expressions_.empty();}

	inline static void asm_cmd(const string&op,const statement&st,toc&tc,ostream&os,const size_t indent_level,const string&dest_resolved,const string&src_resolved){
		if(op=="mov" && dest_resolved==src_resolved){
			return;
		}
		// check if both source and destination are memory operations
		if(dest_resolved.find_first_of('[')!=string::npos and src_resolved.find_first_of('[')!=string::npos){
			const string&r=tc.alloc_scratch_register(st);
			indent(os,indent_level);os<<"mov "<<r<<","<<src_resolved<<endl;
			indent(os,indent_level);os<<op<<" "<<dest_resolved<<","<<r<<endl;
			tc.free_scratch_register(r);
			return;
		}
		indent(os,indent_level);os<<op<<" "<<dest_resolved<<","<<src_resolved<<endl;
	}

private:
	inline static int precedence_for_op(const char ch){
		if(ch=='+'||ch=='-')
			return 1;
		if(ch=='*'||ch=='/')
			return 2;
		throw string(to_string(__LINE__)+" err");
	}

	inline void asm_op(toc&tc,ostream&os,const size_t indent_level,const statement&st,const char op,const string&dest,const string&dest_resolved)const{
		indent(os,indent_level,true);tc.source_comment(os,dest,op,st);
		if(op=='='){
			if(st.is_expression()){
				st.compile(tc,os,indent_level,dest);
				return;
			}
			asm_cmd("mov",st,tc,os,indent_level,dest_resolved,tc.resolve_ident_to_nasm(st));
			return;
		}
		if(op=='+'){// order1op
			if(st.is_expression()){
				const string r=tc.alloc_scratch_register(st);
				st.compile(tc,os,indent_level,r);
				asm_cmd("add",st,tc,os,indent_level,dest_resolved,r);
				tc.free_scratch_register(r);
				return;
			}
			asm_cmd("add",st,tc,os,indent_level,dest_resolved,tc.resolve_ident_to_nasm(st));
			return;
		}
		if(op=='-'){// order1op
			if(st.is_expression()){
				const string r=tc.alloc_scratch_register(st);
				st.compile(tc,os,indent_level,r);
				asm_cmd("sub",st,tc,os,indent_level,dest_resolved,r);
				tc.free_scratch_register(r);
				return;
			}
			asm_cmd("sub",st,tc,os,indent_level,dest_resolved,tc.resolve_ident_to_nasm(st));
			return;
		}
		if(op=='*'){// order2op
			if(st.is_expression()){
				const string r=tc.alloc_scratch_register(st);
				st.compile(tc,os,indent_level,r);
				// imul destination must be a register
				if(tc.is_identifier_register(dest_resolved)){
					asm_cmd("imul",st,tc,os,indent_level,dest_resolved,r);
				}else{
					// imul destination is not a register
					asm_cmd("imul",st,tc,os,indent_level,r,dest_resolved);
					asm_cmd("mov",st,tc,os,indent_level,dest_resolved,r);
//					asm_cmd("imul",st,tc,os,indent_level,dest_resolved,r);
				}
				tc.free_scratch_register(r);
				return;
			}
			// not an expression, either a register or memory location
			// imul destination operand must be register
			if(tc.is_identifier_register(dest_resolved)){
				asm_cmd("imul",st,tc,os,indent_level,dest_resolved,tc.resolve_ident_to_nasm(st));
				return;
			}
			// imul destination is not a register
			const string&r=tc.alloc_scratch_register(st);
			asm_cmd("mov",st,tc,os,indent_level,r,dest_resolved);
			asm_cmd("imul",st,tc,os,indent_level,r,tc.resolve_ident_to_nasm(st));
			asm_cmd("mov",st,tc,os,indent_level,dest_resolved,r);
			tc.free_scratch_register(r);
			return;
		}
	}

	bool enclosed_{false}; //  =(a+b) vs =a+b
	bool in_args_{false}; // foo(a+b)
	int precedence_{0}; //
	char list_op_{0}; // +[...]
	vector<unique_ptr<statement>>expressions_;
	vector<char>ops_;
};
