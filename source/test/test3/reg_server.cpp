#include "../../common/util.hpp"
#include "../../server/rpc_server.hpp"

using namespace JsonRpc;

int main()
{
    Server::RegistryServer reg_server(7777);
    reg_server.start();

    return 0;
}