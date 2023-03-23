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
	inline type(const std::string name,const size_t size):
		name_{name},
		size_{size}
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
		if(path.size()==1)
			return"qword[rbp"+string{stack_idx_base>0?"+":""}+to_string(stack_idx_base)+"]";

		const size_t offset=field_offset(tk,path);
		const int stack_idx=stack_idx_base-int(offset);
		return"qword[rbp"+string{stack_idx>0?"+":""}+to_string(stack_idx)+"]";
	}

	inline size_t size()const{return size_;}

	inline void set_size(const size_t nbytes){size_=nbytes;}

	inline const string&name()const{return name_;}

	inline void set_name(const string&nm){name_=nm;}

private:
	inline size_t field_offset(const token&tk,const vector<string>&path)const{
		const type*tp{this};
		size_t offset{0};
		size_t path_idx{1};
		while(true){
			if(path_idx==path.size())
				return offset;
			const type_field&fld=tp->field(tk,path[path_idx]);
			offset+=fld.offset;
			if(fld.tp.name()=="qword"){ // ? built-int
				return offset;
			}
			tp=&fld.tp;
			path_idx++;
		}
	}

	std::string name_;
	size_t size_{};
	vector<type_field>fields_;
};
