HEAD=../../../build/release-install-cpp11/include/ # 头文件路径
LIB=../../../build/release-install-cpp11/lib # 库路径

.PHONY:all
all:publish_client server subscribe_client

publish_client:publish_client.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp
	
server:server.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp

subscribe_client:subscribe_client.cpp
	g++ -g -o $@ $^ -std=c++11 -I $(HEAD) -L $(LIB) -l muduo_net -l muduo_base -l pthread -l jsoncpp

.PHONY:clean
clean:
	rm -f publish_client server subscribe_client 
    #                           subcribe_client