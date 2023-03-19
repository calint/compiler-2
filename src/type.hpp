#pragma once
#include<cstddef>
#include<string>

class type{
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

private:
    std::string name_;
    size_t size_;
};