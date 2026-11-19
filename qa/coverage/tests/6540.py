#!/usr/bin/python3

# prints each comment of the listing that quotes a string of '6540.baz' once,
# without its location; a continued string is on one line without its line
# end, a carriage return left in it would print as '\x0d'
seen = []
for line in open('gen.s', encoding='utf-8'):
    text = line.split('] ', 1)[-1].rstrip('\n')
    if line.lstrip()[:1] in (';', '#') and '"' in text and text not in seen:
        seen.append(text)

for text in seen:
    print(text)
