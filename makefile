all: server

server:main.cpp Webserver.cpp HttpConn.cpp Timer.cpp ./ThreadPool/ThreadPool.cpp
	g++ -o server main.cpp Timer.cpp HttpConn.cpp Webserver.cpp ./ThreadPool/ThreadPool.cpp

clean:
	rm -rf server
