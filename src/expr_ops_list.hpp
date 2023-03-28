#pragma once

#include"compiler_error.hpp"
#include"toc.hpp"
#include"expression.hpp"
#include"decouple.hpp"

class expr_ops_list final:public expression{
public:
	inline expr_ops_list(toc&tc,tokenizer&t,const bool in_args=false,const bool enclosed=false,unary_ops uops={},const char first_op_precedence=initial_precedence,unique_ptr<statement>first_expression=unique_ptr<statement>()):
		expression{t.next_whitespace_token(),{}},
		enclosed_{enclosed},
		uops_{move(uops)}
	{
		// read first expression i.e. =-a/-(b+1)
		if(first_expression){
			// called in a recursion with first expression provided
			exps_.push_back(move(first_expression));
		}else{
			// check if new recursion is necessary i.e. =-a/-(-(b+c)+d), t at "-a/-("
			unary_ops uo={t};
			if(t.is_next_char('(')){
				// recursion
				exps_.emplace_back(make_unique<expr_ops_list>(tc,t,in_args,true,move(uo)));
			}else{
				// statement
				uo.put_back(t);
				exps_.emplace_back(create_statement_from_tokenizer(tc,t));
			}
		}

		char precedence=first_op_precedence;
		while(true){ // +a  +3
			// if end of subexpression
			if(enclosed_&&t.is_next_char(')'))
				break;

			// if parsed in a function call argument list or in a boolean expression
			if(in_args){ // ? rewrite is_in_bool_expr
				// if in boolean expression exit when an operation is found
				if(t.is_peek_char('<')&&not t.is_peek_char2('<'))break;
				if(t.is_peek_char('='))break;
				if(t.is_peek_char('>')&&not t.is_peek_char2('>'))break;
				// if in arguments exit when ',' or ')' is found
				if(t.is_peek_char(','))break;
				if(t.is_peek_char(')'))break;
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
			}else if(t.is_peek_char('<')&&t.is_peek_char2('<')){
				ops_.push_back('<');
			}else if(t.is_peek_char('>')&&t.is_peek_char2('>')){
			 	ops_.push_back('>');
			}else{
				// no more operations
				break;
			}

			// check if next operation precedence is same or lower
			// if not then a new subexpression is added to the list with the last
			// expression in this list as first expression
			const char next_precedence{precedence_for_op(ops_.back())};
			if(next_precedence>precedence){
				// i.e. =a+b*c+1 where the peeked char is '*'
				// next operation has higher precedence than current
				// list is now =[(=a)(+b)]
				// move last expression (+b) to subexpression
				//   =[(=a) +[(=b)(*c)(+1)]]
				precedence=next_precedence;
				const char first_op_prec{precedence_for_op(ops_.back())};
				ops_.pop_back();
				unique_ptr<statement>prev{move(exps_.back())};
				exps_.pop_back();
				exps_.emplace_back(make_unique<expr_ops_list>(tc,t,in_args,false,unary_ops{},first_op_prec,move(prev)));
				continue;
			}else{
				precedence=next_precedence;
				const char ch=t.next_char();// read the peeked operator
				if(ch=='<'||ch=='>')
					t.next_char(); // one more character for << and >>
			}

			// check if next statement is a subexpression
			unary_ops uo{t};
			if(t.is_next_char('(')){
				// subexpression, recurse
				exps_.emplace_back(make_unique<expr_ops_list>(tc,t,in_args,true,move(uo)));
				continue;
			}
			// not subexpression
			uo.put_back(t);

			// add statement to list
			unique_ptr<statement>stm{create_statement_from_tokenizer(tc,t)};
			if(stm->tok().is_empty())
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
		expression::source_to(os); // whitespace
		uops_.source_to(os);

		if(enclosed_)
			os<<"(";

		if(not exps_.empty()){
			exps_[0]->source_to(os);
			const size_t len{exps_.size()};
			for(size_t i{1};i<len;i++){
				const char op=ops_[i-1];
				os<<op;
				if(op=='<'||op=='>')
					os<<op;
				exps_[i]->source_to(os);
			}
		}

		if(enclosed_)
			os<<")";
	}

	inline void compile(toc&tc,ostream&os,const size_t indent,const string&dst)const override{
		tc.source_comment(*this,os,indent);

		if(exps_.empty()) // ? can this happen?
			throw compiler_error(*this,"expression is empty");

		// first element is assigned to destination
		const statement&st{*exps_[0].get()};
		const ident_resolved&ir{tc.resolve_ident_to_nasm(st,dst,false)};
		asm_op(tc,os,indent,'=',dst,ir.id,st);

		// remaining elements are +,-,*,/
		const size_t n{ops_.size()};
		for(size_t i{0};i<n;i++){
			const char op=ops_[i];
			const statement&stm{*exps_[i+1].get()};
			asm_op(tc,os,indent,op,dst,ir.id,stm);
		}
		uops_.compile(tc,os,indent,ir.id);
	}

	inline bool is_expression()const override{
		if(not uops_.is_empty())
			return true;

		if(exps_.size()==1)
			return exps_[0]->is_expression();

		return true;
	}

	inline const string&identifier()const override{
		if(exps_.size()==1)
			return exps_[0]->identifier();

		throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
	}

	inline const unary_ops&get_unary_ops()const override{
		if(exps_.size()==1)
			return exps_[0]->get_unary_ops();
		
		return uops_;
	}

	inline const type&get_type()const override{
		assert(not exps_.empty());
		return exps_[0]->get_type(); // ! hack  find the size of the largest integral element
	}


private:
	static constexpr char initial_precedence{7}; // higher than the highest precedence
	inline static char precedence_for_op(const char ch){
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
			case'%':return 6;
			default:throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
		}
	}

