#pragma once

class stmt_whitespace final:public statement{
public:
	inline stmt_whitespace(const token&tk):
		statement{tk}
	{}
};
