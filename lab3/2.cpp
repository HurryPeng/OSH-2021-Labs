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

const int MAX_CONNECTION = 4;

std::mutex mutexStdio;

struct Client
{
    bool connected = false;
    int fd = 0;
    std::mutex mutex;
    // Controls modification of the current client info
    // and ensures only one thread at a time
    // sends message to the current client
};

std::vector<Client> clients(MAX_CONNECTION);

struct SendBuffer
{
    static const int SEND_SIZE;
    std::string title;
    std::string buffer;

    SendBuffer(const std::string & _title) : title(_title) {}

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

const int SendBuffer::SEND_SIZE = 1024;

void handle_chat(const int id)
{
    int fd = clients[id].fd;
    char recvBuffer[1024] = "";
    std::string sendTitle = "[";
    sendTitle += std::to_string(id) + "]:";
    SendBuffer sendBuffer(sendTitle);

    for
    (
        ssize_t len = recv(fd, recvBuffer, 1000, 0);
        len > 0;
        len = recv(fd, recvBuffer, 1000, 0)
    )
    {
        recvBuffer[len] = '\0';
        for (char *head = recvBuffer; head != recvBuffer + len; )
        {
            char *tail = strchr(head, '\n'); // find '\n'
            // points to last character if not found
            if (tail == nullptr) tail = recvBuffer + len - 1;
            sendBuffer.append(head, tail - head + 1);
            if (sendBuffer.readyToSend())
            {
                for (int i = 0; i < MAX_CONNECTION; ++i)
                {
                    if (i == id) continue;
                    std::lock_guard<std::mutex> lock(clients[i].mutex);
                    if (clients[i].connected == false) continue;
                    sendBuffer.send(clients[i].fd);
                }
                sendBuffer.clear();
            }
            head = tail + 1;
        }
    }

    // Connection closed
    std::lock_guard<std::mutex> lock(clients[id].mutex);
    clients[id].connected = false;
    {
        std::lock_guard<std::mutex> lock(mutexStdio);
        std::cout << "Connection closed by client " << id << '\n';
    }
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
        int clientFd = accept(fd, nullptr, nullptr);
        if (clientFd == -1)
        {
            perror("accept");
            return 1;
        }
        int clientId = -1;

        // Check if there's room for the new client
        for (int i = 0; i < MAX_CONNECTION; ++i)
        {
            std::lock_guard<std::mutex> lock(clients[i].mutex);
            if (clients[i].connected == false)
            {
                clientId = i;
                break;
            }
        }

        // Refuse if full
        if (clientId == -1)
        {
            send(clientFd, "Chatroom full, connection refused\n", 34, 0);
            {
                std::lock_guard<std::mutex> lock(mutexStdio);
                std::cout << "Connection refused. \n";
            }
            shutdown(clientFd, 2);
            close(clientFd);
            continue;
        }
        
        // Accept
        {
            std::lock_guard<std::mutex> lock(clients[clientId].mutex);
            clients[clientId].connected = true;
            clients[clientId].fd = clientFd;
        }
        send(clientFd, "Welcome to the chatroom! Your ID is ", 36, 0);
        std::string clientIdStr = std::to_string(clientId);
        send(clientFd, clientIdStr.c_str(), clientIdStr.length(), 0);
        send(clientFd, "\n", 1, 0);
        {
            std::lock_guard<std::mutex> lock(mutexStdio);
            std::cout << "Connection accepted for client " << clientId << '\n';
        }

        std::thread(handle_chat, clientId).detach();
    }

    return 0;
}
