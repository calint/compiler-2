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
				t.put_back_token(tk);
				break;
			}
			is_not=not is_not;
			nots_.push_back(move(tk));
		}
		is_not_=is_not;

		lhs_={t,true};

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
		
		rhs_={t,true};
	}

	inline stmt_if_bool_op()=default;
	inline stmt_if_bool_op(const stmt_if_bool_op&)=default;
	inline stmt_if_bool_op(stmt_if_bool_op&&)=default;
	inline stmt_if_bool_op&operator=(const stmt_if_bool_op&)=default;
	inline stmt_if_bool_op&operator=(stmt_if_bool_op&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);

		for(const token&e:nots_)
			e.source_to(os);

		lhs_.source_to(os);
		os<<op_;
		rhs_.source_to(os);
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst="")const override{
		throw compiler_error(tok(),"this code should not be reached: "+string{__FILE__}+":"+to_string(__LINE__));
	}

	inline void compile_or(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_true)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,"?",' ',*this);
		tc.asm_label(*this,os,indent_level,cmp_bgn_label(tc));
		resolve(tc,os,indent_level,"cmp",lhs_,rhs_);
		toc::indent(os,indent_level);
		os<<(is_not_?asm_jxx_for_op_inv(op_):asm_jxx_for_op(op_));
		os<<" "<<jmp_to_if_true<<endl;
	}

	inline void compile_and(toc&tc,ostream&os,size_t indent_level,const string&jmp_to_if_false)const{
		toc::indent(os,indent_level,true);tc.source_comment(os,"?",' ',*this);
		tc.asm_label(*this,os,indent_level,cmp_bgn_label(tc));
		resolve(tc,os,indent_level,"cmp",lhs_,rhs_);
		toc::indent(os,indent_level);
		os<<(is_not_?asm_jxx_for_op(op_):asm_jxx_for_op_inv(op_));
		os<<" "<<jmp_to_if_false<<endl;
	}

	inline string cmp_bgn_label(const toc&tc)const{
		const string&call_path{tc.get_inline_call_path(tok())};
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

	inline void resolve(toc&tc,ostream&os,size_t indent_level,const string&op,const expr_ops_list&lh,const expr_ops_list&rh)const{
		vector<string>allocated_registers;

		const string&dst{resolve_ident(tc,os,indent_level,lh,allocated_registers)};
		const string&src{resolve_ident(tc,os,indent_level,rh,allocated_registers)};

		tc.asm_cmd(*this,os,indent_level,op,dst,src);

		// free allocated registers in reverse order
		for(auto it{allocated_registers.rbegin()};it!=allocated_registers.rend();++it) {
			const string&reg{*it};
			tc.free_scratch_register(os,indent_level,reg);
		}
	}

	inline string resolve_ident(toc&tc,ostream&os,size_t indent_level,const expr_ops_list&exp,vector<string>&allocated_registers)const{
		if(exp.is_expression()){
			const string&reg{tc.alloc_scratch_register(exp,os,indent_level)};
			allocated_registers.push_back(reg);
			exp.compile(tc,os,indent_level+1,reg);
			return reg;
		}
		
		const ident_resolved&dst_r{tc.resolve_ident_to_nasm(exp)};
		if(dst_r.is_const())
			return exp.get_unary_ops().get_ops_as_string()+dst_r.id;

		if(exp.get_unary_ops().is_empty())
			return dst_r.id;
		
		const string&dst=tc.alloc_scratch_register(exp,os,indent_level);
		allocated_registers.push_back(dst);
		tc.asm_cmd(exp,os,indent_level,"mov",dst,dst_r.id);
		exp.get_unary_ops().compile(tc,os,indent_level,dst);
		return dst;
	}

	vector<token>nots_;
	bool is_not_{}; // not a=b
	expr_ops_list lhs_;
	string op_; // '<', '<=', '>', '>=', '='
	expr_ops_list rhs_;
};
