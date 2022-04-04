// Universidade Federal do Paraná
// TE355 - Sistemas Operacionais Embarcados
// Base para o Trabalho 1 - 2022
// Prof. Pedroso

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define PROTOPORT       5193            /* default protocol port number */
#define QLEN            6               /* size of request queue        */
#define ARRAY_LEN 1000
#define STR_LEN 1024

int     visits=0;                       /* counts client connections    */
int     alteracoes=0;
char    msg [ARRAY_LEN][ARRAY_LEN];
int     ditados=0;
sem_t m;

void LeDitado(const char *str)
{
    FILE *arq;
    ditados = 0;
    if ((arq=fopen(str,"r")) == NULL) {
        printf("\n Erro lendo arquivo ...\n\n");
        exit(0);
        }
    while (!feof(arq)) {

       fgets(msg[ditados],ARRAY_LEN,arq);
       // para debug
       //printf("%d %s",ditados,msg[ditados]);
       ditados=(ditados+1)%ARRAY_LEN;
    }
    printf("\n\nCarregou %d ditados",ditados);
    fclose(arq);

}

void GravaDitado(const char *str)
{
    FILE *arq;
    int n = ditados;
    if ( (arq=fopen(str,"w")) == NULL ) {
        printf("\n Erro lendo arquivo ...\n\n");
        exit(0);
        }

    for(int i = 0; i < ditados; i++) {
        fputs(msg[i], arq);
        n--;
        // para debug
        //printf("%d %s",i,msg[i]);
    }
    fclose(arq);
    printf("\n\nSalvou %d ditados",ditados-n);

}

char uppercase(char *input) {
    char i=0;
    for(i = 0; (i<strlen(input)) && (i<STR_LEN); i++) {
        input[i] = toupper(input[i]);
    }
    return i;
}

void remove_element(int index)
{
    int i;
    for(i = index; i < ARRAY_LEN - 1; i++){
        strcpy(msg[i], msg[i + 1]);
    }
}

void busca(int sd, char *input){
    int i;
    char str[STR_LEN];
    printf("\nBuscando:%s", input);
    for(i = 0; i < ARRAY_LEN - 1; i++){
        //strcpy(str, msg[i]);
        if(strstr(msg[i], input) != NULL){
            sprintf(str,"\nDitado %d: %s ", i, msg[i]);
            send(sd,str,strlen(str),0);
        }
    }
}

int palavras_d(char *str) {
    int c = 0, sep = 0, i;
    for(i = 0; i < STR_LEN - 1; i++){
        if (isspace(str[i])) {
            sep = 1;
        } else {
            c += sep;
            sep = 0;
        }
    }
    return c;
}

