#pragma once
#include<cstddef>
#include<string>

class type;
struct type_field final{
	const string name;
	const type&tp;
	const size_t offset{};
};

class type final{
public:
	inline type(const std::string name,const size_t size,const bool is_built_in):
		name_{name},
		size_{size},
		is_built_in_{is_built_in}
	{}

	inline type()=default;
	inline type(const type&)=default;
	inline type(type&&)=default;
	inline type&operator=(const type&)=default;
	inline type&operator=(type&&)=default;

	inline void add_field(const token&tk,const string&name,const type&tp){
		fields_.emplace_back(type_field{name,tp,size_});
		size_+=tp.size_;
	}
	inline const type_field&field(const token&tk,const string&name)const{
		for(const type_field&fld:fields_){
			if(fld.name==name)
				return fld;
		}
		throw compiler_error(tk,"field '"+name+"' not found in type '"+name_+"'");
	}

	inline const string accessor(const token&tk,const vector<string>&path,const int stack_idx_base)const{
		const pair<size_t,size_t>res{field_size_and_offset(tk,path)};
		const size_t offset{res.second};
		const size_t fldsize{res.first};
		const int stack_idx=stack_idx_base+int(offset);

		const string&memsize{get_memory_operand_for_size(tk,fldsize)};

		return memsize+"[rbp"+string{stack_idx>0?"+":""}+to_string(stack_idx)+"]";
	}

	inline string get_memory_operand_for_size(const token&tk,const size_t size)const{
		switch(size){
		case 8:return"qword";
		case 4:return"dword";
		case 2:return"word";
		case 1:return"byte";
		default:throw compiler_error(tk,"illegal size for memory operand: "+to_string(size));
		}
	}

	inline size_t size()const{return size_;}

	inline void set_size(const size_t nbytes){size_=nbytes;}

	inline const string&name()const{return name_;}

	inline void set_name(const string&nm){name_=nm;}

	inline bool is_built_in()const{return is_built_in_;}

private:
	inline pair<size_t,size_t>field_size_and_offset(const token&tk,const vector<string>&path)const{
		if(path.size()==1){
			// path contains only a reference to the variable
			// find first primitive type
			const type*t{this};
			while(true){
				if(t->fields_.empty())
					return{t->size_,0};
				t=&t->fields_[0].tp;
			}
		}
		const type*tp{this};
		size_t offset{0};
		size_t path_idx{1};
		while(true){
			if(path_idx==path.size())
				return{tp->fields_.empty()?size_:tp->fields_[0].tp.size_,offset};

			const type_field&fld=tp->field(tk,path[path_idx]);
			offset+=fld.offset;
			if(fld.tp.is_built_in_){
				return{fld.tp.size_,offset};
			}
			tp=&fld.tp;
			path_idx++;
		}
	}

	std::string name_;
	size_t size_{};
	vector<type_field>fields_;
	bool is_built_in_{};
};
