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
    while(ntimes>0){
        if(connect(sock_fd, (const struct sockaddr *)&addr, sizeof(struct sockaddr_un))==0) return sock_fd;
        else{
            printf("Connessione Fallita. Ritento \n");
            --ntimes;
            sleep(secs);
        }

    }
    printf("Tentativi Massimi superati\n");
    return -1;
}

long openDispatcher(char* path){
    struct sockaddr_un addr;
    long sock_fd;
    createAddress(path,&addr);
    SOCKETCHECK(sock_fd=socket(AF_UNIX,SOCK_STREAM,0));
    SOCKETCHECK(bind(sock_fd,(const struct sockaddr*) &addr, sizeof(struct sockaddr_un)));
    SOCKETCHECK(listen(sock_fd,SOMAXCONN));
    return sock_fd;
}

long acceptConnection(long sock_fd, char* path){
    long new_sock=0;
    socklen_t len=sizeof(struct sockaddr_un);
    struct sockaddr_un addr;
    createAddress(path,&addr);
    SOCKETCHECK(new_sock=accept(sock_fd,(struct sockaddr*) &addr,&len));
    return new_sock;
}

int readHeader(long connfd, message_hdr_t *hdr){
    hdr=memset(hdr,0,sizeof(message_hdr_t));
    TRYREAD(read(connfd,hdr,sizeof(message_hdr_t)));
    return 1;
}

int readData(long fd, message_data_t *data){
    int byte_read=0;
    int len=0;
    char* position;
    data=memset(data,0,sizeof(message_data_t));
    TRYREAD(read(fd,&(data->hdr),sizeof(message_data_hdr_t)));
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
            position+=byte_read+1;
        }
    }

    return 1;
}

int readMsg(long fd, message_t *msg){
    int err=0;
    err=readHeader(fd, &(msg->hdr));
    if(err==-1) return -1;
    if(err==0) return 0;
    err=readData(fd, &(msg->data));
    return err;
}

int sendData(long fd, message_data_t *msg){
    int byte_wrote=0;
    int len=0;
    char* position;
    TRYWRITE(write(fd,&(msg->hdr),sizeof(message_data_hdr_t)));
    len=msg->hdr.len;
    position=msg->buf;
    while(len>0){
        TRYWRITE(byte_wrote=write(fd,position,len));
        len-=byte_wrote;
        position+=byte_wrote+1;
    }
    return 1;
}

int sendRequest(long fd, message_t *msg){
    int byte_wrote=0;
    TRYWRITE(write(fd,&(msg->hdr),sizeof(message_hdr_t)));
    byte_wrote=sendData(fd,&(msg->data));
    return byte_wrote;
}




#endif //_connections_c_
