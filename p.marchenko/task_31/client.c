#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./socket"

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_un addr;
    char buffer[1024];
    const char *name = (argc > 1) ? argv[1] : "CLIENT";

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); exit(1); }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("connect"); exit(1);
    }
    printf("[%s] Подключен к серверу.\n", name);

    int count = 1;
    while (1) {
        /* Формируем сообщение с меткой клиента */
        snprintf(buffer, sizeof(buffer), "[%s] hello from client! msg #%d\n", name, count++);
        
        if (write(sock, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
        
        /* Задержка для демонстрации параллельности */
        sleep(2); 
    }

    close(sock);
    return 0;
}
