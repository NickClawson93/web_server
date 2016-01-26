all: server stat sigint
server: server.c
	g++ -o server server.c
stat: stat.cpp
	g++ -o stat stat.cpp
sigint: sigint.cpp
	g++ -o sigint sigint.cpp
