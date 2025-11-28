all: server

server:main.cpp Webserver.cpp HttpConn.cpp Timer.cpp
	g++ -o server main.cpp Timer.cpp HttpConn.cpp Webserver.cpp

clean:
	rm -rf server