void *atendeConexao( void *sd2 )
{
    int *temp=sd2;
    int sd=*temp;
    char str[STR_LEN], str2[STR_LEN-100], *endptr;

    int i=0, b, val, rc;

        while (1) {
        fflush(stdout);
        sem_wait(&m);
        visits++;
        sprintf(str,"\nRequisição %d \n", visits);
        sem_post(&m);
        rc = send(sd,str,strlen(str),MSG_NOSIGNAL);
        if (rc == -1){
            printf("\nSocket fechado\n");
            break;
        }
        memset(str, 0, STR_LEN);
        b=recv(sd, str, STR_LEN, 0);
                str[b]=0;
                printf("\nComando recebido:%s",str);

        uppercase(str);

        if (!strncmp(str,"GETR",4)) {
            memset(str, 0, STR_LEN);
            sem_wait(&m);
            sprintf(str,"\nDitado %d: %s ", visits%ditados, msg[visits%ditados]);
            sem_post(&m);
            send(sd,str,STR_LEN,0);
        }
        else if (!strncmp(str,"GETN",4)) {
            memset(str, 0, STR_LEN);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            val = strtol(str, &endptr, 10);
            if (endptr==str){
                sprintf(str,"\nFALHA");
                continue;
                }
            else{
                sem_wait(&m);
                send(sd,msg[val],strlen(msg[val]),0);
                sem_post(&m);
            }
        }
        else if (!strncmp(str,"REPLACE",7)) {
            memset(str, 0, STR_LEN);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            val = strtol(str, &endptr, 10);
            if (endptr==str){
                sprintf(str,"\nFALHA");
                continue;
            }
            send(sd,str,strlen(str),0);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            sem_wait(&m);
            strcpy(msg[val],str);
            alteracoes++;
            sem_post(&m);

            sprintf(str,"\nOK");
            send(sd,str,strlen(str),0);
            //printf("\nNovo ditado %d: %s",val,msg[val]);
            }

        else if (!strncmp(str,"HELP",7)) {
            memset(str, 0, STR_LEN); //Limpa o buffer
            sprintf(str,"\nGETR\nGETN\nREPLACE\nVER\nDEL\nROTATE\nSEARCH\nPALAVRAS-D\nPALAVRAS-T\nALTERACOES\nGRAVA\nLE\nFIM");
            send(sd,str,STR_LEN,0); //envia a mensagem
        }

        else if (!strncmp(str,"DEL",3)) {
            memset(str, 0, STR_LEN);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            val = strtol(str, &endptr, 10);
            if (endptr==str){
                sprintf(str,"\nFALHA");
                continue;
            }
            sem_wait(&m);
            printf("\nRecebido %i", val);
            remove_element(val);
            printf("\nRemovido %i", val);
            alteracoes++;
            printf("\nAlteracao++");
            ditados--;
            printf("\nDitados--");
            sem_post(&m);
            sprintf(str,"\nOK");
            send(sd,str,strlen(str),0);
        }

        else if (!strncmp(str,"ROTATE",6)) {
            int val1, val2;

            memset(str, 0, STR_LEN);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            val1 = strtol(str, &endptr, 10);
            if (endptr==str){
                sprintf(str,"\nFALHA");
                continue;
            }

            memset(str, 0, STR_LEN);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            val2 = strtol(str, &endptr, 10);
            if (endptr==str){
                sprintf(str,"\nFALHA");
                continue;
            }

            sem_wait(&m);
            strcpy(str,msg[val1]);
            strcpy(msg[val1],msg[val2]);
            strcpy(msg[val2],str);
            alteracoes++;
            sem_post(&m);

            sprintf(str,"\nOK");
            send(sd,str,strlen(str),0);
            //printf("\nNovo ditado %d: %s",val,msg[val]);
        }

        else if (!strncmp(str,"SEARCH",6)) {
            memset(str, 0, STR_LEN);
            recv(sd, str, STR_LEN, 0); //str com o termo de busca
            sem_wait(&m);
            busca(sd, str);
            sem_post(&m);
        }

        else if (!strncmp(str,"PALAVRAS-D",10)) {
            int c;
            memset(str, 0, STR_LEN);
            b=recv(sd, str, STR_LEN, 0);
            str[b]=0;
            val = strtol(str, &endptr, 10);
            if (endptr==str){
                sprintf(str,"\nFALHA");
                continue;
            }
            sem_wait(&m);
            strcpy(str, msg[val]);
            c = palavras_d(str);
            sem_post(&m);

            sprintf(str, "%d", c);
            send(sd,str,strlen(str),0);
        }

        else if (!strncmp(str,"PALAVRAS-T",10)) {
            int c=0, i;
            sem_wait(&m);
            for(i = 0; i < ARRAY_LEN - 1; i++){
                memset(str, 0, STR_LEN);
                strcpy(str, msg[i]);
                c += palavras_d(str);
            }
            sem_post(&m);
            sprintf(str, "%d", c);
            send(sd,str,strlen(str),0);
        }

        else if (!strncmp(str,"ALTERACOES",10)) {
            memset(str, 0, STR_LEN); //Limpa o buffer
            sem_wait(&m);
            sprintf(str,"\n%d", alteracoes);
            sem_post(&m);
            send(sd,str,STR_LEN,0); //envia a mensagem
        }

        else if (!strncmp(str,"GRAVA",5)) {
            memset(str, 0, STR_LEN); //Limpa o buffer
            memset(str2, 0, STR_LEN-100); //Limpa o buffer
            recv(sd, str2, STR_LEN-100, 0);
            sem_wait(&m);
            GravaDitado(str2);
            sem_post(&m);
            sprintf(str,"\nMensagem gravada em %s", str2);
            send(sd,str,STR_LEN,0); //envia a mensagem
        }

        else if (!strncmp(str,"LE",2)) {
            memset(str, 0, STR_LEN); //Limpa o buffer
            memset(str2, 0, STR_LEN-100); //Limpa o buffer
            recv(sd, str2, STR_LEN-100, 0);
            sem_wait(&m);
            LeDitado(str2);
            alteracoes = 0;
            sem_post(&m);
            sprintf(str,"\nMensagem lida de %s", str2);
            send(sd,str,STR_LEN,0); //envia a mensagem
        }

        else if (!strncmp(str,"FIM",3)) {
            memset(str, 0, STR_LEN);
            sprintf(str,"\nAté Logo");
            send(sd,str,strlen(str),0);
            break;
            }
        else if (!strncmp(str,"VER",3)) {
            memset(str, 0, STR_LEN);
            sprintf(str,"\nServidor de Ditados 2.0 Beta.\nTE355 2022 Primeiro Trabalho");
            send(sd,str,strlen(str),0);
           }
        else if (!strncmp(str,"\n",1)) {
            memset(str, 0, STR_LEN);
            sprintf(str,"\n");
        }
        else{
            rc = send(sd,str,strlen(str),MSG_NOSIGNAL);
            if(rc == -1){
                break;
            }
            memset(str, 0, STR_LEN);
            sprintf(str,"\nErro de Protocolo");
            send(sd,str,strlen(str),0);
           }
       }
       close(sd);
}

