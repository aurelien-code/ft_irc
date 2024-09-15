#include "server.hpp"
#include <iostream>
#include <sstream>

Server::Server(std::string& port, std::string& password)
{
    _password = password;
    std::istringstream  is(port);
    int                 i;

    is >> i;
    _port = i;

    //Check if port is in a good range !!!
}

Server::Server(const Server& ref)
{
    *this = ref;
}

Server::~Server()
{
    //Add things to free memory later here;
}

Server  &Server::operator=(const Server& ref)
{
    if (this != &ref)
    {
        _password = ref._password;
        _port = ref._port;
        _serverSocket = ref._serverSocket;
        _fds = ref._fds;
        _clients = ref._clients;
    }

    return (*this);
}


bool    Server::initialize()
{
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1)
        return (false);
    
    int flags = fcntl(_serverSocket, F_GETFL, 0);
    fcntl(_serverSocket, F_SETFL, flags | O_NONBLOCK);

    
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        return (false);
    }

    if (listen(_serverSocket, SOMAXCONN) == -1)
    {
        return (false);
    }

    pollfd  serverPollFd = {_serverSocket, POLLIN, 0};
    _fds.push_back(serverPollFd);

    return (true);
}

void    Server::run()
{
    while (true)
    {
        int poll_result = poll(&_fds[0], _fds.size(), -1);
        
        if (poll_result > 0)
        {
            for (size_t i = 0; i < _fds.size(); ++i)
            {
                if (_fds[i].revents & POLLIN)
                {
                    if (_fds[i].fd == _serverSocket)
                        acceptNewConnection();
                    else
                        handleClientMessage(_fds[i].fd);
                }
            }
        }
    }
}

void    Server::acceptNewConnection()
{
    sockaddr_in client_address;
    socklen_t   client_address_len;
    int         client_socket;
    int         flags;
    pollfd      new_client;

    client_address_len = sizeof(client_address);
    client_socket = accept(_serverSocket, (struct sockaddr*)&client_address, &client_address_len);

    if (client_socket == -1)
    {
        std::cerr << "[ERROR]   Issue during acceptNewConnection()" << std::endl;
        return ;
    }

    flags = fcntl(client_socket, F_GETFL, 0);
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);

    new_client.fd = client_socket;
    new_client.events = POLLIN;
    _fds.push_back(new_client);

    std::cout << "[INFO]    New connection (fd = " << new_client.fd << ")\n";
}

void    Server::handleClientMessage(int clientSocket)
{
    std::cout << "hcm -> " << clientSocket << std::endl;
    return ;
}

void    Server::removeClient(int clientSocket)
{
    std::cout << "rc -> " << clientSocket << std::endl;
    return ;
}