HEAD=../../../build/release-install-cpp11/include/ # 头文件路径
LIB=../../../build/release-install-cpp11/lib # 库路径

.PHONY:all
all:client server

client:testClient.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp
	
server:testServer.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp

.PHONY:clean
clean:
	rm -f client server

# ==========

# test:test.cpp
# 	g++ -o $@ $^ -std=c++11 -l jsoncpp

# .PHONY:clean
# clean:
# 	rm -f test