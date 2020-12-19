#include <stdio.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>

#define SERV "127.0.0.1"    // adresse IP = boucle locale
#define PORT 1101            // port d’ecoute serveur
int port, sock;                // n°port et socket
char mess[64];                // chaine de caracteres
char contenu[64];
char ok[2];
FILE *fichier = NULL;

struct sockaddr_in serv_addr; // zone adresse
struct hostent *server;   // nom serveur


struct request {
    char *mode;
    char *fichier;
    int isOK;
};

struct request build_request(char mess[]) {
    struct request rqt;
    char *p = strtok(mess, " ");
    if (p == NULL) {
        printf("Mode manquant\n");
        rqt.isOK = 0;
        return rqt;
    }
    rqt.mode = p;
    if (strcmp(p, "QUIT") == 0) {
        rqt.isOK = 1;
        return rqt;
    }
    p = strtok(NULL, " ");
    if (p == NULL) {
        printf("Fichier manquant\n");
        rqt.isOK = 0;
        return rqt;
    }
    rqt.fichier = p;
    p = strtok(NULL, " ");
    if (p != NULL) {
        printf("Trop d'arguments\n");
        rqt.isOK = 0;
        return rqt;
    }
    rqt.isOK = 1;
    return rqt;
}

void creer_socket() {
    port = PORT;
    server = gethostbyname(SERV);    // verification existance adresse
    if (!server) {
        fprintf(stderr, "Problème serveur \"%s\"\n", SERV);
        exit(1);
    }
    // creation socket locale
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur de la socket");
        exit(1);
    }    // creation socket
    bzero(&serv_addr, sizeof(serv_addr));    // preparation champs entete
    serv_addr.sin_family = AF_INET;          // Type d’adresses
    bcopy(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);        // port de connexion du serveur
}

int main(){ // creation socket
    creer_socket();
    // connexion au serveur
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Il n'y a pas de serveur en ligne");
        exit(1);
    } // connexion à l'application du dessus
    printf("Entrez un mode et le fichier a transversé: ");
    fgets(mess, sizeof(mess), stdin);
    int i = strlen(mess);
    mess[(i - 1)] = '\0';
    send(sock, mess, sizeof(mess), 0);
    recv(sock, ok, sizeof(ok), 0);
    if (strcmp(ok, "OK") != 0) {
        printf("Erreur\n");
        close(socket);
        exit(1);
    }
    struct request request = build_request(mess);
    if (strcmp(request.mode, "GET") == 0) {
        fichier = fopen(request.fichier, "w+");
        printf("Copie du fichier\n");
        recv(sock, contenu, sizeof(contenu), 0);
        while (strcmp(contenu, "FIN") != 0) {
            printf("%s", contenu);
            fputs(contenu, fichier);
            recv(sock, contenu, sizeof(contenu), 0);
        }
        printf("\nFichier reçu\n");
        close(fichier);
        send(sock, "OK", sizeof("OK"), 0);
    } else if (strcmp(request.mode, "PUT") == 0) {
        fichier = fopen(request.fichier, "r");
        if (fichier == NULL) {
            printf("Le fichier %s n'existe pas\n", request.fichier);
            close(sock);
            exit(1);
        }
        while (fgets(contenu, sizeof(contenu), fichier) != NULL) {
            printf("%s", contenu);
            send(sock, contenu, sizeof(contenu), 0);
        }
        send(sock, "FIN", sizeof("FIN"), 0);
        recv(sock, ok, sizeof(ok), 0);
        if (strcmp(ok, "OK") == 0) {
            printf("\nFichier copié\n");
        }
    } else if (strcmp(request.mode, "QUIT") == 0) {
        recv(sock, ok, sizeof(ok), 0);
        if (strcmp(ok, "OK") == 0) {
            printf("Adieu\n");
        }
    } else {
        printf("Erreur, le mode n'est pas conforme\n");
    }
    close(sock);
    return 0;
}