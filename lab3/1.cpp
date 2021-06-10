#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>

struct Pipe
{
    int fd_send;
    int fd_recv;
};

void handle_chat(Pipe pipe)
{
    char buffer[1024] = "";
    bool newLine = true;
    for
    (
        ssize_t len = recv(pipe.fd_send, buffer, 1000, 0);
        len > 0;
        len = recv(pipe.fd_send, buffer, 1000, 0)
    )
    {
        for (char *head = buffer; head != buffer + len; )
        {
            char *tail = strchr(head, '\n'); // find '\n'
            // points to last character if not found
            if (tail == nullptr) tail = buffer + len - 1;
            if (newLine)
            {
                send(pipe.fd_recv, "Message:", 8, 0);
                newLine = false;
            }
            send(pipe.fd_recv, buffer, tail - head + 1, 0);
            if (*tail == '\n') newLine = true;
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
