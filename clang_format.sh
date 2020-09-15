#!/bin/bash
while read f
do
	echo $f
	clang-format -style=file -i $f
done < <(find -type d \( -path ./build -o -path ./deps -o -path ./libcomp/libobjgen/res \) -prune -o \( -name '*.h' -o -name '*.cpp' \) ! -name 'LookupTable*.h' -print)
