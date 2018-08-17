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
queue codaClient;
configs* configurazione=NULL;
fd_set fdAscolto;
pthread_mutex_t fdSetLock=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t writingLock=PTHREAD_MUTEX_INITIALIZER;

int maxSock=0;
int sigterm=0;
int sigUsr1Arrived=0;
pthread_t* pool;

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

void setSigTerm(int signum);

void setSigUsr1Arrived(int signum);

void printFileStat();

int setSignalHandler();

void cleanup();

void* ThreadMain(void* arg);

int main(int argc, char *argv[]) {
    int dispatcher=0;
    fd_set localSet;
    int localMax=0;
    int err=0;
    struct timeval timeout={0,1000};
    if(argc!=3){
        usage(argv[0]);
        return -1;
    }
    printf("Inizializzazione Server\n");
    if((configurazione=readConfig(argv[2]))==NULL) return -1;
    INITIALIZE(initializeManager(&usrmngr,configurazione->MaxConnections,configurazione->MaxHistMsgs));
    INITIALIZE(initializeQueue(&codaClient,configurazione->MaxConnections));
    INITIALIZE(initializeStats(&chattyStats));
    if((dispatcher=openDispatcher(configurazione->UnixPath))==-1){
        printf("Errore nell'inizializzazione, termino il server\n");
        return -1;

    }
    INITIALIZE(setSignalHandler());
    pool=malloc(configurazione->ThreadsInPool*sizeof(pthread_t));
    MEMORYCHECK(pool);
    memset(pool,0,configurazione->ThreadsInPool*sizeof(pthread_t));
    for(int i=0;i<configurazione->ThreadsInPool;i++){
        int* threadnum=malloc(sizeof(int));
        MEMORYCHECK(threadnum);
        *threadnum=i+1;
        INITIALIZE(pthread_create(&pool[i],NULL,ThreadMain,threadnum));
    }
    SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
    FD_ZERO(&fdAscolto);
    FD_SET(dispatcher, &fdAscolto);
    if(dispatcher>maxSock) maxSock=dispatcher;
    SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
    if(atexit(cleanup)){
        printf("Impossibile settare la funzione di uscita\n");
        return -1;
    }
    printf("Server Inizializzato\nIn attesa di richieste...\n");
    while(!sigterm){

        if(sigUsr1Arrived) printFileStat();

        SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
        localSet=fdAscolto;
        localMax=maxSock;
        SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
        //select e gestione Coda
        err=select(localMax+1,&localSet,NULL,NULL,&timeout);
        if(err==-1&&(errno!=EINTR)){
            perror("Select");
            return -1;
        }
        else if(err>0){
            for(int i=3;i<=localMax;i++){
                if(FD_ISSET(i,&localSet)){
                    if(i==dispatcher){
                        int accettato=0;
                        int new_fd=0;
                        SYSCALLCHECK(pthread_mutex_lock(&(chattyStats.lock)),"Mutex Lock");
                        if(chattyStats.nonline<configurazione->MaxConnections) accettato=1;
                        SYSCALLCHECK(pthread_mutex_unlock(&(chattyStats.lock)),"Mutex Lock");
                        if(accettato&&(new_fd=acceptConnection(dispatcher,configurazione->UnixPath))>=0){
                            SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
                            FD_SET(new_fd,&fdAscolto);
                            if(new_fd>maxSock) maxSock=new_fd;
                            SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
                            printf("Nuova Connessione Accettata\n");
                        }
                    }
                    else{
                        SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
                        FD_CLR(i,&fdAscolto);
                        SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
                        if(enqueue(&codaClient,i)==-1) return -1;
                        printf("Nuova Richiesta sul socket %d\n",i);
                    }
                }
            }
        }

    }
    printf("Server in chiusura\n");
    return 0;
}

void* ThreadMain(void* arg){
    int n=*(int*)arg;
    printf("Worker %d avviato\n",n);
    while(!sigterm){
        int currfd=0;
        int status=0;
        if(dequeue(&codaClient,&currfd)!=-1){
            printf("[Worker %d]: Comunicazione sul socket %d iniziata\n",n,currfd);
            status=execute(currfd,&usrmngr,configurazione,&chattyStats,&writingLock);
            if(status==-1) {
                close(currfd);
                printf("[Worker %d]: Client sul socket %d disconnesso\n",n,currfd);
            }
            else{
                printf("[Worker %d]: Richiesta sul socket %d eseguita\n",n,currfd);
                if((errno=pthread_mutex_lock(&fdSetLock))) perror("Mutex Lock");
                FD_SET(currfd,&fdAscolto);
                if((errno=pthread_mutex_unlock(&fdSetLock))) perror("Mutex Lock");

            }
        }
    }
    free(arg);
    return 0;
}

void cleanup(){
    sigterm=1;
    if((errno=pthread_cond_broadcast(&codaClient.empty))) perror("BroadCast Variabile Condizionamento");
    if((errno=pthread_cond_broadcast(&codaClient.full))) perror("BroadCast Variabile Condizionamento");
    for(int i=0;i<configurazione->ThreadsInPool;i++)
        if((errno=(pthread_join(pool[i],NULL)))) perror("Errore nel pthread_join");

    for(int i=3;i<=maxSock;i++) close(i);

    destroy(&usrmngr);
    destroystats(&chattyStats);
    freeQueue(&codaClient);
    freeC(configurazione);
    free(pool);
    printf("Server Chiuso\n" );
}

int setSignalHandler(){
    struct sigaction sigPipe;
    struct sigaction termina;
    struct sigaction stampastats;
    memset(&sigPipe,0,sizeof(sigPipe));
    memset(&termina,0,sizeof(termina));
    memset(&stampastats,0,sizeof(stampastats));
    sigPipe.sa_handler=SIG_IGN;
    SIG_ACTION(SIGPIPE, sigPipe);
    termina.sa_handler=setSigTerm;
    SIG_ACTION(SIGINT, termina);
    SIG_ACTION(SIGTERM, termina);
    SIG_ACTION(SIGQUIT, termina);
    stampastats.sa_handler=setSigUsr1Arrived;
    SIG_ACTION(SIGUSR1, stampastats);
    return 0;
}

void setSigUsr1Arrived(int signum){
    sigUsr1Arrived=1;
}

void setSigTerm(int signum){
    sigterm=1;
}

void printFileStat(){
    printf("Ricevuta Richiesta di Stampa delle Statistiche\n");
    FILE* statFile;
    if((statFile=fopen(configurazione->StatFileName,"w+"))!=NULL){
        MUTEXLOCK(chattyStats.lock);
        if(printStats(statFile)==-1)printf("Errore Stampa File Statistiche");
        MUTEXUNLOCK(chattyStats.lock);
        fclose(statFile);
        printf("Statistiche stampate con successo\n");
    }
    else perror("Apertura File Statistiche");
    sigUsr1Arrived=0;
}
