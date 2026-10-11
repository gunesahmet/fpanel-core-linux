#pragma once
#ifndef CORE_TCPSERVER_H
#define CORE_TCPSERVER_H
#include <execinfo.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <list>

#include <cstdio>
#include <cstring>

#include <functional>

#include <cerrno>
#include <strings.h>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/lexical_cast.hpp>

#include <Logger/Logger.h>
#include <Helpers/Helpers.h>
#include <Config/Config.h>
#include <Crypto/Crypto.h>
#include <Commands/Commands.h>

#include <Rapidjson/document.h>
#include <Rapidjson/writer.h>
#include <Rapidjson/stringbuffer.h>


using namespace rapidjson;

class TcpServer {
private:
    std::string _host;
    int _port;
    Logger _logger;
    Config *_config;
    Crypto _crypto;
    Helpers _helpers;
    Commands *_commands;

    static void errorHandler(int sig) {
        size_t nStackTraces = 20; //number of backtraces you want at most
        void *array[nStackTraces];
        size_t size;
        //fills array and returns actual number of backtraces at the moment
        size = backtrace(array, nStackTraces);
//        printf("signal no %d \n", sig);
        //prints array to std error after converting array to
        //human-readable strings
        backtrace_symbols_fd(array, size, STDERR_FILENO);
        exit(0);
    };

    std::string getIpAddress(int newfd)
    {
        sockaddr_in SockAddr;
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        int res = getpeername(newfd, (struct sockaddr *)&addr, &addr_size);
        char *clientip = new char[20];
        strcpy(clientip, inet_ntoa(addr.sin_addr));
        int port = ntohs(addr.sin_port);
        _logger.Debug("[IP: %s Connected On PORT: %d]", clientip, port);
        return std::string(clientip);
    }

    int connectionHandler(void *socket_desc)
    {
        auto start = std::chrono::steady_clock::now();
        int clientSock = *(int*) socket_desc;
        std::string rcv;
        int bytesReceived = 0;
        const int MAX_BUF_LENGTH = 2048;
        char buffer[MAX_BUF_LENGTH];
        bool closed = false;
        while(true) {
            bytesReceived = recv(clientSock, buffer, sizeof(buffer), 0);
            if (bytesReceived < 0) {
                _logger.Debug("Receiving data failed");
                break;
            }
            else if(bytesReceived == 0) {
                _logger.Debug("Client disconnected. Received byte is 0");
                closed = true;
                break;
            }
            else {
                rcv.append(buffer, bytesReceived);
                if(bytesReceived < MAX_BUF_LENGTH) {
                    _logger.Debug("");
                    break;
                }
            }
        }
        if(closed) {
            _logger.Debug("Client disconnected. returning 1");
            close(clientSock);
            return 1;
        }
        std::string command = _crypto.Decrypt(rcv, _config->getValue("secret.key"), _config->getValue("secret.iv"));
        if(command.length() == 0) {
            _logger.Debug("rcv: " + rcv);
            _logger.Debug("Command length is 0");
            close(clientSock);
            return 1;
        }

        if(_helpers.commandIsValid(command)) {
            _logger.Debug("Command is valid");
            initializeRequest(command, clientSock);
        }
        else {
            _logger.Debug("Not valid Command: %s", command.c_str());
            Document d;
            const char *json = command.c_str();
            if (!d.Parse(json).HasParseError()) {
                if(d.HasMember("job")) {
                    std::string job = d["job"].GetString();
                    if(d.HasMember("domain_name")) {
                        if(std::string(job).compare("add-domain") == 0) {
                            _logger.Debug("Domain adding");
                            std::string ip_addr = std::string(d["ip_addr"].GetString());
                            std::string domain_name = std::string(d["domain_name"].GetString());
//                            _domain_map->addDomain(domain_name, ip_addr);
                        }
                        else if(std::string(job).compare("delete-domain") == 0) {
                            std::string domain_name = std::string(d["domain_name"].GetString());
//                            _domain_map->removeDomain(domain_name);
                        }
                    }
                }
            }
            else {
                _logger.Debug("Received: %s", rcv.c_str());
                _logger.Debug("Invalid command");
            }
        }

        _logger.Debug("Socket closed");
        _logger.Debug("Message cleared");
        _logger.Debug("=============================");
        _logger.Debug("=============================");
        _logger.Debug("=============================");
        rcv.clear();
        _logger.Debug("Socket client closing");
        close(clientSock);
        _logger.Debug("Socket client closed");
        return 0;
    }

    void initializeRequest(const std::string& command, int clientSock)
    {
        std::string response = _commands->Run(command);
        Send(clientSock, response);
    }
public:
    TcpServer& SetLogger(Logger logger)
    {
        _logger = logger;
        return *this;
    }

    TcpServer& SetConfig(Config *config)
    {
        _config = config;
        return *this;
    }

    TcpServer& SetHost(const std::string& host)
    {
        _host = std::move(host);
        return *this;
    }

    TcpServer& SetCommands(Commands *commands)
    {
        _commands = commands;
        return *this;
    }

    TcpServer& SetPort(int port)
    {
        _port = port;
        return *this;
    }

    void Send(int clientSock, std::string &response)
    {
        unsigned int error_code;
        unsigned int error_code_size = sizeof(error_code);
        signal(SIGSEGV, errorHandler);
        std::string crypted_response = _crypto.Encrypt(response, _config->getValue("secret.key"), _config->getValue("secret.iv")) + "\n";
        if(send(clientSock, crypted_response.c_str(), strlen(crypted_response.c_str()), 0) < 0) {
            _logger.Error("Send failed to client");
        }
    }

    void Start()
    {
        struct sockaddr_in saServer{};
        int listenSocket;

        struct sockaddr clientAddress;
        socklen_t sizeOfClientAddress = sizeof(clientAddress);
        listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == listenSocket) {
            _logger.Error("Could not create socket");
            exit(EXIT_FAILURE);
        }

        int enable = 1;
        if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
            _logger.Error("setsockopt(SO_REUSEADDR) failed");
        }
        signal(SIGSEGV, errorHandler);

        uint16_t port = uint16_t(_port);

        saServer.sin_family = AF_INET;
        saServer.sin_addr.s_addr = inet_addr(_host.c_str());
        saServer.sin_port = htons(port);

        if (-1 == bind(listenSocket, (struct sockaddr *) &saServer, sizeof (saServer))) {
            _logger.Error("Error : Bind Failed");
            close(listenSocket);
            exit(EXIT_FAILURE);
        }
        if (-1 == listen(listenSocket, 1000)) {
            _logger.Error("Error : Listen Failed");
            close(listenSocket);
            exit(EXIT_FAILURE);
        }
        /**
         * Running infinite loop for reading client messages
         */
        while(true) {
            try {
                int new_socket = accept(listenSocket, &clientAddress, &sizeOfClientAddress);
                std::string connected_ip = getIpAddress(new_socket);
                if (connected_ip.compare(_config->getValue("socket.allowed.address")) == 0 || connected_ip.compare("127.0.0.1") == 0) {
                    _logger.Debug("Client allowed");
                    std::thread t(&TcpServer::connectionHandler, this, (void*) &new_socket);
                    t.detach();
                }
                else {
                    close(new_socket);
                    _logger.Debug("Client disallowed");
                }
            }
            catch (const std::exception& e) {
                std::string what = std::string(e.what());
                _logger.Error("Error : Socket accept. Exception: %s", what.c_str());
                continue;
            }
        }
    }
};
#endif //CORE_TCPSERVER_H