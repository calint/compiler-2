#pragma once

class stmt_whitespace final:public statement{
public:
	inline stmt_whitespace(const statement&parent,const token&tk):
		statement{parent,tk}
	{}
};
