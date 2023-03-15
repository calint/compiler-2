#pragma once
#include<cstddef>
#include<string>

class type{
public:
    inline type(const std::string name,const size_t size):
        name_{name},
        size_{size}
    {}
private:
    std::string name_;
    size_t size_;
};