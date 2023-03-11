#pragma once

template<class T>
class lut{
public:
	inline T get(const string&key)const{
		if(last_has_el){
			if(last_has_el->is_key(key)){
				return last_has_el->data;
			}
		}
		for(const auto&e:elems_){
			if(e.is_key(key)){
				return e.data;
			}
		}
		throw "element not found";
	}

	inline bool has(const string&key)const{
		for(const auto&e:elems_){
			if(e.is_key(key)){
				last_has_el=&e;
				return true;
			}
		}
		return false;
	}

	inline void put(const string&key,T data){
		elems_.emplace_back(el{key,data});
	}

private:
	class el{
	public:
		string key;
		T data;
		inline bool is_key(const string&k)const{return k==key;}
	};
	vector<el>elems_;
	mutable const el*last_has_el{nullptr};
};
