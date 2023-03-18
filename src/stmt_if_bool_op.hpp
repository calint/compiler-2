#pragma once

#include"statement.hpp"

class stmt_if_bool_op final:public statement{
public:
	inline stmt_if_bool_op(tokenizer&t):
		statement{t.next_whitespace_token()}
	{
		// if not a=3
		bool is_not{false};
		while(true){
			token tk{t.next_token()};
			if(not tk.is_name("not")){
				t.pushback_token(tk);
				break;
			}
			is_not=not is_not;
			nots_.push_back(move(tk));
		}
		is_not_=is_not;

		lhs_=make_unique<expr_ops_list>(t,true);
		if(lhs_->is_empty())
			throw compiler_error(*this,"expected left hand side of boolean operation");

		if(t.is_next_char('=')){
			op_="=";
		}else if(t.is_next_char('<')){
			if(t.is_next_char('=')){
				op_="<=";
			}else{
				op_="<";
			}
		}else if(t.is_next_char('>')){
			if(t.is_next_char('=')){
				op_=">=";
			}else{
				op_=">";
			}
		}else{
			throw compiler_error(*this,"expected boolean op");
		}
		rhs_=make_unique<expr_ops_list>(t,true);
		if(rhs_->is_empty())// unary
			throw compiler_error(*lhs_,"expected right hand side of boolean operation");
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);

		for(const token&e:nots_)
			e.source_to(os);

		lhs_->source_to(os);
		os<<op_;
		rhs_->source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		throw compiler_error(tok(),"this code should not be reached: "+string{__FILE__}+":"+to_string(__LINE__));
	}

	inline void compile_or(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_true)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,"?",' ',*this);
		tc.asm_label(*this,os,indent_level,cmp_bgn_label(tc));
		resolve("cmp",tc,os,indent_level,*lhs_,*rhs_);
		toc::indent(os,indent_level);
		os<<(is_not_?asm_jxx_for_op_inv(op_):asm_jxx_for_op(op_));
		os<<" "<<jmp_to_if_true<<endl;
	}

	inline void compile_and(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_false)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,"?",' ',*this);
		tc.asm_label(*this,os,indent_level,cmp_bgn_label(tc));
		resolve("cmp",tc,os,indent_level,*lhs_,*rhs_);
		toc::indent(os,indent_level);
		os<<(is_not_?asm_jxx_for_op(op_):asm_jxx_for_op_inv(op_));
		os<<" "<<jmp_to_if_false<<endl;
	}

	inline string cmp_bgn_label(const toc&tc)const{
		const string&call_path=tc.get_inline_call_path(tok());
		return "cmp_"+tc.source_location(tok())+(call_path.empty()?"":"_"+call_path);
	}

private:
	inline static string asm_jxx_for_op(const string&op){
		if(op=="="){
			return "je";
		}else if(op=="<"){
			return "jl";
		}else if(op=="<="){
			return "jle";
		}else if(op==">"){
			return "jg";
		}else if(op==">="){
			return "jge";
		}else{
			throw "unknown op "+op;
		}
	}

	inline static string asm_jxx_for_op_inv(const string&op){
		if(op=="="){
			return "jne";
		}else if(op=="<"){
			return "jge";
		}else if(op=="<="){
			return "jg";
		}else if(op==">"){
			return "jle";
		}else if(op==">="){
			return "jl";
		}else{
			throw "unknown op "+op;
		}
	}

	inline void resolve(const string&op,toc&tc,ostream&os,size_t indent_level,const statement&sa,const statement&sb)const{
		string dest,src;
		vector<string>allocated_registers;
		if(sa.is_expression()){
			dest=tc.alloc_scratch_register(sa,os,indent_level);
			allocated_registers.push_back(dest);
			sa.compile(tc,os,indent_level+1,dest);
		}else{
			const ident_resolved&ir_src{tc.resolve_ident_to_nasm(sa)};
			if(ir_src.is_const()){
				dest=ir_src.as_const();
			}else{
				if(ir_src.negated){
					dest=tc.alloc_scratch_register(sb,os,indent_level);
					allocated_registers.push_back(dest);
					tc.asm_cmd(sb,os,indent_level,"mov",dest,ir_src.id);
					tc.asm_negate(sb,os,indent_level,dest);
				}else{
					dest=ir_src.id;
				}
			}
		}
		if(sb.is_expression()){
			src=tc.alloc_scratch_register(sb,os,indent_level);
			allocated_registers.push_back(src);
			sb.compile(tc,os,indent_level+1,src);
		}else{
			const ident_resolved&ir_src{tc.resolve_ident_to_nasm(sb)};
			if(ir_src.is_const()){
				src=ir_src.as_const();
			}else{
				if(ir_src.negated){
					src=tc.alloc_scratch_register(sb,os,indent_level);
					allocated_registers.push_back(src);
					tc.asm_cmd(sb,os,indent_level,"mov",src,ir_src.id);
					tc.asm_negate(sb,os,indent_level,src);
				}else{
					src=ir_src.id;
				}
			}
		}

		tc.asm_cmd(*this,os,indent_level,op,dest,src);

		// free allocated registers in reverse order
		for(auto it=allocated_registers.rbegin();it!=allocated_registers.rend();++it) {
			const string&reg=*it;
			tc.free_scratch_register(os,indent_level,reg);
		}
	}

	vector<token>nots_;
	bool is_not_;
	unique_ptr<expr_ops_list>lhs_;
	string op_;
	unique_ptr<expr_ops_list>rhs_;
};
