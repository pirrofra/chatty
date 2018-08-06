/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/**
 * @file chatty.c
 * @author Francesco Pirrò 544539
 * @brief File principale del server chatterbox
 * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

/* inserire gli altri include che servono */
#include <sys/select.h>
#include<stats.h>
#include<user.h>
#include<queue.h>
#include<fileconfig.h>
#include<config.h>
#include<requestexec.h>
#include<connections.h>

/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 *
 */
struct statistics chattyStats;
manager usrmngr;
queue coda_client;
configs* configurazione=NULL;
fd_set fdascolto;
int local_max=0;
pthread_mutex_t fd_set_lock=PTHREAD_MUTEX_INITIALIZER;
int max_sock=0;
int sigterm=0;
pthread_t* pool;

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

void cleanup();

void* ThreadMain(void* arg);

int main(int argc, char *argv[]) {
    int dispatcher=0;
    fd_set localset;
    int err=0;
    struct timeval time={0,10000};
    if(argc!=3){
        usage(argv[0]);
        return -1;
    }
    printf("Inizializzo il Server\nLeggo il file di configurazione\n");
    if((configurazione=readConfig(argv[2]))==NULL) return -1;
    printf("File configurazione letto correttamente\nInizializzo il gestore degli utenti\n");
    INITIALIZE(initializeManager(&usrmngr,configurazione->MaxConnections,configurazione->MaxHistMsgs));
    printf("Gestore degli Utenti inizializzato correttamente\nInizializzo Coda delle richieste\n");
    INITIALIZE(initializeQueue(&coda_client,configurazione->MaxConnections));
    printf("Coda inizializzata correttamente\nAzzeramento Statistiche\n");
    INITIALIZE(initializeStats(&chattyStats));
    printf("Statistiche Azzerate\nApertura del Socket\n");
    if((dispatcher=openDispatcher(configurazione->UnixPath))==-1){
        printf("Errore nell'inizializzazione, termino il server\n");
        return -1;

    }
    printf("Socket aperto con successo\n");
    //GESTIONE DEI SEGNALI
    printf("Avvio il Pool di Thread\n");
    pool=malloc(configurazione->ThreadsInPool*sizeof(pthread_t));
    MEMORYCHECK(pool);
    memset(pool,0,configurazione->ThreadsInPool*sizeof(pthread_t));
    for(int i=0;i<configurazione->ThreadsInPool;i++)
        INITIALIZE(pthread_create(&pool[i],NULL,ThreadMain,NULL));
    printf("Pool Thread Avviato\n");
    SYSCALLCHECK(pthread_mutex_lock(&fd_set_lock),"Mutex Lock");
    FD_ZERO(&fdascolto);
    FD_SET(dispatcher, &fdascolto);
    if(dispatcher>max_sock) max_sock=dispatcher;
    SYSCALLCHECK(pthread_mutex_unlock(&fd_set_lock),"Mutex Lock");
    if(atexit(cleanup)){
        printf("Impossibile settare la funzione di uscita\n");
        return -1;
    }
    while(!sigterm){
        SYSCALLCHECK(pthread_mutex_lock(&fd_set_lock),"Mutex Lock");
        localset=fdascolto;
        local_max=max_sock;
        SYSCALLCHECK(pthread_mutex_unlock(&fd_set_lock),"Mutex Lock");
        //select e gestione Coda
        err=select(local_max+1,&localset,NULL,NULL,&time);
        if(err==-1){
            perror("Select");
            return -1;
        }

        for(int i=0;i<=local_max;i++){
            if(FD_ISSET(i,&localset)){
                if(i==dispatcher){
                    int accettato=0;
                    int new_fd=0;
                    SYSCALLCHECK(pthread_mutex_lock(&(chattyStats.lock)),"Mutex Lock");
                    if(chattyStats.nonline<configurazione->MaxConnections) accettato=1;
                    SYSCALLCHECK(pthread_mutex_unlock(&(chattyStats.lock)),"Mutex Lock");

                    if(accettato&&(new_fd=acceptConnection(dispatcher,configurazione->UnixPath))>=0){
                        SYSCALLCHECK(pthread_mutex_lock(&fd_set_lock),"Mutex Lock");
                        FD_SET(new_fd,&fdascolto);
                        if(new_fd>max_sock) max_sock=new_fd;
                        SYSCALLCHECK(pthread_mutex_unlock(&fd_set_lock),"Mutex Lock");
                    }
                }
                else{
                    SYSCALLCHECK(pthread_mutex_lock(&fd_set_lock),"Mutex Lock");
                    FD_CLR(i,&fdascolto);
                    SYSCALLCHECK(pthread_mutex_unlock(&fd_set_lock),"Mutex Lock");
                    if(enqueue(&coda_client,i)==-1) return -1;
                }
            }
        }

    }
    printf("SIGTERM arrivato, Termino il Server\n");
    return 0;
}

void* ThreadMain(void* arg){
    while(!sigterm){
        int currfd=0;
        int status=0;
        if(dequeue(&coda_client,&currfd)==-1) return (void*) -1;
        status=execute(currfd,&usrmngr,configurazione,&chattyStats);
        if(status==-1) close(currfd);
        else{
            if((errno=pthread_mutex_lock(&fd_set_lock))) perror("Mutex Lock");
            FD_SET(currfd,&fdascolto);
            if((errno=pthread_mutex_unlock(&fd_set_lock))) perror("Mutex Lock");
        }
    }
    return 0;
}

void cleanup(){
    sigterm=1;
    printf("termino i Thread");
    for(int i=0;i<configurazione->ThreadsInPool;i++)
        if((errno=(pthread_join(pool[i],NULL)))) perror("Errore nel pthread_join");

    printf("Chiudo i Socket\n");
    for(int i=0;i<max_sock;i++) close(i);
    printf("Socket Chiusi\n");

    printf("Libero la memoria\n");
    destroy(&usrmngr);
    destroystats(&chattyStats);
    freeQueue(&coda_client);
    freeC(configurazione);
    printf("Memoria Terminata\n");
}
