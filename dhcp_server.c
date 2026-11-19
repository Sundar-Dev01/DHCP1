#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 5000
#define POOL_SIZE 5

int main(void)
{
    int server_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_length = sizeof(client_address);
    char buffer[256];
    const char *ip_pool[POOL_SIZE] = {
        "192.168.1.100", "192.168.1.101", "192.168.1.102",
        "192.168.1.103", "192.168.1.104"
    };
    int next_ip = 0;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) return 1;

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address,
             sizeof(server_address)) < 0)
    {
        close(server_socket);
        return 1;
    }

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        if (recvfrom(server_socket, buffer, sizeof(buffer) - 1, 0,
                     (struct sockaddr *)&client_address, &client_length) < 0)
            continue;

        if (strcmp(buffer, "DISCOVER") == 0)
        {
            char offer[64];
            if (next_ip < POOL_SIZE)
                snprintf(offer, sizeof(offer), "OFFER|%s", ip_pool[next_ip]);
            else
                snprintf(offer, sizeof(offer), "POOL_FULL");

            sendto(server_socket, offer, strlen(offer), 0,
                   (struct sockaddr *)&client_address, client_length);
        }
        else if (strncmp(buffer, "REQUEST|", 8) == 0)
        {
            char requested_ip[32] = {0};
            char ack[160];

            if (sscanf(buffer, "REQUEST|%31s", requested_ip) == 1 &&
                next_ip < POOL_SIZE && strcmp(requested_ip, ip_pool[next_ip]) == 0)
            {
                snprintf(ack, sizeof(ack),
                         "ACK|%s|255.255.255.0|192.168.1.1|8.8.8.8|60",
                         requested_ip);
                sendto(server_socket, ack, strlen(ack), 0,
                       (struct sockaddr *)&client_address, client_length);
                next_ip++;
            }
            else
            {
                const char reject[] = "NAK";
                sendto(server_socket, reject, strlen(reject), 0,
                       (struct sockaddr *)&client_address, client_length);
            }
        }
    }


    close(server_socket);
    return 0;
}
