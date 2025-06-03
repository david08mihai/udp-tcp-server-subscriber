#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>
#include <vector>
#include <algorithm>
#include <unistd.h>  
#include <signal.h>

using namespace std;


/*Functie ce asigura transmiterea completa a unui mesaj catre
un client, evitand problemele cauza de send().*/
int send_message(int sockfd, char *buffer, int len) {
    char *iter = buffer;
    for (int bytes_sent = 0; bytes_sent < len;) {
        int bytes = send(sockfd, iter, len - bytes_sent, 0);
        if (bytes <= 0) 
            return -1; 
        bytes_sent += bytes;
        iter += bytes;
    }
    return 0;
}

/*Functie ce primeste un mesaj de la un server, asteptand
exact lungimea stiuta a fi a buffer-ului, apelandu-se
recv() pana se primesc toti bytes asteptati.*/
int recieve_message(int client_sock, char *request, int len) {
    char *iter = request;
    for (int received_bytes = 0; received_bytes < len;) {
        int bytes = recv(client_sock, iter, len - received_bytes, 0);
        if (bytes <= 0) 
            return -1;
        iter += bytes;
        received_bytes += bytes;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4)
        exit(1);
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    struct sockaddr_in server_addr;

    int socketfd;
    char message[70] = {0}, response[1551] = {0};
    memset(&server_addr, 0, sizeof(server_addr));
    socketfd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[3]));
    server_addr.sin_addr.s_addr = inet_addr(argv[2]);

    string id(argv[1]);
    //ma conectez la server pentru fiecare client in parte
    connect(socketfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    uint8_t id_len = id.size();
    send(socketfd, &id_len, 1, 0);
    send(socketfd, id.c_str(), id_len, 0); // trimit id ul primul

    vector<pollfd> pfds;
    pollfd stinput, sock;
    memset(&stinput, 0, sizeof(stinput));
    memset(&sock, 0, sizeof(sock));
    stinput.fd = STDIN_FILENO;
    stinput.events = POLLIN;

    sock.fd = socketfd;
    sock.events = POLLIN;

    pfds.push_back(stinput);
    pfds.push_back(sock);
    while (1) {
        poll(pfds.data(), pfds.size(), -1);

        if (pfds[0].revents & POLLIN) {
            bool exit = false;
            if (!fgets(message, sizeof(message), stdin))
                continue;
            char request[70];
            char *token = strtok(message, " \n");
            while (token) {
                if (!strcmp(token, "subscribe")) {
                    token = strtok(NULL, " \n"); // topic
                    if (!token) continue;

                    uint8_t topic_len = strlen(token);
                    uint8_t command = 0;
                    uint16_t total_len = 2 + topic_len;
                    uint16_t network_len = htons(total_len);

                    memcpy(request, &command, 1);
                    memcpy(request + 1, &topic_len, 1);
                    memcpy(request + 2, token, topic_len);

                    send_message(socketfd, (char*)&network_len, 2);
                    send_message(socketfd, request, total_len);

                    cout << "Subscribed to topic " << token << endl;
                } else if(!strcmp(token, "unsubscribe")) {
                    token = strtok(NULL, " \n"); //topic
                    if (!token) continue;

                    uint8_t topic_len = strlen(token);
                    uint8_t command = 1;
                    uint16_t total_len = 2 + topic_len;
                    uint16_t network_len = htons(total_len);

                    memcpy(request, &command, 1);
                    memcpy(request + 1, &topic_len, 1);
                    memcpy(request + 2, token, topic_len);

                    send_message(socketfd, (char*)&network_len, 2);
                    send_message(socketfd, request, total_len);
                    cout << "Unsubscribed from topic " << token << endl; 
                } else if (!strcmp(token, "exit")) {
                    uint16_t total_len = 1;
                    uint16_t  network_len = htons(total_len); 
                    uint8_t command  = 2;

                    memcpy(request, &command, 1);
                    exit = true;

                    send_message(socketfd, (char*)&network_len, 2);
                    send_message(socketfd, request, total_len);
                }
                else
                    break; // comanda incorecta
                token = strtok(NULL, " \n");
            }

        if (exit) {
            char dummy;
            while (recv(socketfd, &dummy, 1, 0) > 0);
            close(socketfd);
            return 0;
        }
        } else if (pfds[1].revents & POLLIN) {
            char message_len[4];
            int n;
            //prima data primesc lungimea buffer-ului
            n = recieve_message(pfds[1].fd, message_len, 4);
            if (n < 0)
                break;
            int total_len;
            memcpy(&total_len, message_len, 4);
            //convertesc pentru a sti valoareai in host order
            total_len = ntohl(total_len);
            // astept mesajul de lungime total_len
            n = recieve_message(pfds[1].fd, response, total_len);
            response[total_len] = '\0';
            cout << response << endl;
        }
    }
    close(socketfd);
    return 0;
}