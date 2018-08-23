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
#include<arrayLock.h>

/* struttura che memorizza le statistiche del server, struct statistics
 * e' definita in stats.h.
 *
 */
struct statistics chattyStats; //statistiche server
manager usrmngr; //user manager
queue codaClient; //coda dei client in attesa
configs* configurazione=NULL; //configurazione dei server
arrayLock writingLock; //array di Mutex lock per le scritture sui socket

fd_set fdAscolto;
pthread_mutex_t fdSetLock=PTHREAD_MUTEX_INITIALIZER;
int maxSock=0; //fd più alto
//fd_set e mutex lock per la select


int sigterm=0; //variabile per avvissare l'arrivo di un segnale di terminazione
int sigUsr1Arrived=0; //variabile per avvissare l'arrivo di un segale SIGUSR1


pthread_t* pool; //id dei thread nel pool

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

/**
 *@function setSigTerm
 *@brief procedura handler per il segnale di terminazione
 *@param signum segnale catturato
*/
void setSigTerm(int signum);

/**
 *@function setSigUsr1Arrived
 *@brief procedura handler per il segnale USR1
 *@param signum segnale catturato
*/
void setSigUsr1Arrived(int signum);

/**
 *@function printFileStat
 *@brief procedura che stampa le statistiche del server su file
*/
void printFileStat();

/**
 *@function setSignalHandler
 *@brief funzione che imposta gli handler dei segnali
 *@return 0 successo, -1 in caso di errore
*/
int setSignalHandler();

/**
 * @function cleanup
 *@brief funzione che alla chiusura libera la memoria allocata, chiude i thread e i socket
*/
void cleanup();

/**
 *@funcion ThreadMain
 *@brief main dei thread Worker
 *@param arg argomento del thread
 *@return valore di ritorno del thread
*/
void* ThreadMain(void* arg);

int main(int argc, char *argv[]) {
    int dispatcher=0; //dispatcher su cui fare accept
    fd_set localSet;
    int localMax=0;
    //local set e localMax per la select
    int err=0; //valore di ritorno della select

    struct timeval timeout={0,1000};
    if(argc!=3){ //programma lanciato con un numero incorretto di argomenti
        usage(argv[0]);
        return -1;
    }
    printf("Inizializzazione Server\n");
    if((configurazione=readConfig(argv[2]))==NULL) return -1; //legge il file di Configurazione
    if((unlink(configurazione->UnixPath)==-1 && errno!=ENOENT)){ //se esiste già UnixPath lo elimina
        perror("Pulizia UnixPath");
        return -1;
    }

    mymkdir(configurazione->UnixPath);
    mymkdir(configurazione->StatFileName);
    mymkdir(configurazione->DirName);
    //si assicura che i path in UnixPath, StatFileName e DirName esistano

    INITIALIZE(initializeManager(&usrmngr,configurazione->MaxConnections,configurazione->MaxHistMsgs,configurazione->ThreadsInPool));
    INITIALIZE(initializeQueue(&codaClient,configurazione->MaxConnections));
    INITIALIZE(initializeStats(&chattyStats));
    INITIALIZE(initializeArrayLock(&writingLock, configurazione->ThreadsInPool));
    //Inizializza User Manager, Coda, Statistiche e ArrayLock per le scritture sui socket

    if((dispatcher=openDispatcher(configurazione->UnixPath))==-1){ //apre dispatcher
        printf("Errore nell'inizializzazione, termino il server\n");
        return -1;

    }
    INITIALIZE(setSignalHandler()); //setta i nuovi handler per i segnali

    pool=malloc(configurazione->ThreadsInPool*sizeof(pthread_t));
    MEMORYCHECK(pool);
    memset(pool,0,configurazione->ThreadsInPool*sizeof(pthread_t));
    for(int i=0;i<configurazione->ThreadsInPool;i++){
        int* threadnum=malloc(sizeof(int)); //thread id locale (da 1 a ThreadsInPool)
        MEMORYCHECK(threadnum);
        *threadnum=i+1;
        INITIALIZE(pthread_create(&pool[i],NULL,ThreadMain,threadnum));
    }
    //spawn dei thread del pool. Essendo la coda vuota, si metteranno subito in attesa di enqueue

    SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
    FD_ZERO(&fdAscolto);
    FD_SET(dispatcher, &fdAscolto);
    if(dispatcher>maxSock) maxSock=dispatcher;
    SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
    //inserisce il dispatcher nel fd_set

    if(atexit(cleanup)){
        printf("Impossibile settare la funzione di uscita\n");
        return -1;
    }
    //imposta che la funzione cleanup venga eseguita automanticamente all'uscita

    printf("Server Inizializzato\nIn attesa di richieste...\n");
    while(!sigterm){

        if(sigUsr1Arrived) printFileStat(); //se è stato catturato un segnale USR1 stampa il file delle statistiche

        SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
        localSet=fdAscolto;
        localMax=maxSock;
        SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
        //salva il set globale in un set locale

        err=select(localMax+1,&localSet,NULL,NULL,&timeout);

        if(err==-1&&(errno!=EINTR)){ //errore nella select che non è una interruzione a causa di Segnale
            perror("Select");
            return -1;
        }
        else if(err>0){ //err>0 vuol dire che la select ha trovato almeno un socket che non si blocca
            for(int i=3;i<=localMax;i++){ //controlla tutti i socket

                if(FD_ISSET(i,&localSet)){
                    if(i==dispatcher){ //il dispatcher non si blocca, nuova connessione in attesa
                        int accettato=0;
                        int new_fd=0;
                        SYSCALLCHECK(pthread_mutex_lock(&(chattyStats.lock)),"Mutex Lock");
                        if(chattyStats.nonline<configurazione->MaxConnections) accettato=1;
                        SYSCALLCHECK(pthread_mutex_unlock(&(chattyStats.lock)),"Mutex Lock");
                        //controlla il numero di utenti online e se puo' accettarne di nuovi

                        if(accettato&&(new_fd=acceptConnection(dispatcher,configurazione->UnixPath))>=0){
                            SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
                            FD_SET(new_fd,&fdAscolto);
                            if(new_fd>maxSock) maxSock=new_fd;
                            SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
                            printf("Nuova Connessione Accettata\n");
                            //nuova connessione accettata, e inserita nel fd_set globale
                        }
                        //se non è stata accettata resta in attesa che si disconnetti qualcuno

                    }
                    else{

                        SYSCALLCHECK(pthread_mutex_lock(&fdSetLock),"Mutex Lock");
                        FD_CLR(i,&fdAscolto);
                        SYSCALLCHECK(pthread_mutex_unlock(&fdSetLock),"Mutex Lock");
                        if(enqueue(&codaClient,i)==-1) return -1;
                        printf("Nuova Richiesta sul socket %d\n",i);
                        //c'è una richiesta da un client, mette nella coda ed elimina dal fd_set globale
                    }
                }
            }
        }

    }
    printf("Server in chiusura\n");
    return 0;
    //uscito dal while perchè sigterm=1
}

