#pragma once
template<class T>class lut{public:

	inline T get(const string&key)const{
		for(auto e:elems)
			if(e.is_key(key))
				return e.data;
		throw "element not found";
	}

	inline T get_valid(const string&key,bool&is_valid)const{
		for(auto e:elems){
			if(e.is_key(key)){
				is_valid=true;
				return e.data;
			}
		}
		is_valid=false;
		return T();
	}

	inline bool has(const string&key)const{
		for(auto e:elems)
			if(e.is_key(key)){
				return true;
			}
		return false;
	}

	inline void put(const string&key,T data){
		elems.push_back(el{key,data});
	}

	inline void clear(){elems.clear();}


private:
	class el{public:
		string key;
		T data;
		inline bool is_key(const string&k)const{return k==key;}
	};
	vector<el>elems{};
//	el*last_has_found{nullptr}
};
