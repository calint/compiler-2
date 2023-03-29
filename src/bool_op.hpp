#pragma once

#include"expr_ops_list.hpp"

class bool_op final:public statement{
public:
	inline bool_op(toc&tc,tokenizer&t):
		statement{t.next_whitespace_token()}
	{
		set_type(tc.get_type_bool());

		bool is_not{false};
		// if not a=3
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

		lhs_={tc,t,true};

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
			// if a ...
			is_shorthand_=true;
			return;
			// throw compiler_error(*this,"expected boolean operation '=','<','<=','>','>='");
		}
		
		rhs_={tc,t};
	}

	inline bool_op()=default;
	inline bool_op(const bool_op&)=default;
	inline bool_op(bool_op&&)=default;
	inline bool_op&operator=(const bool_op&)=default;
	inline bool_op&operator=(bool_op&&)=default;

	inline void source_to(ostream&os)const override{
		statement::source_to(os);

		for(const token&e:nots_)
			e.source_to(os);

		lhs_.source_to(os);

		if(not is_shorthand_){
			os<<op_;
			rhs_.source_to(os);
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent,const string&dst="")const override{
		throw compiler_error(tok(),"this code should not be reached: "+string{__FILE__}+":"+to_string(__LINE__));
	}

	inline void compile_or(toc&tc,ostream&os,size_t indent,const string&jmp_to_if_true,const bool inverted)const{
		const bool invert{inverted?not is_not_:is_not_};
		toc::indent(os,indent,true);tc.source_comment(os,"?",inverted?" 'or' inverted: ":" ",*this);
		tc.asm_label(*this,os,indent,cmp_bgn_label(tc));
		if(is_shorthand_){
			resolve_cmp_shorthand(tc,os,indent,lhs_);
			toc::indent(os,indent);
			os<<(not invert?asm_jxx_for_op_inv("="):asm_jxx_for_op("="));
			os<<" "<<jmp_to_if_true<<endl;
			return;
		}
		resolve_cmp(tc,os,indent,lhs_,rhs_);
		toc::indent(os,indent);
		os<<(invert?asm_jxx_for_op_inv(op_):asm_jxx_for_op(op_));
		os<<" "<<jmp_to_if_true<<endl;
	}

	inline void compile_and(toc&tc,ostream&os,size_t indent,const string&jmp_to_if_false,const bool inverted)const{
		const bool invert{inverted?not is_not_:is_not_};
		toc::indent(os,indent,true);tc.source_comment(os,"?",inverted?" 'and' inverted: ":" ",*this);
		tc.asm_label(*this,os,indent,cmp_bgn_label(tc));
		if(is_shorthand_){
			resolve_cmp_shorthand(tc,os,indent,lhs_);
			toc::indent(os,indent);
			os<<(not invert?asm_jxx_for_op("="):asm_jxx_for_op_inv("="));
			os<<" "<<jmp_to_if_false<<endl;
			return;
		}
		resolve_cmp(tc,os,indent,lhs_,rhs_);
		toc::indent(os,indent);
		os<<(invert?asm_jxx_for_op(op_):asm_jxx_for_op_inv(op_));
		os<<" "<<jmp_to_if_false<<endl;
	}

	inline string cmp_bgn_label(const toc&tc)const{
		const string&call_path{tc.get_inline_call_path(tok())};
		return "cmp_"+tc.source_location_for_label(tok())+(call_path.empty()?"":"_"+call_path);
	}

	inline const string&identifier()const override{
		assert(not is_expression());

		return lhs_.identifier();

		throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
	}

	inline bool is_expression()const override{
		if(is_not_)
			return true;

		if(lhs_.is_expression())
			return true;

		const string&id=lhs_.identifier();
		if(id=="true" or id=="false")
			return false;

		return true;
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
			throw"unknown op "+op;
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
			throw"unknown op "+op;
		}
	}

	inline void resolve_cmp(toc&tc,ostream&os,size_t indent,const expr_ops_list&lh,const expr_ops_list&rh)const{
		vector<string>allocated_registers;

		const string&dst{resolve_expr(tc,os,indent,lh,allocated_registers)};
		const string&src{resolve_expr(tc,os,indent,rh,allocated_registers)};

		tc.asm_cmd(*this,os,indent,"cmp",dst,src);

		// free allocated registers in reverse order
		for(auto it{allocated_registers.rbegin()};it!=allocated_registers.rend();++it) {
			const string&reg{*it};
			tc.free_scratch_register(os,indent,reg);
		}
	}

	inline void resolve_cmp_shorthand(toc&tc,ostream&os,size_t indent,const expr_ops_list&lh)const{
		vector<string>allocated_registers;

		const string&dst{resolve_expr(tc,os,indent,lh,allocated_registers)};

		tc.asm_cmd(*this,os,indent,"cmp",dst,"0");

		// free allocated registers in reverse order
		for(auto it{allocated_registers.rbegin()};it!=allocated_registers.rend();++it) {
			const string&reg{*it};
			tc.free_scratch_register(os,indent,reg);
		}
	}

	inline string resolve_expr(toc&tc,ostream&os,size_t indent,const expr_ops_list&exp,vector<string>&allocated_registers)const{
		if(exp.is_expression()){
			const string&reg{tc.alloc_scratch_register(exp,os,indent)};
			allocated_registers.push_back(reg);
			exp.compile(tc,os,indent+1,reg);
			return reg;
		}
		
		const ident_resolved&id_r{tc.resolve_identifier(exp,true)};
		if(id_r.is_const())
			return exp.get_unary_ops().get_ops_as_string()+id_r.id;

		if(exp.get_unary_ops().is_empty())
			return id_r.id;
		
		const string&reg=tc.alloc_scratch_register(exp,os,indent);
		allocated_registers.push_back(reg);
		tc.asm_cmd(exp,os,indent,"mov",reg,id_r.id);
		exp.get_unary_ops().compile(tc,os,indent,reg);
		return reg;
	}

	vector<token>nots_;
	expr_ops_list lhs_;
	string op_; // '<', '<=', '>', '>=', '='
	expr_ops_list rhs_;
	bool is_not_{}; // if not a=b
	bool is_shorthand_{}; // if a ...
};
