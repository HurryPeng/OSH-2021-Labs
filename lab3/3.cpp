#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <string>
#include <sstream>
#include <vector>
#include <fcntl.h>

const int MAX_CONNECTION = 4;

struct SendBuffer
{
    static const int SEND_SIZE;
    std::string title;
    std::string buffer;

    SendBuffer() = default;

    void append(const char *head, std::streamsize count)
    {
        buffer.append(head, count);
    }

    bool readyToSend() const
    {
        return buffer.back() == '\n';
    }

    void send(int fd)
    {
        ::send(fd, title.c_str(), title.size(), 0);
        const char *str = buffer.c_str();
        const char *head = str;
        for (; head - str + SEND_SIZE < buffer.length(); head += SEND_SIZE)
            ::send(fd, head, SEND_SIZE, 0);
        ::send(fd, head, buffer.length() - (head - str), 0);
    }

    void clear()
    {
        buffer.clear();
    }
};

const int SendBuffer::SEND_SIZE = 1000;

struct Client
{
    bool connected = false;
    int fd = 0;
    SendBuffer sendBuffer;
};

std::vector<Client> clients(MAX_CONNECTION);

int handle_chat(const int id)
{
    int fd = clients[id].fd;
    char recvBuffer[1024] = "";
    std::string sendTitle = "[";
    sendTitle += std::to_string(id) + "]:";

    ssize_t len = recv(fd, recvBuffer, 1000, 0);
    if (len <= 0) return -1; // Connection closed
    recvBuffer[len] = '\0';
    for (char *head = recvBuffer; head != recvBuffer + len; )
    {
        char *tail = strchr(head, '\n'); // find '\n'
        // points to last character if not found
        if (tail == nullptr) tail = recvBuffer + len - 1;
        clients[id].sendBuffer.append(head, tail - head + 1);
        if (clients[id].sendBuffer.readyToSend())
        {
            for (int i = 0; i < MAX_CONNECTION; ++i)
            {
                if (i == id) continue;
                if (clients[i].connected == false) continue;
                clients[id].sendBuffer.send(clients[i].fd);
            }
            clients[id].sendBuffer.clear();
        }
        head = tail + 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int port = atoi(argv[1]);
    int fd;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket");
        return 1;
    }
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    socklen_t addr_len = sizeof(addr);
    if (bind(fd, (sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind");
        return 1;
    }
    if (listen(fd, 2))
    {
        perror("listen");
        return 1;
    }

    while (true)
    {
        // Accept connections
        while (true) 
        {
            // Non-blocking accept()
            int clientFd = accept(fd, nullptr, nullptr);
            if (clientFd == -1 && errno == EAGAIN)
            {
                // No more pending requests
                break;
            }
            else if (clientFd == -1)
            {
                perror("accept");
                return 1;
            }

            // Handle new connection
            int clientId = -1;

            // Check if there's room for the new client
            for (int i = 0; i < MAX_CONNECTION; ++i)
            {
                if (clients[i].connected == false)
                {
                    clientId = i;
                    break;
                }
            }

            // Refuse if full
            if (clientId == -1)
            {
                send(clientFd, "Chatroom full, connection refused. \n", 36, 0);
                {
                    std::cout << "Connection refused. \n";
                }
                shutdown(clientFd, 2);
                close(clientFd);
                continue;
            }
            
            // Accept
            clients[clientId].connected = true;
            clients[clientId].fd = clientFd;
            clients[clientId].sendBuffer.title = "[" + std::to_string(clientId) + "]:";
            clients[clientId].sendBuffer.clear();
            fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL, 0) | O_NONBLOCK);
            send(clientFd, "Welcome to the chatroom! Your ID is ", 36, 0);
            std::string clientIdStr = std::to_string(clientId);
            send(clientFd, clientIdStr.c_str(), clientIdStr.length(), 0);
            send(clientFd, "\n", 1, 0);
            {
                std::cout << "Connection accepted for client " << clientId << '\n';
            }
        }

        // Handle messages
        fd_set fdSetClients;
        FD_ZERO(&fdSetClients);
        int maxFdClient = 0;
        for (int i = 0; i < MAX_CONNECTION; ++i)
        {
            if (clients[i].connected)
            {
                FD_SET(clients[i].fd, &fdSetClients);
                if (clients[i].fd > maxFdClient) maxFdClient = clients[i].fd;
            }
        }
        if (maxFdClient == 0) continue;
        timeval timeout = {0, 100000};
        if (select(maxFdClient + 1, &fdSetClients, nullptr, nullptr, &timeout) > 0)
        {
            for (int i = 0; i < MAX_CONNECTION; ++i)
            {
                if (clients[i].connected && FD_ISSET(clients[i].fd, &fdSetClients))
                {
                    if (handle_chat(i) == -1) // Connection closed
                    {
                        clients[i].connected = false;
                        clients[i].sendBuffer.clear();
                        std::cout << "Connection closed by client " << i << '\n';
                    }
                }
            }
        }
    }

    return 0;
}
