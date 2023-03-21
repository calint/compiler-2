#pragma once

#include"compiler_error.hpp"
#include"toc.hpp"
#include"expression.hpp"
#include"decouple.hpp"

class expr_ops_list final:public expression{
public:
	inline expr_ops_list(tokenizer&t,const bool in_args=false,const bool enclosed=false,const char list_op='=',const int first_op_precedence=3,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		expression{t.next_whitespace_token(),{}},
		enclosed_{enclosed},
		in_args_{in_args},
		list_op_{list_op}
	{
		// read first expression
		// if called in a recursion with a first expression passed
		if(first_expression){
			// put in list
			exps_.push_back(move(first_expression));
		}else{
			uops_={t};
			// check for negated expression list. i.e. -(a+b)
			if(not uops_.is_empty()){
				if(t.is_next_char('(')){
					exps_.emplace_back(make_unique<expr_ops_list>(t,in_args,true));
				}else{
					uops_.put_back(t);
					uops_={};
					exps_.emplace_back(create_statement_from_tokenizer(t));
				}
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
		}

		int precedence=first_op_precedence;
		while(true){ // +a  +3
			// if parsed in a function call argument list or in a boolean expression
			if(in_args){ // ? rewrite is_in_bool_expr
				// if in boolean expression exit when an operation is found
				if(t.is_peek_char('<'))break;
				if(t.is_peek_char('='))break;
				if(t.is_peek_char('>'))break;
				// if in arguments exit when ',' or ')' is found
				if(t.is_peek_char(','))break;
				if(t.is_peek_char(')'))break;
			}

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
			}else if(t.is_peek_char('&')){
				ops_.push_back('&');
			}else if(t.is_peek_char('|')){
				ops_.push_back('|');
			}else if(t.is_peek_char('^')){
				ops_.push_back('^');
			}else{
				// no more operations
				break;
			}

			// check if next operation precedence is same or lower
			// if not then a new subexpression is added to the list with the last
			// expression in this list as first expression
			const int next_precedence{precedence_for_op(t.peek_char())};
			if(next_precedence>precedence){
				// i.e. =a+b*c+1 where the peeked char is '*'
				// next operation has higher precedence than current
				// list is now =[(=a)(+b)]
				// move last expression (+b) to subexpression
				//   =[(=a) +[(=b)(*c)(+1)]]
				precedence=next_precedence;
				if(not ops_.empty()){
					const int first_op_prec{precedence_for_op(ops_.back())};
					ops_.pop_back();
					const char lst_op{ops_.back()};
					unique_ptr<statement>prev{move(exps_.back())};
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

			unique_ptr<statement>stm{create_statement_from_tokenizer(t)};
			if(stm->tok().is_blank())
				throw compiler_error(*stm,"unexpected '"+string{t.peek_char()}+"'");

			exps_.push_back(move(stm));
		}
	}

	inline expr_ops_list()=default;
	inline expr_ops_list(const expr_ops_list&)=default;
	inline expr_ops_list(expr_ops_list&&)=default;
	inline expr_ops_list&operator=(const expr_ops_list&)=default;
	inline expr_ops_list&operator=(expr_ops_list&&)=default;

	inline void source_to(ostream&os)const override{
		// expression::source_to(os);//?
		uops_.source_to(os);

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
		const statement&st{*exps_[0].get()};
		const ident_resolved&ir{tc.resolve_ident_to_nasm(st,dest)};
		asm_op(tc,os,indent_level,'=',dest,ir.id,st);

		// remaining elements are +,-,*,/
		const size_t n{ops_.size()};
		for(size_t i{0};i<n;i++){
			const char op=ops_[i];
			const statement&stm{*exps_[i+1].get()};
			asm_op(tc,os,indent_level,op,dest,ir.id,stm);
		}
		uops_.compile(tc,os,indent_level,ir.id);
	}

	inline bool is_expression()const override{
		if(exps_.size()==1){
			return exps_[0]->is_expression();
		}
		return true;
	}

	inline const string&identifier()const override{
		if(exps_.size()==1)
			return exps_[0]->identifier();
		throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
	}

	inline const unary_ops&get_unary_ops()const override{
		if(exps_.size()==1){
			return exps_[0]->get_unary_ops();
		}
		return uops_;
	}

	// inline string as_const()const override{
	// 	if(exps_.size()==1)
	// 		return exps_[0]->as_const();
	// 	throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
	// }

private:
	inline static int precedence_for_op(const char ch){
		switch(ch){
			case'|':return 1;
			case'^':return 2;
			case'&':return 3;
			case'<':return 4; // shift left
			case'>':return 4; // shift right
			case'+':return 5;
			case'-':return 5;
			case'*':return 6;
			case'/':return 6;
			default:throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
		}
	}

	inline void asm_op(toc&tc,ostream&os,const size_t indent_level,const char op,const string&dest,const string&dest_resolved,const statement&src)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,dest,op,src);
		if(op=='='){
			if(src.is_expression()){
				src.compile(tc,os,indent_level,dest);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
				return;
			}
			tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,ir.id);
			src.get_unary_ops().compile(tc,os,indent_level,dest_resolved);
			return;
		}
		if(op=='+'){// order1op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				src.compile(tc,os,indent_level,r);
				tc.asm_cmd(src,os,indent_level,"add",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"add",dest_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
				return;
			}
			const unary_ops&uops=src.get_unary_ops();
			if(uops.is_empty()){
				tc.asm_cmd(src,os,indent_level,"add",dest_resolved,ir.id);
				return;
			}
			if(uops.is_only_negated()){
				tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,ir.id);
				return;
			}
			const string&r{tc.alloc_scratch_register(src,os,indent_level)};
			tc.asm_cmd(src,os,indent_level,"mov",r,ir.id);
			uops.compile(tc,os,indent_level,r);		
			tc.asm_cmd(src,os,indent_level,"add",dest_resolved,r);
			tc.free_scratch_register(os,indent_level,r);
			return;
		}
		if(op=='-'){// order1op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				src.compile(tc,os,indent_level,r);
				tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
				return;
			}
			const unary_ops&uops=src.get_unary_ops();
			if(uops.is_empty()){
				tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,ir.id);
				return;
			}
			if(uops.is_only_negated()){
				tc.asm_cmd(src,os,indent_level,"add",dest_resolved,ir.id);
				return;
			}
			const string&r{tc.alloc_scratch_register(src,os,indent_level)};
			tc.asm_cmd(src,os,indent_level,"mov",r,ir.id);
			uops.compile(tc,os,indent_level,r);		
			tc.asm_cmd(src,os,indent_level,"sub",dest_resolved,r);
			tc.free_scratch_register(os,indent_level,r);
			return;
		}
		if(op=='*'){// order2op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				src.compile(tc,os,indent_level,r);
				// imul destination must be a register
				if(tc.is_identifier_register(dest_resolved)){
					tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,r);
				}else{
					// imul destination is not a register
					tc.asm_cmd(src,os,indent_level,"imul",r,dest_resolved);
					tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,r);
				}
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			// not an expression, either a register or memory location
			const ident_resolved&src_r=tc.resolve_ident_to_nasm(src);
			// imul destination operand must be register
			if(tc.is_identifier_register(dest_resolved)){
				if(src_r.is_const()){
					tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,src.get_unary_ops().get_ops_as_string()+src_r.id);
					return;
				}
				const unary_ops&uops=src.get_unary_ops();
				if(uops.is_empty()){
					tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,src_r.id);
					return;
				}
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				tc.asm_cmd(src,os,indent_level,"mov",r,src_r.id);
				uops.compile(tc,os,indent_level,r);
				tc.asm_cmd(src,os,indent_level,"imul",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			// imul destination is not a register
			if(src_r.is_const()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				tc.asm_cmd(src,os,indent_level,"mov",r,dest_resolved);
				tc.asm_cmd(src,os,indent_level,"imul",r,src.get_unary_ops().get_ops_as_string()+src_r.id);
				tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			const unary_ops&uops=src.get_unary_ops();
			if(uops.is_empty()){
				const string&r{tc.alloc_scratch_register(src,os,indent_level)};
				tc.asm_cmd(src,os,indent_level,"mov",r,dest_resolved);
				tc.asm_cmd(src,os,indent_level,"imul",r,src_r.id);
				tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,r);
				tc.free_scratch_register(os,indent_level,r);
				return;
			}
			const string&r{tc.alloc_scratch_register(src,os,indent_level)};
			tc.asm_cmd(src,os,indent_level,"mov",r,src_r.id);
			uops.compile(tc,os,indent_level,r);
			tc.asm_cmd(src,os,indent_level,"imul",r,dest_resolved);
			tc.asm_cmd(src,os,indent_level,"mov",dest_resolved,r);
			tc.free_scratch_register(os,indent_level,r);
			return;
		}
		if(op=='&'){
			asm_op_bitwise(tc,os,indent_level,"and",dest,dest_resolved,src);
			return;
		}
		if(op=='|'){
			asm_op_bitwise(tc,os,indent_level,"or",dest,dest_resolved,src);
			return;
		}
		if(op=='^'){
			asm_op_bitwise(tc,os,indent_level,"xor",dest,dest_resolved,src);
			return;
		}
	}

	inline void asm_op_bitwise(toc&tc,ostream&os,const size_t indent_level,const string&op,const string&dest,const string&dest_resolved,const statement&src)const{
		if(src.is_expression()){
			const string&r{tc.alloc_scratch_register(src,os,indent_level)};
			src.compile(tc,os,indent_level,r);
			tc.asm_cmd(src,os,indent_level,op,dest_resolved,r);
			tc.free_scratch_register(os,indent_level,r);
			return;
		}
		const ident_resolved&ir{tc.resolve_ident_to_nasm(src)};
		if(ir.is_const()){
			tc.asm_cmd(src,os,indent_level,op,dest_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
			return;
		}
		const unary_ops&uops=src.get_unary_ops();
		if(uops.is_empty()){
			tc.asm_cmd(src,os,indent_level,op,dest_resolved,ir.id);
			return;
		}
		const string&r{tc.alloc_scratch_register(src,os,indent_level)};
		tc.asm_cmd(src,os,indent_level,"mov",r,ir.id);
		uops.compile(tc,os,indent_level,r);		
		tc.asm_cmd(src,os,indent_level,op,dest_resolved,r);
		tc.free_scratch_register(os,indent_level,r);
	}

	bool enclosed_{}; //  (a+b) vs a+b
	bool in_args_{}; // foo(a+b)
	char list_op_{}; // +[...] -[...] *[...] /[...]
	bool negated_{};
	vector<unique_ptr<statement>>exps_;
	vector<char>ops_;
	unary_ops uops_;
};
