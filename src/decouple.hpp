#pragma once

inline unique_ptr<statement>create_statement_from_tokenizer(tokenizer&t);
inline unique_ptr<statement>create_statement_from_tokenizer(token tk,tokenizer&t);
