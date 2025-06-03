#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  
#include <fcntl.h>     
#include <poll.h>
#include <netinet/tcp.h> 
#include <cmath>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>

using namespace std;

struct udp_message {
    char topic[50];
    uint8_t type;
    char payload[1500];
};

struct tcp_client {
    char id[11];
    vector<string> topics;
    int sockfd;
};

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

/* Functie ce aproba/dezaproba daca un topic al unui subscriber
este potrivit sau nu pentru a fi de interes in momentul in care
se primesc mesaje UDP si clientul trebuie notificat*/
bool isSuitableTopic(string topic_from_udp, string client_topic) {
    char *server_topic = (char *)calloc(topic_from_udp.size() + 1, sizeof(char));
    strcpy(server_topic, topic_from_udp.c_str());
    char * user_topic = (char *)calloc(client_topic.size() + 1, sizeof(char));
    strcpy(user_topic, client_topic.c_str());
    char *server, *user;

    char *token_server = strtok_r(server_topic, "/ \n", &server);
    char *token_user = strtok_r(user_topic, "/ \n", &user);

    while (token_server && token_user) { //daca exista primele cuvinte 
        if (!strcmp(token_user, "*")) { // daca e *, merg cu topic ul de la UDP
        // extrag urmatorul cuvant care este dupa * 
            token_user = strtok_r(NULL, "/ \n", &user) ;  
            if (token_user == NULL) { //nu am nimic dupa, trimit tot
                free(server_topic);
                free(user_topic);
                return true;
            }
            // altfel, am ceva dupa
            while (token_server && strcmp(token_server, token_user) != 0)
                token_server = strtok_r(NULL, "/ \n", &server);
             //am ajuns la final si nu am gasit un match
            if (token_server == NULL) {
                free(server_topic);
                free(user_topic);   
                return false;
            }
            token_user = strtok_r(NULL, "/ \n", &user);
            token_server = strtok_r(NULL, "/ \n", &server);
        } else if (!strcmp(token_user, "+")) {
            token_user = strtok_r(NULL, "/ \n", &user); //escapam doar un cuvant
            token_server = strtok_r(NULL, "/ \n", &server); //cuvantul escapat
        } else if (!strcmp(token_server, token_user)) {
            token_user = strtok_r(NULL, "/ \n", &user);
            token_server = strtok_r(NULL, "/ \n", &server);
        } else {
            free(server_topic);
            free(user_topic);
            return false;
        }
    }

    free(server_topic);
    free(user_topic);
    //daca am ajuns la sfarsit la ambele, inseamna ca au fost identice
    return (token_server == NULL && token_user == NULL);
}

/*Functie ce itereaza prin toti clientii si verifica
daca topic-urile la care sunt abonati sunt de interes
pentru a trimite mesajul primit de la server-ul UDP*/
void send_to_subscribers(char *topic, char* buff_to_send, 
    int len_buffer, const unordered_map<string, tcp_client>clients) {
        string topic_from_udp(topic);
        for (const auto& [id, client] : clients) {
            if (client.sockfd == -1) continue;
            for (const auto& client_topic : client.topics) {
                if(isSuitableTopic(topic_from_udp, client_topic)) {
                    int net_len = htonl(len_buffer);
                    send_message(client.sockfd, (char*)&net_len, 4);
                    send_message(client.sockfd, buff_to_send, len_buffer); 
                    break;
                }
            }
        }
}

