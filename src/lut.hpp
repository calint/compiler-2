#pragma once
template<class T>class lut{public:

	inline T get(const char*key)const{
		for(auto e:elems)
			if(e.is_key(key))
				return e.data;
		throw "element not found";
	}

	inline T get_valid(const char*key,bool&is_valid)const{
		for(auto e:elems){
			if(e.is_key(key)){
				is_valid=true;
				return e.data;
			}
		}
		is_valid=false;
		return T();
	}

	inline bool has(const char*key)const{
		for(auto&e:elems)
			if(e.is_key(key)){
				return true;
			}
		return false;
	}

	inline void put(const char*key,T data){
		elems.push_back(el{key,data});
	}

	inline void clear(){elems.clear();}


private:
	class el{public:
		const char*key{nullptr};
		T data{nullptr};
		inline bool is_key(const char*k)const{return!strcmp(key,k);}
	};
	vector<el>elems{};
};
