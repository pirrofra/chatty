/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file connection.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#include <connections.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>


#ifndef _connections_c_
#define _connections_c_

void createAddress(char* path, struct sockaddr_un* addr){
    addr->sun_family=AF_UNIX;
    strncpy(addr->sun_path, path, sizeof(addr->sun_path)-1);
}

/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server
 *
 * @param path Path del socket AF_UNIX
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs){
    struct sockaddr_un addr;
    int sock_fd;

    if(ntimes>MAX_RETRIES){
       printf("Numero di tentativi troppo alto");
       return -1;
    }
    else if(secs>MAX_SLEEPING){
        printf("Tempo di Attesa troppo alto");
        return -1;
    }

    SOCKETCHECK(sock_fd=socket(AF_UNIX,SOCK_STREAM,0));
    createAddress(path, &addr);
    //crea Socket e indirizzo

    while(ntimes>0){
        if(connect(sock_fd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un))==0) return sock_fd;
        else{
            printf("Connessione Fallita. Ritento \n");
            --ntimes;
            sleep(secs);
        }
        //se connect ha successo ritorna sock_fd, altrimenti aspetta secs secondi

    }
    printf("Tentativi Massimi superati\n");
    return -1;
}

/**
 * @function openDispatcher
 * @brief contiene le funzioni che implementano la creazione del dispatcher per l'accettazione delle connessioni dei clients
 * @param path   indirizzo del socket
 * @return il descrittore del dispatcher se ha successo, -1 se fallisce
 */
long openDispatcher(char* path){
    struct sockaddr_un addr;
    long sock_fd;
    createAddress(path,&addr);
    SOCKETCHECK(sock_fd=socket(AF_UNIX,SOCK_STREAM,0));
    SOCKETCHECK(bind(sock_fd,(const struct sockaddr*) &addr, sizeof(struct sockaddr_un)));
    SOCKETCHECK(listen(sock_fd,SOMAXCONN));
    return sock_fd;
    //crea socket, esegue bind sull'indirizzo e lo prepara ad accettare connessioni con listen
}

/**
 * @function acceptConnection
 * @brief contiene funzioni che implementano l'accettazione delle connessioni dai client. Se non ci sono connessioni in attesa, si blocca in attesa di una nuova richiesta di connessione.
 * @param sock_fd    descrittore del dispatcher
 * @param path       indirizzo del socket
 * @return il descrittore associato alla nuova connessione, -1 errore
 */
long acceptConnection(long sock_fd, char* path){
    long new_sock=0;
    socklen_t len=sizeof(struct sockaddr_un);
    struct sockaddr_un addr;
    createAddress(path,&addr);
    //crea indirizzo
    SOCKETCHECK(new_sock=accept(sock_fd,(struct sockaddr*) &addr,&len));
    return new_sock;
    //ritorna il nuovo socket accettato
}

/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param fd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readHeader(long connfd, message_hdr_t *hdr){
    hdr=memset(hdr,0,sizeof(message_hdr_t));
    TRYREAD(read(connfd,hdr,sizeof(message_hdr_t)));
    return 1;
}

/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readData(long fd, message_data_t *data){
    int byte_read=0;
    int len=0;
    char* position;
    data=memset(data,0,sizeof(message_data_t));
    TRYREAD(read(fd,&(data->hdr),sizeof(message_data_hdr_t)));
    //legge header dei data

    len=data->hdr.len;
    if(len==0) data->buf=NULL;
    else {
        data->buf=malloc(len*sizeof(char));
        MEMORYCHECK(data->buf);
        memset(data->buf,'\0',len*sizeof(char));
        position=data->buf;
        while(len>0){
            byte_read=read(fd,position, len*sizeof(char));
            if(byte_read<0) {
                perror("Read Buffer");
                free(data->buf);
                return -1;
            }
            if(byte_read==0){
                free(data->buf);
                return 0;
            }
            len-=byte_read;
            position+=byte_read;
        }
        //legge il buffer del messaggio in un while, in modo che sia sicuro di aver letto l'intero buffer.
    }

    return 1;
}

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa)
 */
int readMsg(long fd, message_t *msg){
    int err=0;
    err=readHeader(fd, &(msg->hdr)); //legge Header
    if(err==-1) return -1;
    if(err==0) return 0;
    err=readData(fd, &(msg->data)); //legge Data
    return err;
}

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg){
    int byte_wrote=0;
    int len=0;
    char* position;
    TRYWRITE(write(fd,&(msg->hdr),sizeof(message_data_hdr_t)));
    //scrive header di data
    len=msg->hdr.len;
    position=msg->buf;
    while(len>0){
        TRYWRITE(byte_wrote=write(fd,position,len));
        len-=byte_wrote;
        position+=byte_wrote;
    }
    //scrive il buffer in un while, per essere sicuro di averlo spedito tutto.
    return 1;
}

/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg){
    int byte_wrote=0;
    TRYWRITE(write(fd,&(msg->hdr),sizeof(message_hdr_t))); //spedisce header
    byte_wrote=sendData(fd,&(msg->data)); //spedisce Data
    return byte_wrote;
}

/**
 * @function sendHeader
 * @brief Invia l'header del messaggio
 * @param fd descrittore della connessione
 * @param hdr puntatore header da inviare
 * @return <=0 se c'è stato un errore
*/
int sendHeader(long fd, message_hdr_t* hdr){
    TRYWRITE(write(fd,hdr,sizeof(message_hdr_t))); //spedisce header
    return 1;
}



#endif //_connections_c_
