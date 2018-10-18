#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"
#include "seasocks/util/Json.h"

#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <memory>
#include <cstring>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

using namespace seasocks;
using namespace std;

namespace {

    const char usage[] = "Usage: %s [-p PORT] [-v] DIR\n"
            "   Serves files from DIR over HTTP on port PORT\n";

}

class MyHandler : public WebSocket::Handler {
public:

    explicit MyHandler(Server* server) : _server(server), _currentValue(0) {
        setValue(1);
    }

    virtual void onConnect(WebSocket* connection) {
        _connections.insert(connection);
        connection->send(_currentSetValue.c_str());
        cout << "Connected: " << connection->getRequestUri()
                << " : " << formatAddress(connection->getRemoteAddress())
                << endl;
        cout << "Credentials: " << *(connection->credentials()) << endl;
    }

    virtual void onData(WebSocket* connection, const char* data) {
        if (0 == strcmp("die", data)) {
            _server->terminate();
            return;
        }
        if (0 == strcmp("close", data)) {
            cout << "Closing.." << endl;
            connection->close();
            cout << "Closed." << endl;
            return;
        }

        int value = atoi(data) + 1;
        if (value > _currentValue) {
            setValue(value);
            for (auto c : _connections) {
                c->send(_currentSetValue.c_str());
            }
        }
    }

    virtual void onDisconnect(WebSocket* connection) {
        _connections.erase(connection);
        cout << "Disconnected: " << connection->getRequestUri()
                << " : " << formatAddress(connection->getRemoteAddress())
                << endl;
    }

private:
    set<WebSocket*> _connections;
    Server* _server;
    int _currentValue;
    string _currentSetValue;

    void setValue(int value) {
        _currentValue = value;
        _currentSetValue = makeExecString("set", _currentValue);
    }
};

int main(int argc, char* const argv[]) {
    int port = 80;
    bool verbose = false;
    int opt;
    while ((opt = getopt(argc, argv, "vp:")) != -1) {
        switch (opt) {
            case 'v': verbose = true;
                break;
            case 'p': port = atoi(optarg);
                break;
            default:
                fprintf(stderr, usage, argv[0]);
                exit(1);
        }
    }
    if (optind >= argc) {
        fprintf(stderr, usage, argv[0]);
        exit(1);
    }

    auto logger = std::make_shared<PrintfLogger>(verbose ? Logger::Level::DEBUG : Logger::Level::ACCESS);
//    Server server(logger);
//    auto root = argv[optind];
//    std::cout << root << std::endl;
//    server.serve(root, port);
    
    
    Server ws_server(logger);

    auto handler = std::make_shared<MyHandler>(&ws_server);
    ws_server.addWebSocketHandler("/ws", handler);
    ws_server.serve(".", 9090);
    return 0;
}