#pragma once

class stmt_whitespace final:public statement{
public:
	inline stmt_whitespace(token tk):
		statement{move(tk)}
	{}
};