/*Parseaza mesajul astfel incat, in functie de tipul
acestuia, sa il transmita clientilor in fomratul
corect*/
void parse_message(sockaddr_in client_addr, char *buffer,
                    const unordered_map<string, tcp_client>&clients) {
    udp_message *message = (udp_message *) calloc(1, sizeof(udp_message));

    memcpy(message->topic, buffer, 50);
    memcpy(&message->type, buffer + 50, 1);
    memcpy(message->payload, buffer + 51, 1500);
    
    char ip_client[16];
    char buff_to_send[1551] = {0};
    int len_buffer = 0;
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_client, 16);
    int port_client = ntohs(client_addr.sin_port);

    switch (message->type) {
        case 0: {// int
            uint8_t sign = message->payload[0];
            uint32_t pay;
            int payload;
            memcpy(&pay, message->payload + 1, sizeof(uint32_t));
            pay = ntohl(pay);
            if (sign)
                payload = -(int)pay;
            else
                payload = (int)pay;
            len_buffer = sprintf(buff_to_send, "%s:%d - %s - INT - %d", 
                        ip_client, port_client, message->topic, payload);
            break;
        }
        case 1: {// short_real
            uint16_t pay;
            memcpy(&pay, message->payload, sizeof(uint16_t));
            pay = ntohs(pay);
            float payload = pay/100.0;
            len_buffer = sprintf(buff_to_send, "%s:%d - %s - SHORT_REAL - %.2f",
                                 ip_client, port_client, message->topic, payload);
            break;
        }
        case 2: { // float
            uint8_t sign = message->payload[0], power;
            uint32_t pay;
            memcpy(&pay, message->payload + 1, sizeof(uint32_t));
            pay = ntohl(pay);
            memcpy(&power, message->payload + 1 + sizeof(uint32_t),
            sizeof(uint8_t));
            double payload = pay * pow(10, -power);
            if (sign == 1)
                payload = -payload;
            len_buffer = sprintf(buff_to_send, "%s:%d - %s - FLOAT - %.4f",
                        ip_client, port_client, message->topic, payload);
            break;
        }
        case 3: // string
            len_buffer = snprintf(buff_to_send, sizeof(buff_to_send), 
                    "%s:%d - %s - STRING - %.1000s", ip_client, port_client, 
                    message->topic, message->payload);
            break;
        default:
            break;
    }
    send_to_subscribers(message->topic, buff_to_send, len_buffer, clients);
    free(message); 

}

/*Functie ce primeste un mesaj de la un
UDP si il parseaza*/
void receive_udp(int udp_socket, const unordered_map<string, tcp_client>&clients) {
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t addr_len = sizeof(client_addr);
    char buffer[1551];

    int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
                                (struct sockaddr *)&client_addr, &addr_len);
    buffer[bytes_received] = '\0';
    parse_message(client_addr, buffer, clients);
}


/*Functie ce verifica un nou client in momentul in care
pe canalul de listen vine ceva*/
void accept_client(int tcp_socket, vector<pollfd>&pfds,
    unordered_map<string, tcp_client>&clients, unordered_map<int, 
    string>&sock_to_id) {
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));
    int client_sock = accept(tcp_socket, (struct sockaddr*)&client_addr,
                            &client_size);
    //dezactivez algoritmul lui Naglae, exact cum a fost specificat in cerinta
    int flag = 1;
    setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    char len[1];
    recieve_message(client_sock, len, 1);
    uint8_t id_len;
    memcpy(&id_len, len, 1);
    char id[11] ;
    memset(id, 0, sizeof(id));
    int n = recieve_message(client_sock, id, id_len);
    if (n < 0) {
        // clientul s-a deconectat brusc
        close(client_sock);
        return;
    }
    id[id_len] = '\0';
    string id_client(id);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);
    //il adaug pentru a putea fi iterat de fiecare data in main
    pollfd new_entry;
    new_entry.events = POLLIN;
    new_entry.fd = client_sock;
    new_entry.revents = 0;
    pfds.push_back(new_entry);

    //daca e un client nou, il adaug in hashmap-uri
    if (!clients.count(id_client)) {
        tcp_client new_client;
        new_client.sockfd = client_sock;
        clients[id_client] = new_client;
        sock_to_id[client_sock] = id_client;
        cout << "New client " << id_client << " connected from "
        << ip_str << ":" << ntohs(client_addr.sin_port) << endl;
    } else {
        //daca e deja activ
        if (clients[id_client].sockfd != -1) {
            close(client_sock);
            for (size_t i = 3; i < pfds.size(); ++i)
                if(pfds[i].fd == client_sock) {
                    pfds.erase(pfds.begin() + i);
                    break;
                }
            cout <<  "Client " << id_client <<" already connected." << endl;
            return;
        //daca sockfd e -1 inseamna ca s-a reconectat
        } else {
            sock_to_id[client_sock] = id_client;
            clients[id_client].sockfd = client_sock;
            cout << "New client " << id_client << " connected from "
            << ip_str << ":" << ntohs(client_addr.sin_port) << endl;
        }
    }
}


/*Functie ce primeste de la un client TCP o structura de date 
implementata de mine ce contine comanda efectiva si, in caz
de subscribe/unsubscribe si topic-ul la care face referire
comanda.*/

