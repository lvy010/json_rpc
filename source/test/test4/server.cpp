#include "../../server/rpc_server.hpp"

int main()
{
    auto server = std::make_shared<JsonRpc::Server::TopicServer>(6666);

    server->start();
    return 0;
}