int main(int argc, char **argv)
{
    struct  protoent *ptrp;  /* pointer to a protocol table entry   */
    struct  sockaddr_in sad; /* structure to hold server's address  */
    struct  sockaddr_in cad; /* structure to hold client's address  */
    int     sd, sd2;         /* socket descriptors                  */
    int     port;            /* protocol port number                */
    int     alen;            /* length of address                   */
    pthread_t t;
    sem_init(&m, 0, 1);

    fflush(stdout);srandom(time(NULL)); /* inicializa a semente do gerador de números aleatórios */

    memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET;         /* set family to Internet     */
    sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address   */

    /* Check command-line argument for protocol port and extract    */
    /* port number if one is specified.  Otherwise, use the default */
    /* port value given by constant PROTOPORT                       */

    if (argc > 1) {                 /* if argument specified        */
        port = atoi(argv[1]);   /* convert argument to binary   */
    } else {
        port = PROTOPORT;       /* use default port number      */
    }
    if (port > 0)                   /* test for illegal value       */
        sad.sin_port = htons((u_short)port);
    else {                          /* print error message and exit */
        fprintf(stderr,"bad port number %s\n",argv[1]);
        exit(1);
    }

        LeDitado("Ditados.txt");

    /* Map TCP transport protocol name to protocol number */

    if ( ((ptrp = getprotobyname("tcp"))) == NULL) {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit(1);
    }

    /* Create a socket */

    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "socket creation failed\n");
        exit(1);
    }

    /* Bind a local address to the socket */

    if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        fprintf(stderr,"bind failed\n");
        exit(1);
    }

    /* Specify size of request queue */

    if (listen(sd, QLEN) < 0) {
        fprintf(stderr,"listen failed\n");
        exit(1);
    }

    /* Main server loop - accept and handle requests */

    while (1) {
        alen = sizeof(cad);
        if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
            fprintf(stderr, "accept failed\n");
            exit(1);
        }
        printf("\nServidor atendendo conexão %d", visits);
        pthread_create(&t, NULL,  atendeConexao, &sd2 );
    }
}