int client_request(unordered_map<string, tcp_client>&clients,
    unordered_map<int, string>&sock_to_id, int client_sock, vector<pollfd>&pfds){

    int ret;
    string client_id = sock_to_id[client_sock];
    //primi doi bytes din mesaj reprezinta lungimea totala a mesajului
    char len[2];
    //in len am uint_16 care stocheaza len in host format
    ret = recieve_message(client_sock, len, 2);
    if (ret == -1)
        return -1;

    uint16_t request_len = 0;
    memcpy(&request_len, len, 2);
    request_len = ntohs(request_len);
    char request[70] = {0}; 
    ret = recieve_message(client_sock, request, request_len);
    if (ret == -1)
        return -1;
    
    uint8_t command, topic_len;
    memcpy(&command, request, 1); // nu trebuie convertita in host, are 8 bytes
    memcpy(&topic_len, request + 1, 1);
    char topic[51];
    memcpy(topic, request + 2, topic_len);
    topic[topic_len] = '\0';

    if (command == 0) {
        string new_topic(topic);
        auto& topics = clients[client_id].topics;
        bool is_already = false;
        for (auto& topic : topics)
            if (topic == new_topic) {
                is_already = true;
                break;
            }
        //daca nu este in topic-urile clinetului deja, il adaug
        if (!is_already)
            clients[client_id].topics.push_back(new_topic);
    } else if (command == 1) {
        string topic_to_remove(topic); 
        for (size_t i = 0; i < clients[client_id].topics.size(); ++i)
            if (clients[client_id].topics[i] == topic_to_remove) {
                clients[client_id].topics.erase(clients[client_id].topics.begin() + i);
                break;
            }
    } else if (command == 2) {
        //pastrez in hashmap clientul, in caz de se reconecteaza
        clients[client_id].sockfd = -1;
        send(client_sock, "", 1, 0);
        close(client_sock);
        for (size_t i = 3; i < pfds.size(); ++i)
            if (pfds[i].fd == client_sock) {
                pfds.erase(pfds.begin() + i);
                break;
            }
        //il sterg din acest hasmap, deoarece data viitoare poate
        //primi un alt sockfd
        sock_to_id.erase(client_sock);
        cout << "Client " << client_id << " disconnected." << endl;
        cout.flush();
    } 
    return 0;
}



int main(int argc, char *argv[]) {
    if (argc != 2)
        return 0;
    setvbuf(stdout, NULL, _IONBF, BUFSIZ); 
    unordered_map<int, string> sock_to_id; //sockfd -> id
    unordered_map<string, tcp_client> clients; // id -> tcp
    int tcp_socket, udp_socket; 
    sockaddr_in serveraddr;

    //deschid socketii pentru a comunica
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(atoi(argv[1]));
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    //leg server-ul la socketi
    bind(udp_socket, (sockaddr *)&serveraddr, sizeof(serveraddr));
    bind(tcp_socket, (sockaddr *)&serveraddr, sizeof(serveraddr));

    listen(tcp_socket, 4096);

    vector<pollfd> pfds;
    pollfd stinput, udp, tcp;

    memset(&stinput, 0, sizeof(stinput));
    memset(&udp, 0, sizeof(udp));
    memset(&tcp, 0, sizeof(udp));

    stinput.fd = STDIN_FILENO;
    stinput.events = POLLIN;

    udp.fd = udp_socket;
    udp.events = POLLIN;

    tcp.fd = tcp_socket;
    tcp.events = POLLIN;

    pfds.push_back(stinput);
    pfds.push_back(udp);
    pfds.push_back(tcp);
    
    while(1) {
        poll(pfds.data(), pfds.size(), -1); // astept orice comunicare
        if ((pfds[0].revents & POLLIN)) { // stdin
            char command_from_server[1551] = {0};
            if (!fgets(command_from_server, sizeof(command_from_server), stdin))
                continue;
            if (!strncmp(command_from_server, "exit", 4)) {
                for (auto socket : pfds)
                    close(socket.fd);
            exit(0);
            }
        }
        if (pfds[1].revents & POLLIN) { // udp socket
            receive_udp(udp_socket, clients);
        }
        if (pfds[2].revents & POLLIN) {
            accept_client(tcp_socket, pfds, clients, sock_to_id);
        }
        //iterez prin toti clientii de fiecare data
        for (size_t i = 3; i < pfds.size(); ++i) {
            //daca doresc sa comunice
            if (pfds[i].revents & POLLIN) {
                int n = client_request(clients, sock_to_id, pfds[i].fd, pfds);
                //inseamna ca ceva rau s-a intamplat
                if (n == -1) {
                    i--;
                }
            }
        }
    }
}