#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#include "stmt_block.hpp"
#include "compiler_error.hpp"
#include "statement.hpp"
#include "toc.hpp"
#include "token.hpp"
#include "tokenizer.hpp"

class stmt_def_func_param final:public statement{public:

	inline stmt_def_func_param(const statement&parent,tokenizer&t):
		statement{parent,t.next_token()}
	{
		assert(!tok().is_name(""));

		if(!t.is_next_char(':'))
			return;

		while(true){
			if(t.is_eos())throw compiler_error(*this,"unexpected end of stream",tok().name_copy());
			keywords_.push_back(t.next_token());
			if(t.is_next_char(':'))
					continue;
			break;
		}
	}

	inline void source_to(ostream&os)const override{
		statement::source_to(os);
		if(!keywords_.empty()){
			os<<":";
			const size_t sz=keywords_.size()-1;
			size_t i{0};
			for(auto&t:keywords_){
				t.source_to(os);
				if(i++!=sz)os<<":";
			}
		}
	}
	vector<token>keywords_;
private:
};
