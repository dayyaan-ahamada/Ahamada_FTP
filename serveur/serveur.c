#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>


#define PORT 1101

int sock, socket2, lg;
char mess[64];
char contenu[64];
char ok[2];
struct sockaddr_in local;   // IP expediteur
struct sockaddr_in distant; // IP destinataire

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
        rqt.fichier = NULL;
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
    // preparation des champs d’entete
    bzero(&local, sizeof(local));        // Mise à zero du champ adresse
    local.sin_family = AF_INET;          // C'est un champ internet
    local.sin_port = htons(PORT);        // Le port est 12345
    local.sin_addr.s_addr = INADDR_ANY;  // Le systeme met l'adresse IP de la machine
    bzero(&(local.sin_zero), 8);          // Mise à zero du champ des options de local

    lg = sizeof(struct sockaddr_in);
    // Créer la socket
    // Si ça bug renvois une erreur et exit le programme
    // AF_INIT = pour internet
    // SOCK_STREAM = Mode connecté
    // 0 = TCP
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erreur de la socket");
        exit(1);
    }
}

int main() {
    // Initialisation du socket d'écoute sock
    creer_socket();
    // Attribution d'un port à sock avec local en param d'adresse
    if (bind(sock, (struct sockaddr *) &local, sizeof(struct sockaddr)) == -1) {
        perror("Erreur au bind");
        exit(1);
    }
    // mise en service avec une file d'attente de max 5 connexions
    if (listen(sock, 5) == -1) {
        perror("Erreur au listen");
        exit(1);
    }
    // Le socket boucle à l'infini
    while (1) { // se met en attente d'une demande de connexion sur sock et créer un socket du socket2
        // avec les paramettre d'adresse distante du client/
        printf("En attente de client\n");
        if ((socket2 = accept(sock, (struct sockaddr *) &distant, &lg)) == -1) {
            perror("Erreur au accept");
            exit(1);
        }
        pid_t ret = fork();

        if (ret == 0) {
            recv(socket2, mess, sizeof(mess), 0);
            struct request request = build_request(mess);

            if (request.isOK == 0) {
                close(socket2);
                exit(1);
            } else {
                printf("Mode : %s\n", request.mode);
                printf("Fichier : %s\n", request.fichier);
                FILE *fichier = NULL;
                if (strcmp(request.mode, "GET") == 0) {
                    fichier = fopen(request.fichier, "r");
                    if (fichier != NULL) {
                        printf("ok\n");
                        send(socket2, "OK", 2, 0);
                        printf("Envoi du fichier\n");
                        while (fgets(contenu, sizeof(contenu), fichier) != NULL) {
                            printf("%s", contenu);
                            send(socket2, contenu, sizeof(contenu), 0);
                        }
                        printf("\nFichier envoyé\n");
                        send(socket2, "FIN", sizeof("FIN"), 0);
                        recv(socket2, ok, sizeof(ok), 0);
                        if (strcmp(ok, "OK") != 0) {
                            printf("Erreur\n");
                            close(socket2);
                            exit(1);
                        }
                        fclose(fichier);
                    } else {
                        printf("%s : Le fichier n'existe pas\n", request.fichier);
                        send(socket2, "Le fichier n'existe pas\n", sizeof("Le fichier n'existe pas\n"), 0);
                        close(socket2);
                        exit(1);
                    }
                } else if (strcmp(request.mode, "PUT") == 0) {
                    printf("Copie du fichier\n");
                    fichier = fopen(request.fichier, "w+");
                    send(socket2, "OK", 2, 0);
                    recv(socket2, contenu, sizeof(contenu), 0);
                    while (strcmp(contenu, "FIN") != 0) {
                        fputs(contenu, fichier);
                        recv(socket2, contenu, sizeof(contenu), 0);
                    }
                    fclose(fichier);
                    send(socket2, "OK", 2, 0);
                    printf("Fichier copié\n");
                } else if (strcmp(request.mode, "QUIT") == 0) {
                    printf("Quitter\n");
                    send(socket2, "OK", 2, 0);
                } else {
                    printf("%s : le mode n'est pas conforme\n", request.mode);
                    send(socket2, "le mode n'est pas conforme", sizeof("le mode n'est pas conforme"), 0);
                }
                close(socket2);
                exit(1);
            }
        }
        close(socket2);
        printf("Connexion réussie : %i\n", ret);
        // On ferme le socket2 une fois la requete terminé et on se remet en attente de connexion
    }
    close(sock);
    return 0;
}
