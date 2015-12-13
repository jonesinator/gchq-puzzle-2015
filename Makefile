all:
	g++ -g -Wall -Werror -Wextra -std=c++14 gchq.cpp

release:
	g++ -O3 -Wall -Werror -Wextra -std=c++14 gchq.cpp

test: all
	./a.out
