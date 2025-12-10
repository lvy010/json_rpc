HEAD=../../../build/release-install-cpp11/include/ # 头文件路径
LIB=../../../build/release-install-cpp11/lib # 库路径

.PHONY:all
all:rpc_client rpc_server reg_server

rpc_client:rpc_client.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp
	
rpc_server:rpc_server.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp

reg_server:reg_server.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp

.PHONY:clean
clean:
	rm -f rpc_client rpc_server reg_server