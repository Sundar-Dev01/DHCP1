#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 5000
#define BUFFER_SIZE 256


int main(void)
{
    int client_socket;
    struct sockaddr_in server_address;
    socklen_t server_length = sizeof(server_address);
    char buffer[BUFFER_SIZE];
    struct timeval timeout = {5, 0};

    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("Socket creation failed");
        return 1;
    }
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Send DHCP Discover */
    strcpy(buffer, "DISCOVER");
    sendto(client_socket, buffer, strlen(buffer), 0,
           (struct sockaddr *)&server_address, sizeof(server_address));

    /* Receive DHCP Offer */
    memset(buffer, 0, sizeof(buffer));
    if (recvfrom(client_socket, buffer, sizeof(buffer) - 1, 0,
                 (struct sockaddr *)&server_address, &server_length) < 0)
    {
        printf("No response from DHCP server. Is it running?\n");
        close(client_socket);
        return 1;
    }

    if (strcmp(buffer, "POOL_FULL") == 0)
    {
        printf("DHCP server has no available IP addresses.\n");
        close(client_socket);
        return 1;
    }

    char offered_ip[32] = {0};
    if (sscanf(buffer, "OFFER|%31s", offered_ip) != 1)
    {
        printf("Invalid DHCP offer received.\n");
        close(client_socket);
        return 1;
    }

    /* Send DHCP Request */
    snprintf(buffer, sizeof(buffer), "REQUEST|%s", offered_ip);
    sendto(client_socket, buffer, strlen(buffer), 0,
           (struct sockaddr *)&server_address, sizeof(server_address));

    /* Receive DHCP Acknowledgement with all configuration details */
    memset(buffer, 0, sizeof(buffer));
    if (recvfrom(client_socket, buffer, sizeof(buffer) - 1, 0,
                 (struct sockaddr *)&server_address, &server_length) < 0)
    {
        printf("No DHCP acknowledgement received.\n");
        close(client_socket);
        return 1;
    }

    if (strncmp(buffer, "ACK|", 4) != 0)
    {
        printf("DHCP allocation failed: %s\n", buffer);
        close(client_socket);
        return 1;
    }

    /* Parse only the values received in the server's ACK message */
    char *token = strtok(buffer, "|");
    char *ip_address = strtok(NULL, "|");
    char *subnet_mask = strtok(NULL, "|");
    char *gateway = strtok(NULL, "|");
    char *dns_server = strtok(NULL, "|");
    char *lease_time = strtok(NULL, "|");

    if (ip_address && subnet_mask && gateway && dns_server && lease_time)
    {
        printf("\n===== DHCP CONFIGURATION RECEIVED FROM SERVER =====\n");
        printf("IP Address   : %s\n", ip_address);
        printf("Subnet Mask  : %s\n", subnet_mask);
        printf("Default GW   : %s\n", gateway);
        printf("DNS Server   : %s\n", dns_server);
        printf("Lease Time   : %s seconds\n", lease_time);
        printf("================================================\n");
    }
    else
    {
        printf("Incomplete configuration received from server.\n");
    }

    (void)token;
    close(client_socket);
    return 0;
}
