#pragma once

#include"tokenizer.hpp"

class toc;

class unary_ops final{
public:
	inline unary_ops(tokenizer&t){
		while(true){
			if(t.is_next_char('~')){
				ops_.push_back('~');
			}else if(t.is_next_char('-')){
				ops_.push_back('-');
			}else if(t.is_next_char('!')){
				ops_.push_back('!');
			}else{
				break;
			}
		}
	}

	inline unary_ops()=default;
	inline unary_ops(const unary_ops&)=default;
	inline unary_ops(unary_ops&&)=default;
	inline unary_ops&operator=(const unary_ops&)=default;
	inline unary_ops&operator=(unary_ops&&)=default;

	inline bool is_only_negated()const{return ops_.size()==1&&ops_.back()=='-';}

	inline void put_back(tokenizer&t)const{
		size_t i{ops_.size()};
		while(i--){
			t.put_back_char(ops_[i]);
		}
	}

	inline void source_to(ostream&os)const{
		const size_t n{ops_.size()};
		for(size_t i=0;i<n;i++){
			os<<ops_[i];
		}
	}

	inline void compile(toc&tc,ostream&os,size_t indent_level,const string&dst_resolved)const;

	inline bool is_empty()const{return ops_.empty();}

	inline string get_ops_as_string()const{
		return{ops_.begin(),ops_.end()};
	}

	inline long int evaluate_constant(long int v)const{
		size_t i{ops_.size()};
		while(i--){
			switch(ops_[i]){
			case'-':v=-v;break;
			case'~':v=~v;break;
			case'!':v=v^1;break;
			default:throw"unexpected code path "+string{__FILE__}+":"+to_string(__LINE__);
			}
		}
		return v;
	}

private:
	vector<char>ops_;
};