/**
 *@funcion ThreadMain
 *@brief main dei thread Worker
 *@param arg argomento del thread
 *@return valore di ritorno del thread
*/
void* ThreadMain(void* arg){
    int n=*(int*)arg; //thread id locale
    printf("Worker %d avviato\n",n);
    while(!sigterm){
        int currfd=0;
        int status=0;
        if(dequeue(&codaClient,&currfd)!=-1){
            printf("[Worker %d]: Comunicazione sul socket %d iniziata\n",n,currfd);
            status=execute(currfd,&usrmngr,configurazione,&chattyStats,&writingLock);
            //prende un fd dalla coda ed esegue un'operazione

            if(status==-1) {
                close(currfd);
                printf("[Worker %d]: Client sul socket %d disconnesso\n",n,currfd);
                //connessione persa, chiude il socket
            }
            else{
                printf("[Worker %d]: Richiesta sul socket %d eseguita\n",n,currfd);
                if((errno=pthread_mutex_lock(&fdSetLock))) perror("Mutex Lock");
                FD_SET(currfd,&fdAscolto);
                if((errno=pthread_mutex_unlock(&fdSetLock))) perror("Mutex Lock");
                //reinserisce il fd nel fd_set in attesa di altre richieste

            }
        }
    }
    free(arg); //libera l'argomento prima di terminare il thread
    return 0;
}

/**
 * @function cleanup
 *@brief funzione che alla chiusura libera la memoria allocata, chiude i thread e i socket
*/
void cleanup(){
    sigterm=1;

    forceWakeEveryone(&codaClient);
    //sveglia eventuali thread in attesa

    for(int i=0;i<configurazione->ThreadsInPool;i++)
        if((errno=(pthread_join(pool[i],NULL)))) perror("Errore nel pthread_join"); //join dei thread

    for(int i=3;i<=maxSock;i++) close(i);
    //chiusura di eventuali socket aperti.

    freeArrayLock(&writingLock);
    destroy(&usrmngr);
    destroystats(&chattyStats);
    freeQueue(&codaClient);
    freeC(configurazione);
    free(pool);
    //libera memoria allocata

    printf("Server Chiuso\n" );
}

/**
 *@function setSignalHandler
 *@brief funzione che imposta gli handler dei segnali
 *@return 0 successo, -1 in caso di errore
*/
int setSignalHandler(){
    struct sigaction sigPipe;
    struct sigaction termina;
    struct sigaction stampastats;
    memset(&sigPipe,0,sizeof(sigPipe));
    memset(&termina,0,sizeof(termina));
    memset(&stampastats,0,sizeof(stampastats));

    sigPipe.sa_handler=SIG_IGN;
    SIG_ACTION(SIGPIPE, sigPipe);
    //sigpipe viene ignorato

    termina.sa_handler=setSigTerm;
    SIG_ACTION(SIGINT, termina);
    SIG_ACTION(SIGTERM, termina);
    SIG_ACTION(SIGQUIT, termina);
    //sigint, sigterm e sigquit eseguono la funzione setSigTerm

    stampastats.sa_handler=setSigUsr1Arrived;
    SIG_ACTION(SIGUSR1, stampastats);
    //sigusr1 esegue setSigUsr1Arrived

    return 0;
}

/**
 *@function setSigUsr1Arrived
 *@brief procedura handler per il segnale USR1
 *@param signum segnale catturato
*/
void setSigUsr1Arrived(int signum){
    sigUsr1Arrived=1;
}

/**
 *@function setSigTerm
 *@brief procedura handler per il segnale di terminazione
 *@param signum segnale catturato
*/
void setSigTerm(int signum){
    sigterm=1;
}

/**
 *@function printFileStat
 *@brief procedura che stampa le statistiche del server su file
*/
void printFileStat(){
    printf("Ricevuta Richiesta di Stampa delle Statistiche\n");
    FILE* statFile;
    if((statFile=fopen(configurazione->StatFileName,"w+"))!=NULL){ //apre file statistiche
        LOCKACQUIRE(chattyStats.lock);
        if(printStats(statFile)==-1)printf("Errore Stampa File Statistiche");
        LOCKRELEASE(chattyStats.lock);
        //Stampa eseguita in muta esculsione
        fclose(statFile); //chiusura file
        printf("Statistiche stampate con successo\n");
    }
    else perror("Apertura File Statistiche");
    sigUsr1Arrived=0; //reset sigUsr1Arrived
}