	inline void asm_op(toc&tc,ostream&os,const size_t indent,const char op,const string&dst,const string&dst_resolved,const statement&src)const{
		toc::indent(os,indent,true);tc.source_comment(os,dst,{op},src);
		if(op=='='){
			if(src.is_expression()){
				src.compile(tc,os,indent,dst);
				return;
			}
			const ident_resolved&ir{tc.resolve_ident_to_nasm(src,true)};
			if(ir.is_const()){
				tc.asm_cmd(src,os,indent,"mov",dst_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
				return;
			}
			tc.asm_cmd(src,os,indent,"mov",dst_resolved,ir.id);
			src.get_unary_ops().compile(tc,os,indent,dst_resolved);
			return;
		}
		if(op=='+'){
			asm_op_add_sub(tc,os,indent,"add","sub",dst,dst_resolved,src);
			return;
		}
		if(op=='-'){// order1op
			asm_op_add_sub(tc,os,indent,"sub","add",dst,dst_resolved,src);
			return;
		}
		if(op=='*'){// order2op
			if(src.is_expression()){
				const string&r{tc.alloc_scratch_register(src,os,indent)};
				src.compile(tc,os,indent,r);
				// imul destination must be a register
				if(tc.is_identifier_register(dst_resolved)){
					tc.asm_cmd(src,os,indent,"imul",dst_resolved,r);
				}else{
					// imul destination is not a register
					tc.asm_cmd(src,os,indent,"imul",r,dst_resolved);
					tc.asm_cmd(src,os,indent,"mov",dst_resolved,r);
				}
				tc.free_scratch_register(os,indent,r);
				return;
			}
			// not an expression, either a register or memory location
			const ident_resolved&src_r=tc.resolve_ident_to_nasm(src,true);
			// imul destination operand must be register
			if(tc.is_identifier_register(dst_resolved)){
				if(src_r.is_const()){
					tc.asm_cmd(src,os,indent,"imul",dst_resolved,src.get_unary_ops().get_ops_as_string()+src_r.id);
					return;
				}
				const unary_ops&uops=src.get_unary_ops();
				if(uops.is_empty()){
					tc.asm_cmd(src,os,indent,"imul",dst_resolved,src_r.id);
					return;
				}
				const string&r{tc.alloc_scratch_register(src,os,indent)};
				tc.asm_cmd(src,os,indent,"mov",r,src_r.id);
				uops.compile(tc,os,indent,r);
				tc.asm_cmd(src,os,indent,"imul",dst_resolved,r);
				tc.free_scratch_register(os,indent,r);
				return;
			}
			// imul destination is not a register
			if(src_r.is_const()){
				const string&r{tc.alloc_scratch_register(src,os,indent)};
				tc.asm_cmd(src,os,indent,"mov",r,dst_resolved);
				tc.asm_cmd(src,os,indent,"imul",r,src.get_unary_ops().get_ops_as_string()+src_r.id);
				tc.asm_cmd(src,os,indent,"mov",dst_resolved,r);
				tc.free_scratch_register(os,indent,r);
				return;
			}
			const unary_ops&uops=src.get_unary_ops();
			if(uops.is_empty()){
				const string&r{tc.alloc_scratch_register(src,os,indent)};
				tc.asm_cmd(src,os,indent,"mov",r,dst_resolved);
				tc.asm_cmd(src,os,indent,"imul",r,src_r.id);
				tc.asm_cmd(src,os,indent,"mov",dst_resolved,r);
				tc.free_scratch_register(os,indent,r);
				return;
			}
			const string&r{tc.alloc_scratch_register(src,os,indent)};
			tc.asm_cmd(src,os,indent,"mov",r,src_r.id);
			uops.compile(tc,os,indent,r);
			tc.asm_cmd(src,os,indent,"imul",r,dst_resolved);
			tc.asm_cmd(src,os,indent,"mov",dst_resolved,r);
			tc.free_scratch_register(os,indent,r);
			return;
		}
		if(op=='/'){
			asm_op_div(tc,os,indent,"rax",dst,dst_resolved,src);
			return;
		}
		if(op=='%'){
			asm_op_div(tc,os,indent,"rdx",dst,dst_resolved,src);
			return;
		}
		if(op=='&'){
			asm_op_bitwise(tc,os,indent,"and",dst,dst_resolved,src);
			return;
		}
		if(op=='|'){
			asm_op_bitwise(tc,os,indent,"or",dst,dst_resolved,src);
			return;
		}
		if(op=='^'){
			asm_op_bitwise(tc,os,indent,"xor",dst,dst_resolved,src);
			return;
		}
		if(op=='<'){
			asm_op_shift(tc,os,indent,"sal",dst,dst_resolved,src);
			return;
		}
		if(op=='>'){
			asm_op_shift(tc,os,indent,"sar",dst,dst_resolved,src);
			return;
		}
	}

	inline void asm_op_add_sub(toc&tc,ostream&os,const size_t indent,const string&op,const string&op_inv,const string&dst,const string&dst_resolved,const statement&src)const{
		if(src.is_expression()){
			const string&r{tc.alloc_scratch_register(src,os,indent)};
			src.compile(tc,os,indent,r);
			tc.asm_cmd(src,os,indent,op,dst_resolved,r);
			tc.free_scratch_register(os,indent,r);
			return;
		}
		const ident_resolved&ir{tc.resolve_ident_to_nasm(src,true)};
		if(ir.is_const()){
			tc.asm_cmd(src,os,indent,op,dst_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
			return;
		}
		const unary_ops&uops=src.get_unary_ops();
		if(uops.is_empty()){
			tc.asm_cmd(src,os,indent,op,dst_resolved,ir.id);
			return;
		}
		if(uops.is_only_negated()){
			tc.asm_cmd(src,os,indent,op_inv,dst_resolved,ir.id);
			return;
		}
		const string&r{tc.alloc_scratch_register(src,os,indent)};
		tc.asm_cmd(src,os,indent,"mov",r,ir.id);
		uops.compile(tc,os,indent,r);		
		tc.asm_cmd(src,os,indent,op,dst_resolved,r);
		tc.free_scratch_register(os,indent,r);
	}

	inline void asm_op_bitwise(toc&tc,ostream&os,const size_t indent,const string&op,const string&dst,const string&dst_resolved,const statement&src)const{
		if(src.is_expression()){
			const string&r{tc.alloc_scratch_register(src,os,indent)};
			src.compile(tc,os,indent,r);
			tc.asm_cmd(src,os,indent,op,dst_resolved,r);
			tc.free_scratch_register(os,indent,r);
			return;
		}
		const ident_resolved&ir{tc.resolve_ident_to_nasm(src,true)};
		if(ir.is_const()){
			tc.asm_cmd(src,os,indent,op,dst_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
			return;
		}
		const unary_ops&uops=src.get_unary_ops();
		if(uops.is_empty()){
			tc.asm_cmd(src,os,indent,op,dst_resolved,ir.id);
			return;
		}
		const string&r{tc.alloc_scratch_register(src,os,indent)};
		tc.asm_cmd(src,os,indent,"mov",r,ir.id);
		uops.compile(tc,os,indent,r);		
		tc.asm_cmd(src,os,indent,op,dst_resolved,r);
		tc.free_scratch_register(os,indent,r);
	}

	inline void asm_op_shift(toc&tc,ostream&os,const size_t indent,const string&op,const string&dst,const string&dst_resolved,const statement&src)const{
		if(src.is_expression()){
			// the operand must be stored in CL
			const bool rcx_allocated=tc.alloc_named_register(src,os,indent,"rcx");
			if(not rcx_allocated){
				tc.asm_push(src,os,indent,"rcx");
			}
			src.compile(tc,os,indent,"rcx");
			tc.asm_cmd(src,os,indent,op,dst_resolved,"cl");
			if(rcx_allocated){
				tc.free_named_register(os,indent,"rcx");
			}else{
				tc.asm_pop(src,os,indent,"rcx");
			}
			return;
		}
		const ident_resolved&ir{tc.resolve_ident_to_nasm(src,true)};
		if(ir.is_const()){
			tc.asm_cmd(src,os,indent,op,dst_resolved,src.get_unary_ops().get_ops_as_string()+ir.id);
			return;
		}
		if(ir.id=="rcx")
			throw compiler_error(src,"cannot use 'rcx' as operand in shift because that registers is used");
		const unary_ops&uops=src.get_unary_ops();
		if(uops.is_empty()){
			// the operand must be stored in CL
			const bool rcx_allocated=tc.alloc_named_register(src,os,indent,"rcx");
			if(not rcx_allocated){
				tc.asm_push(src,os,indent,"rcx");
			}
			tc.asm_cmd(src,os,indent,"mov","rcx",ir.id);
			tc.asm_cmd(src,os,indent,op,dst_resolved,"cl");
			if(rcx_allocated){
				tc.free_named_register(os,indent,"rcx");
			}else{
				tc.asm_pop(src,os,indent,"rcx");
			}
			return;
		}
		const bool rcx_allocated=tc.alloc_named_register(src,os,indent,"rcx");
		if(not rcx_allocated){
			tc.asm_push(src,os,indent,"rcx");
		}
		tc.asm_cmd(src,os,indent,"mov","rcx",ir.id);
		uops.compile(tc,os,indent,"rcx");		
		tc.asm_cmd(src,os,indent,op,dst_resolved,"cl");
		if(rcx_allocated){
			tc.free_named_register(os,indent,"rcx");
		}else{
			tc.asm_pop(src,os,indent,"rcx");
		}
	}

	// op is either "rax" for the quotient or "rdx" for the reminder
	inline void asm_op_div(toc&tc,ostream&os,const size_t indent,const string&op,const string&dst,const string&dst_resolved,const statement&src)const{
		if(src.is_expression()){
			const string&r{tc.alloc_scratch_register(src,os,indent)};
			src.compile(tc,os,indent,r);
			const bool rax_allocated=tc.alloc_named_register(src,os,indent,"rax");
			if(not rax_allocated){
				tc.asm_push(src,os,indent,"rax");
			}
			tc.asm_cmd(src,os,indent,"mov","rax",dst_resolved);
			const bool rdx_allocated=tc.alloc_named_register(src,os,indent,"rdx");
			if(not rdx_allocated){
				tc.asm_push(src,os,indent,"rdx");
			}
			tc.indent(os,indent,false);os<<"cqo"<<endl;
			tc.indent(os,indent,false);os<<"idiv "<<r<<endl;
			tc.asm_cmd(src,os,indent,"mov",dst_resolved,op);
			if(rdx_allocated){
				tc.free_named_register(os,indent,"rdx");
			}else{
				tc.asm_pop(src,os,indent,"rdx");
			}
			if(rax_allocated){
				tc.free_named_register(os,indent,"rax");
			}else{
				tc.asm_pop(src,os,indent,"rax");
			}
			tc.free_scratch_register(os,indent,r);
			return;
		}
		const ident_resolved&ir{tc.resolve_ident_to_nasm(src,true)};
		if(ir.is_const()){
			const bool rax_allocated=tc.alloc_named_register(src,os,indent,"rax");
			if(not rax_allocated){
				tc.asm_push(src,os,indent,"rax");
			}
			tc.asm_cmd(src,os,indent,"mov","rax",dst_resolved);
			const bool rdx_allocated=tc.alloc_named_register(src,os,indent,"rdx");
			if(not rdx_allocated){
				tc.asm_push(src,os,indent,"rdx");
			}
			tc.indent(os,indent,false);os<<"cqo"<<endl;
			const string&r{tc.alloc_scratch_register(src,os,indent)};
			tc.asm_cmd(src,os,indent,"mov",r,src.get_unary_ops().get_ops_as_string()+ir.id);
			tc.indent(os,indent,false);os<<"idiv "<<r<<endl;
			tc.free_scratch_register(os,indent,r);
			tc.asm_cmd(src,os,indent,"mov",dst_resolved,op);
			if(rdx_allocated){
				tc.free_named_register(os,indent,"rdx");
			}else{
				tc.asm_pop(src,os,indent,"rdx");
			}
			if(rax_allocated){
				tc.free_named_register(os,indent,"rax");
			}else{
				tc.asm_pop(src,os,indent,"rax");
			}
			return;
		}
		if(ir.id=="rdx" or ir.id=="rax")
			throw compiler_error(src,"cannot use 'rdx' or 'rax' as operands in division because those registers are used");
		const unary_ops&uops=src.get_unary_ops();
		if(uops.is_empty()){
			const bool rax_allocated=tc.alloc_named_register(src,os,indent,"rax");
			if(not rax_allocated){
				tc.asm_push(src,os,indent,"rax");
			}
			tc.asm_cmd(src,os,indent,"mov","rax",dst_resolved);
			const bool rdx_allocated=tc.alloc_named_register(src,os,indent,"rdx");
			if(not rdx_allocated){
				tc.asm_push(src,os,indent,"rdx");
			}
			tc.indent(os,indent,false);os<<"cqo"<<endl;
			tc.indent(os,indent,false);os<<"idiv "<<ir.id<<endl;
			// op is either "rax" for the quotient or "rdx" for the reminder
			tc.asm_cmd(src,os,indent,"mov",dst_resolved,op);
			if(rdx_allocated){
				tc.free_named_register(os,indent,"rdx");
			}else{
				tc.asm_pop(src,os,indent,"rdx");
			}
			if(rax_allocated){
				tc.free_named_register(os,indent,"rax");
			}else{
				tc.asm_pop(src,os,indent,"rax");
			}
			return;
		}
		const string&r{tc.alloc_scratch_register(src,os,indent)};
		tc.asm_cmd(src,os,indent,"mov",r,ir.id);
		uops.compile(tc,os,indent,r);
		const bool rax_allocated=tc.alloc_named_register(src,os,indent,"rax");
		if(not rax_allocated){
			tc.asm_push(src,os,indent,"rax");
		}
		tc.asm_cmd(src,os,indent,"mov","rax",dst_resolved);
		const bool rdx_allocated=tc.alloc_named_register(src,os,indent,"rdx");
		if(not rdx_allocated){
			tc.asm_push(src,os,indent,"rdx");
		}
		tc.indent(os,indent,false);os<<"cqo"<<endl;
		tc.indent(os,indent,false);os<<"idiv "<<r<<endl;
		tc.asm_cmd(src,os,indent,"mov",dst_resolved,op);
		if(rdx_allocated){
			tc.free_named_register(os,indent,"rdx");
		}else{
			tc.asm_pop(src,os,indent,"rdx");
		}
		if(rax_allocated){
			tc.free_named_register(os,indent,"rax");
		}else{
			tc.asm_pop(src,os,indent,"rax");
		}
		tc.free_scratch_register(os,indent,r);
	}

	bool enclosed_{}; //  (a+b) vs a+b
	vector<unique_ptr<statement>>exps_;
	vector<char>ops_;
	unary_ops uops_;
};
