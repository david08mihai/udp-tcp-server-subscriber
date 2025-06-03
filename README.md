--Tema 2--

In cadrul acestei teme am implementat atat un server cat si un client de tip TCP.
Pentru a fi clara diferenta, voi pune in serie cele doua fisiere si le voi descrie.

--Server--

Initial, am implementat primirea unui mesaj de la udp(recieve_udp), unde primeam
intr-un buffer mesajul provenit de la UDP si il parsam exact ca in enunt, pentru
a putea mai apoi sa trimit clientilor TCP subscribed la acel topic. 
In parse_message am luat adresa IP si port-ul clientului UDP si, in functie de 
tipul mesajului, am creat intr-un buffer mesajul respectiv, avand grija la 
transformarile ntohs/ntohl. Mai apoi, mesajul este trimis spre a fi verificat
si a fi expediat catre clientii interesati.
Intrucat conexiunea TCP este una orientata, accept fiecare client ascultand
pe socket-ul de listen, iar primul mesaj provenit din client este ID-ul, 
asa am convenit. Verific daca il am, iar daca nu il am, il adaug, iar
daca il am, verific daca este conectat sau a fost deconectat.
Am creat doua hashmap-uri:
1. Cheia socket cu valoarea ID : l am folosit pentru ca atunci and un client
al carui socket il stiu din pollfd[i].fd se conecteaza, am nevoie de id-ul
clientului pentru a cauta in structura lui sa vad ce topic-uri urmaresc. 
2. Cheia ID cu valoarea Structura TCP_Client, tocmai , stiind acum ID-ul,
pot lua vectorul de topic-uri.
Fiecare client TCP are o structura in care am retinut id-ul, topic-urile la
care s-a abonat si socket-ul pe care a accesat server-ul. Socket-ul a fost
nevoie sa-l tin minte pentru ca, atunci cand se deconecteaza, intrarea
nu trebuie sa dispara, ci doar setez socket-ul la -1, stiind ca daca va fi
nevoie, il voi reconecta. 
Pentru a parsa wildcard-urile am folosit strtok_r, intrucat aveam doua siruri
de parsat in acelasi timp. Daca intalneam * luam urmatorul cuvant si vedeam 
daca in udp exista acel cuvant la un moment dat, iar daca aveam + saream peste
un nivel in ambele topic-uri(server + client). Aceasta functie intorcea daca
un client e potrivit pentru a-i fi trimis un mesaj venit de la server-ul UDP.
Client request primeste mesajul TCP si verifica daca doreste "subscribe", 
"unsubscribe" sau "exit", pentru a putea actualiza structura clientului.
In caz de "subscribe", se verifica daca nu exista deja topic-ul adaugat, iar
daca nu este, il adaug in vectorul de string-uri. Daca este unsubscribe, elimin,
iar daca este exit, inchid socket-ul corespunzator, marchez cu -1 socket-ul din
structura, pentru ca se poate reconecta, nu vreau sa pierd topic-urile la care
s-a abonat si il sterg din vectorul de pollfd, pentru ca nu mai are rost sa astept,
daca el s-a deconectat deja.
In main, adaug initial cele 3 canale intr-un vector de pollfd, STDIN, udp_socket
si tcp_scoket, iar mai apoi, la fiecare client nou, se va adauga si socket-ul
clientului respectiv.


--Subscriber--

Initial, imi creasem o strucutra in care aveam comanda si topic-ul pe care
trebuia sa le trimit la server, insa, dupa ce m-am uitat mai atent, inainte
sa trimit, am inteles ca structurile, din cauza padding-ului sau a altor
inadvertente, pot pierde date. Asa ca, am recurs la a trimite un char 
buffer care contine, tipul comenzii - uint_8t, nu trebuie mai mult de un byte
(0 - subscribe, 1- unsubscribe, 2 -exit), lungimea topic-ului, tot pe un byte,
lungimea unui topic putand fi maxim 50 + 1, iar dupa aceea, un sir de caractere
seminificand topic-ul la care s-a abonat/dezabonat clientul. Insa, la inceput,
am trimis dimensiunea buffer-ului care urma sa vina, pentru a sti exact server-ul/
client-ul cat are de asteptat. La primirea in server a comenzii, despachetez
intr-un alt buffer de maxim 70 caractere(am adaugat mai mult de 60 in caz de orice)
si stiu ca primul byte este 0, 1 sau 2, al doilea reprezinta lungimea topicului pe
care o pun in alt buffer. 
Am creat si doua functii care asteapta trimiterea/primirea buffer-ului, pentru a
nu se trimite bucati de date sau date incomplete. 