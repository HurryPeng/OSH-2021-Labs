#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <string>
#include <sstream>

struct Pipe
{
    int fd_send;
    int fd_recv;
};

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

void handle_chat(Pipe pipe)
{
    char recvBuffer[1024] = "";
    SendBuffer sendBuffer("Message:");
    //bool newLine = true;
    for
    (
        ssize_t len = recv(pipe.fd_send, recvBuffer, 1000, 0);
        len > 0;
        len = recv(pipe.fd_send, recvBuffer, 1000, 0)
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
                sendBuffer.send(pipe.fd_recv);
                sendBuffer.clear();
            }
            head = tail + 1;
        }
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

    int fd1 = accept(fd, NULL, NULL);
    int fd2 = accept(fd, NULL, NULL);
    if (fd1 == -1 || fd2 == -1)
    {
        perror("accept");
        return 1;
    }

    struct Pipe pipe1;
    struct Pipe pipe2;
    pipe1.fd_send = fd1;
    pipe1.fd_recv = fd2;
    pipe2.fd_send = fd2;
    pipe2.fd_recv = fd1;

    std::thread thread1(handle_chat, pipe1);
    std::thread thread2(handle_chat, pipe2);
    
    thread1.join();
    thread2.join();

    std::cout << "Chatroom closed by clients. \n";

    return 0;
}
