cat gen.s |grep -v -e'^;.*' -e'^\s*_end_.*' -e'^\s*$' -e'^[a-zA-z].*$' -e '^\s*_loop_.*$'
