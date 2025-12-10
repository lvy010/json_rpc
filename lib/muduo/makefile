HEAD=../../build/release-install-cpp11/include/ # 头文件路径
LIB=../../build/release-install-cpp11/lib # 库路径

.PHONY: all
all: server.exe client.exe

server.exe: server.cpp #                            net 必须在 base 前面连接
	g++ -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread

client.exe: client.cpp
	g++ -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread

.PHONY:clean
clean:
	rm -f server.exe client.exe
