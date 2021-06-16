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
#include <optional>
#include <coroutine>

const int MAX_CONNECTION = 32;

auto asyncRecv(int fd, char * buffer, std::size_t n)
{
    struct Awaitable
    {
        int fd;
        char * buffer;
        std::size_t n;

        int len;
        bool recvSuccess;

        bool await_ready()
        {
            len = recv(fd, buffer, n, 0);
            if (len == -1 && errno == EAGAIN)
            {
                recvSuccess = false;
                return false;
            }
            recvSuccess = true;
            return true;
        }
        void await_suspend(std::coroutine_handle<> h) {}
        std::optional<int> await_resume() const
        {
            if (recvSuccess) return len;
            return {};
        }
    };

    return Awaitable{ fd, buffer, n };
}

struct HandleChatCoroutine
{
    struct promise_type
    {
        HandleChatCoroutine get_return_object()
        {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {};
    };

    std::coroutine_handle<promise_type> handle;

    HandleChatCoroutine(std::coroutine_handle<promise_type> h) : handle(h) {}
};

struct Client
{
    bool connected = false;
    int fd = 0;
    std::coroutine_handle<HandleChatCoroutine::promise_type> handle;
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

const int SendBuffer::SEND_SIZE = 1000;

HandleChatCoroutine handle_chat(const int id)
{
    int fd = clients[id].fd;
    char recvBuffer[1024] = "";
    std::string sendTitle = "[";
    sendTitle += std::to_string(id) + "]:";
    SendBuffer sendBuffer(sendTitle);

    while (true)
    {
        int len = 0;
        while (true)
        // Keep awaiting (returning control to event loop) until receive is finished
        {
            if (auto optionalLen = co_await asyncRecv(fd, recvBuffer, 1000))
            {
                len = optionalLen.value();
                break;
            }
        }

        if (len <= 0) break; // Connection closed
        
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
                    if (clients[i].connected == false) continue;
                    sendBuffer.send(clients[i].fd);
                }
                sendBuffer.clear();
            }
            head = tail + 1;
        }
    }

    // Connection closed
    clients[id].connected = false;
    std::cout << "Connection closed by client " << id << '\n';
}

auto asyncAccept(int fd)
{
    struct Awaitable
    {
        int fd;

        int clientFd;
        bool acceptSuccess;

        bool await_ready()
        {
            clientFd = accept(fd, nullptr, nullptr);
            if (clientFd == -1 && errno == EAGAIN)
            {
                // No more pending requests
                acceptSuccess = false;
                return false;
            }
            else if (clientFd == -1)
            {
                perror("accept");
                return 1;
            }
            acceptSuccess = true;
            return true;
        }
        void await_suspend(std::coroutine_handle<> h) {}
        std::optional<int> await_resume() const
        {
            if (acceptSuccess) return clientFd;
            return {};
        }
    };

    return Awaitable{ fd };
}

struct AcceptCoroutine
{
    struct promise_type
    {
        AcceptCoroutine get_return_object()
        {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }
        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {};
    };

    std::coroutine_handle<promise_type> handle;

    AcceptCoroutine(std::coroutine_handle<promise_type> h) : handle(h) {}
};

AcceptCoroutine handle_accept(int fd)
{
    while (true)
    {
        int clientFd = 0;
        
        while (true)
        // Keep awaiting (returning control to event loop) until a new request is caught
        {
            if (auto optionalClientFd = co_await asyncAccept(fd))
            {
                clientFd = optionalClientFd.value();
                break;
            }
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
            std::cout << "Connection refused. \n";
            shutdown(clientFd, 2);
            close(clientFd);
            continue;
        }
        
        // Accept
        fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL, 0) | O_NONBLOCK);
        clients[clientId].connected = true;
        clients[clientId].fd = clientFd;
        clients[clientId].handle = handle_chat(clientId).handle;
        send(clientFd, "Welcome to the chatroom! Your ID is ", 36, 0);
        std::string clientIdStr = std::to_string(clientId);
        send(clientFd, clientIdStr.c_str(), clientIdStr.length(), 0);
        send(clientFd, "\n", 1, 0);
        std::cout << "Connection accepted for client " << clientId << '\n';
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

    std::coroutine_handle<AcceptCoroutine::promise_type> acceptorHandle = handle_accept(fd).handle;

    while (true) // Coroutine event loop
    {
        acceptorHandle.resume();
        for (int i = 0; i < MAX_CONNECTION; ++i)
        {
            if (clients[i].connected == false) continue;
            clients[i].handle.resume();
        }
    }

    return 0;
}
