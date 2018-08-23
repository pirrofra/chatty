/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file requestexe.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#include<requestexec.h>
#include<connections.h>
#include<stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include<arrayLock.h>

/**
 * @function register_op
 * @brief funzione che registra un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int register_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    stringlist* users;
    op_t result=OP_FAIL;
    result=registerUser(usrmngr,msg.hdr.sender,fd);
    //registro l'utente

    setHeader(&(reply.hdr),result,"");
    if(result==OP_OK){
        users=connectedUserList(usrmngr); //genera lista utenti collegati
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->lenght);
        updusers(chattystats,1);
        updonline(chattystats,1);
        //operazione con esito positivo, aggiorna statistiche e setta il messaggio di risposta
    }
    else{
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
        //operazione con esito negativo, aggiorna statistiche e setta il messaggio di risposta
    }

    MUTEXARRAYLOCK((*writingLock), fd);
    if(sendRequest(fd,&reply)<=0) errConnection=-1; //se la trasmissione fallisce, setta errConnection
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //manda l'esito dell'operazione al client
    if(result==OP_OK) freeStringList(users);
    return errConnection; //stato della connessione
}

/**
 * @function connect_op
 * @brief funzione che connette un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int connect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    stringlist* users=NULL;
    op_t result=OP_FAIL;
    result=connectUser(usrmngr,msg.hdr.sender,fd);
    //segna l'utente come connesso sul file descriptor fd

    setHeader(&(reply.hdr),result,"");
    if(result==OP_OK)users=connectedUserList(usrmngr);
    if(users){
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->lenght);
        updonline(chattystats,1);
        //esito positivo, aggiorna statistche e setta il messaggio di risposta
    }
    else {
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
        //esito negativo, aggiorna statistiche e setta il messaggio di risposta
    }
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendRequest(fd,&reply)<=0) errConnection=-1; //se la trasmissione fallisce, setta errConnection
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //manda l'esito dell'operazione al client
    if(result==OP_OK) freeStringList(users);
    return errConnection; //stato connessione
}

/**
 * @function posttxt_op
 * @brief funzione che notifica l'arrivo di un messaggio ad un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return op_t estio dell'operazione
*/
op_t notifymex(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    op_t result=OP_FAIL;
    int fd_receiver;
    stringlist* users;
    message_t* notify;
    if (msg.hdr.op==TXT_MESSAGE && msg.data.hdr.len>configurazione->MaxMsgSize) result=OP_MSG_TOOLONG; //messaggio testuale troppo lungo
    else if(msg.hdr.op==FILE_MESSAGE && msg.data.hdr.len>configurazione->MaxFileSize) result= OP_MSG_TOOLONG; //file troppo grande
    else if(!groupexist(usrmngr,msg.data.hdr.receiver)){ //il destinatario non è un grupppo
        notify=copymex(msg);
        fd_receiver=storeMessage(usrmngr,msg.data.hdr.receiver,notify);
        //conserva il messaggio nella history del distinatario
        if(fd_receiver==-3) { //il destinatario non esiste
            result=OP_NICK_UNKNOWN;
            free((notify->data).buf);
            free(notify);
        }
        else if(fd_receiver==-2) { //storeMessage ha avuto un fallimento generico
            result=OP_FAIL;
            free((notify->data).buf);
            free(notify);
        }
        else if(fd_receiver>-1){ //il destinatario è attualmente connesso, storeMessage ha restituito il fd
            int success=0;
            MUTEXARRAYLOCK((*writingLock),fd_receiver);
            if((success=sendRequest(fd_receiver,&msg))<=0) printf("Errore Comunicazione");
            MUTEXARRAYUNLOCK((*writingLock),fd_receiver);
            //invia il messaggio al destinatario

            if(msg.hdr.op==TXT_MESSAGE && success >0) updelivered(chattystats,1);
            else if(msg.hdr.op==TXT_MESSAGE && success <=0) updndelivered(chattystats,1);
            //aggiorna le statistiche in base all'esito della trasmissione
            result=OP_OK;
        }
        else if(msg.hdr.op==TXT_MESSAGE && fd_receiver==-1) { //destinatario offline
            updndelivered(chattystats,1);
            result=OP_OK;

        }

        if(msg.hdr.op==FILE_MESSAGE) {
            updnfile(chattystats,1);
            result=OP_OK;
        }
        //aggiorna le statistiche corrette se il messaggio spedito è un identificatore di file
    }
    else if(isingroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver)){
        //il mittente fa parte del gruppo destinatario
        char* tmp;
        users=userGroupList(usrmngr,msg.data.hdr.receiver);
        //ottiene la lista degli utenti registrati al gruppo
        tmp=users->str;
        for(int i=0;i<users->lenght;i++){ //itera su ogni utente registrato al gruppo
                notify=copymex(msg);
                fd_receiver=storeMessage(usrmngr,tmp,notify);
                //salva il messaggio nella history
                if(msg.hdr.op==FILE_MESSAGE)updnfile(chattystats,1);
                if(fd_receiver==-1 && msg.hdr.op==TXT_MESSAGE) updndelivered(chattystats,1);
                else if(fd_receiver>-1){ //destinatario connesso
                    int success=0;
                    MUTEXARRAYLOCK((*writingLock),fd_receiver);
                    if((success=sendRequest(fd_receiver,&msg)<=0)) printf("Errore Comunicazione");
                    MUTEXARRAYUNLOCK((*writingLock),fd_receiver);
                    //trasmette il messaggio
                    if(msg.hdr.op==TXT_MESSAGE&&success>0)updelivered(chattystats,1);
                    else if(msg.hdr.op==TXT_MESSAGE&&success<=0)updndelivered(chattystats,1);
                    //aggiorna le statistiche in base all'esito della trasmissione
                }
                else if(fd_receiver==-2){ //fallimento generico
                    free((notify->data).buf);
                    free(notify);
                }

            tmp+=(users->str_dim+1); //fa puntare tmp al prossimo utente
        }
        freeStringList(users);
        result=OP_OK;
        //esito positivo
    }
    else  //il destinatario è un gruppo, ma il mittente non ne fa parte
        result=OP_NICK_UNKNOWN;


   if(result!=OP_OK){
       upderrors(chattystats,1); //aggiorna statistiche in caso di errore
   }
    return result;
}

/**
 * @function posttxt_op
 * @brief funzione che manda un messaggio ad un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int posttxt_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    op_t result=OP_FAIL;
    msg.hdr.op=TXT_MESSAGE;
    //modifica l'header del messaggio prima di notificarlo
    result=notifymex(fd,msg,usrmngr,configurazione,chattystats,writingLock); //notifica del messaggio
    setHeader(&(reply.hdr),result,"");
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendHeader(fd,&reply.hdr)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //manda l'estito dell'operazione al client
    return errConnection; //stato connessione
}

/**
 * @function posttextall_op
 * @brief funzione che manda un messaggio a tutti gli utenti
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int posttextall_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0;  //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    stringlist* users;
    op_t result=OP_FAIL;
    char* tmp;
    message_t* notify=NULL;
    message_t reply;
    int fd_receiver;
    msg.hdr.op=TXT_MESSAGE;
    //modifica l'header del messaggio prima di notificarlo
    if (msg.data.hdr.len>configurazione->MaxMsgSize) {
        result=OP_MSG_TOOLONG;
        upderrors(chattystats, 1);
        //controlla preventivamente se il messaggio è troppo lungo
    }
    else{
        int numRegUsers=0;
        numRegUsers=getnusers(chattystats); //numero di utenti registrati
        users=registredUserList(usrmngr,numRegUsers);
        //ottiene la lista degli utenti registrati
        tmp=users->str;
        for(int i=0;i<users->lenght;i++){ //itera su tutti gli utenti registrati
            if(strcmp(msg.hdr.sender,tmp)){
                notify=copymex(msg);
                fd_receiver=storeMessage(usrmngr,tmp,notify);
                //conserva il messaggio nella history
                if(fd_receiver==-1) updndelivered(chattystats,1); //utente  disconnesso
                else if(fd_receiver>-1){ //utente connesso
                    int success=0;
                    MUTEXARRAYLOCK((*writingLock),fd_receiver);
                    if(sendRequest(fd_receiver,&msg)<=0) success=1;
                    MUTEXARRAYUNLOCK((*writingLock),fd_receiver);
                    //trasmette il messaggio all'utente
                    if(success==0) updelivered(chattystats,1); //se la trasmissione ha avuto esito positivo, aggiorna statistiche
                    else updndelivered(chattystats,1); //esito negativo, aggiorna statistiche
                }
                else{
                    free((notify->data).buf);
                    free(notify);
                    //messaggio non conservato. Libero la memoria
                }
            }
            tmp+=(users->str_dim+1); //fa puntare tmp al prossimo utente
        }
        freeStringList(users);
        result=OP_OK;
    }
    setHeader(&(reply.hdr),result,"");
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendHeader(fd,&reply.hdr)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione
    return errConnection; //stato della connessione
}

/**
 * @function mymkdir
 * @brief mkdir ricorsiva che si assicura che il path esista
 * @param path path del file da creare
*/
void mymkdir(char*path){
    int lastSlash=0; //posizione ultimo slash in path
    int nChar=0; //posizione corrente
    char* dirPath; //path con le sole directory

    int firstSlash=0;
    struct stat dirStat;

    for(char* p=path;*p!='\0';p++){
        if(*p == '/') lastSlash=nChar;
        ++nChar;
    }
    //segna la posizione dell'ultimo slash

    dirPath=malloc((lastSlash+1)*sizeof(char));
    MEMORYCHECK(dirPath);
    memset(dirPath,'\0',(lastSlash+1)*sizeof(char));
    strncpy(dirPath,path,lastSlash);
    //copia path fino all'ultimo slash in dirPath

    if(stat(dirPath,&dirStat)!=0){ //se il path esiste già è inutile proseguire

        for(char*p=dirPath;*p!='\0';p++){
            if(*p == '/'){
                if(firstSlash==0) ++firstSlash; //se è il primo slash del path non eseguo mkdir
                else{
                    *p='\0';
                    if(mkdir(dirPath,S_IRWXU)<0 && errno!=EEXIST) perror("Creazione directory");
                    *p='/';
                }
            }
        }
        //mkdir ricorsivo
    }
    if(mkdir(dirPath,S_IRWXU)==-1 && errno!=EEXIST) perror("Creazione directory"); //mkdir sul path completo.

    free(dirPath);//libero la stringa

}

/**
 * @function postfile_op
 * @brief funzione che invia un file all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    op_t result=OP_FAIL;
    int byte_wrote=0;
    message_t reply;
    message_data_t file;
    int newFile;
    readData(fd,&file);
    //legge dal socket il contenuto dal file e lo salva in file
    msg.hdr.op=FILE_MESSAGE;
    if(file.hdr.len>(configurazione->MaxFileSize)){
        result=OP_MSG_TOOLONG;
        upderrors(chattystats,1);
        //file troppo grande, aggiorna le statistiche
    }
    else{
        int dim=strlen(configurazione->DirName)+msg.data.hdr.len;
        char* completepath;
        completepath=malloc(sizeof(char)*(dim+1));
        MEMORYCHECK(completepath);
        memset(completepath,'\0',(dim+1)*sizeof(char));
        strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
        strncat(completepath,msg.data.buf,msg.data.hdr.len);
        //genera il path completo in cui salvare il file

        mymkdir(completepath); //si assicura che il path esista
        if((newFile=open(completepath,O_WRONLY| O_CREAT,00700))==-1) //apertura del file
            perror("Apertura del File");

        else{
            char* position=file.buf;
            int len=file.hdr.len;
            while(len>0){
                byte_wrote=write(newFile,file.buf, len);
                if(byte_wrote<0){
                    len=0;
                    perror("scrittura file");
                }
                len-=byte_wrote;
                position+=byte_wrote;

            }
            //scrive il contenuto nel file, con un while per essere sicuro di scrivere tutto
            if(byte_wrote<0){
                result=OP_FAIL;
                upderrors(chattystats,1);
                //write fallita
            }

            else result=notifymex(fd,msg,usrmngr,configurazione,chattystats,writingLock);
            //notifica il destinatario dell'esistenza di un file da scaricare, restituisce l'esito dell'operazione
            close(newFile);
        }
        free(completepath);
    }
    free(file.buf);// libera il buffer letto

    setHeader(&(reply.hdr),result,"");

    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendHeader(fd,&reply.hdr)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione

    return errConnection; //stato connessione
}

/**
 * @function getfile_op
 * @brief funzione che recupera un file inviato all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    int fileToMap; //fd su cui aprire il  file da spedire
    struct stat s_file;
    op_t result=OP_FAIL;
    message_t reply;
    int dim=strlen(configurazione->DirName)+(msg.data.hdr.len);
    char* fileData=NULL;
    char* completepath;

    completepath=malloc(sizeof(char)*(dim+1));
    MEMORYCHECK(completepath);
    memset(completepath,'\0',(dim+1)*sizeof(char));
    strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
    strncat(completepath,msg.data.buf,msg.data.hdr.len);
    //genera il path completo del file da aprire
    if(stat(completepath,&s_file)){
        result=OP_NO_SUCH_FILE;
        //il file non esiste
    }
    else if(!(fileToMap=open(completepath, O_RDONLY))){ //apertura del file
        perror("Apertura del File");//errore nell'apertura
    }
    else {
        //apertura avvenuta con successo
        result=OP_OK;
        fileData=mmap(NULL,s_file.st_size,PROT_READ,MAP_PRIVATE,fileToMap,0);
        //salva in fileData il contenuto del file
        setHeader(&(reply.hdr),result,"");
        setData(&reply.data,"",fileData,s_file.st_size);
        //setta il messaggio di risposta
        MUTEXARRAYLOCK((*writingLock),fd);
        if(sendRequest(fd,&reply)<=0) errConnection=-1;
        else{
            updnfile(chattystats,-1);
            updfile(chattystats,1);
            //file inviato correttamente, aggiorno statistiche
        }
        MUTEXARRAYUNLOCK((*writingLock),fd);
        //trasmette il file al client
    }

    free(completepath);

    if(result!=OP_OK){
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        MUTEXARRAYLOCK((*writingLock),fd);
        if(sendRequest(fd,&reply)<=0) errConnection=-1;
        MUTEXARRAYUNLOCK((*writingLock),fd);
        upderrors(chattystats,1);
        //esito non positivo, spedisce l'esito e aggiorna le statistiche
    }
    return errConnection; //stato connessione
}

/**
 * @function getprevmsgs_op
 * @brief funzione che invia la history dei messaggi all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int getprevmsgs_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    op_t result=OP_FAIL;
    message_t reply;
    history* newHistory=NULL; //nuova history in cui salvare i messaggi da spedire
    result=prevMessage(usrmngr, msg.hdr.sender, &newHistory);
    //richiede i messaggi nella History
    if(result!=OP_OK){ //esito negativo
        upderrors(chattystats,1);
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        MUTEXARRAYLOCK((*writingLock),fd);
        if(sendRequest(fd,&reply)<=0) errConnection=-1; //se la trasmissione non ha successo setta errConnection
        MUTEXARRAYUNLOCK((*writingLock),fd);
        //setta il messaggio di risposta e trasmette l'esito
    }
    else{
        //esito positivo
        size_t nMex=newHistory->nel;
        memset(&reply,0,sizeof(message_t));
        setHeader(&reply.hdr,result,"");
        setData(&reply.data,"",(char*)&(nMex),sizeof(size_t)); //nel buffer del messaggio mette il numero di messaggi che il client sta per ricevere (da 0 a MaxHistMsgs)
        //setta il messaggio di risposta
        MUTEXARRAYLOCK((*writingLock),fd);
        if(sendRequest(fd,&reply)<=0) errConnection=-1; //trasmette il messaggio di risposta
        else{
            int i=newHistory->first;
            int el=0;
            while(el<newHistory->nel){ //itero sui messaggi contenuti nella history
                if(sendRequest(fd,newHistory->data[i])<=0) errConnection=-1; //trasmette il messaggio, in caso di fallimento setto errConnection
                else{

                    if(newHistory->data[i]->hdr.op==TXT_MESSAGE){
                        if(newHistory->pending[i]==1)
                            updndelivered(chattystats, -1);
                        updelivered(chattystats, 1);
                    }
                    //aggiorna le statistche del server
                }
                ++el;
                i=(i+1)%newHistory->size;
            }
        }
        MUTEXARRAYUNLOCK((*writingLock),fd);
        freeHistory(newHistory);
        //libero la memoria per la History che ho ricevuto
    }
    return errConnection; //stato connessione
}

/**
 * @function usrlist_op
 * @brief funzione che invia la lista degli utenti connessi al momento
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int usrlist_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0;
    message_t reply;
    stringlist* users;
    op_t result=OP_FAIL;
    users=connectedUserList(usrmngr);
    //ottiene la lista degli utenti connessi
    if(users){
        result=OP_OK;
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->lenght);
        //esito positivo, setta il messaggio
    }
    else {
        result=OP_FAIL;
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
        //esito negativo, aggiorna le statistiche
    }
    setHeader(&(reply.hdr),result,"");
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendRequest(fd,&reply)<=1) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione

    freeStringList(users); //liberro la lista degli utenti connessi

    return errConnection; //stato connessione
}

/**
 * @function unregister_op
 * @brief funzione che deregistra un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int unregister_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    op_t result=OP_FAIL;
    message_t reply;
    result=unregisterUser(usrmngr,msg.hdr.sender);
    //deregistra l'utente, ritorna l'esito dell'operazione

    if(result==OP_OK){
        updusers(chattystats,-1);
        updonline(chattystats,-1);
        //operazione positiva, aggiorna statistiche
    }
    else upderrors(chattystats,1); //aggiorna statistiche in caso di errore
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXARRAYLOCK((*writingLock),fd);
    sendRequest(fd,&reply);
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette il risultato dell'operazione
    return -1; //dopo unregister_op il client è automaticamente disconesso, a prescindere dall'esito
}

/**
 * @function disconnect_op_op
 * @brief funzione che disconnette un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int disconnect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    op_t result=OP_FAIL;
    result=disconnectUser(usrmngr,fd);
    //esegue le operazioni di disconnessione
    if(result==OP_OK){
        updonline(chattystats,-1);
    }
    else upderrors(chattystats,1);
    //aggiorna le statistiche in base all'esito
    //non spedisce nessun esito perchè il client si sta disconnettendo
    return -1; //dopo disconnect_op la connessione con il client è automaticamente chiusa.
}

/**
 * @function creategroup_op
 * @brief funzione che  crea un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int creategroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    op_t result=OP_FAIL;
    result=createGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    //crea un gruppo, restituisce l'esito dell'operazione

    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendRequest(fd,&reply)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione
    return errConnection; //stato connessione
}

/**
 * @function addgroup_op
 * @brief funzione che aggiunge un utente ad un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int addgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    op_t result=OP_FAIL;
    result=addtoGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    //aggiunge il richiedente al gruppo
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendRequest(fd,&reply)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione
    return errConnection; //stato connessione
}

/**
 * @function delfromgroup_op
 * @brief funzione che elimina un utente da un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int delfromgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    op_t result=OP_FAIL;
    result=deletefromGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    //elimina l'utente richiedente dal gruppo. Se il richiedente è il creatore del gruppo, anche il gruppo viene eliminato
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendRequest(fd,&reply)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione
    return errConnection; //stato Connessione
}

/**
 * @function delgroup_op
 * @brief funzione che elimina un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int delgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t reply;
    op_t result=OP_FAIL;
    result=deleteGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    //elimina il gruppo richiesto.
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXARRAYLOCK((*writingLock),fd);
    if(sendRequest(fd,&reply)<=0) errConnection=-1;
    MUTEXARRAYUNLOCK((*writingLock),fd);
    //trasmette l'esito dell'operazione
    return errConnection; //stato connessione
}

/**
 * @function execute
 * @brief funzione che legge un messaggio inviato al server sul socket fd e ne esegue la richiesta
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @param lock ArrayLock per le scritture sui socket
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int execute(int fd, manager* usrmngr, configs* configurazione,struct statistics* chattystats,arrayLock* writingLock){
    int errConnection=0; //errore durante la trasmissione dei dati. 0 successo, -1 connessione fallita
    message_t msg;
    message_t reply_err;
    if(readMsg(fd,&msg)<=0){ //legge la richiesta del client
        msg.hdr.op=OP_FAIL;
         errConnection=-1;
    }
    else{
        switch(msg.hdr.op){ //switch sul tipo di richiesta
            case REGISTER_OP: errConnection=register_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case CONNECT_OP: errConnection=connect_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case POSTTXT_OP: errConnection=posttxt_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case POSTTXTALL_OP: errConnection=posttextall_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case POSTFILE_OP: errConnection=postfile_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case GETFILE_OP: errConnection=getfile_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case GETPREVMSGS_OP: errConnection=getprevmsgs_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case USRLIST_OP: errConnection=usrlist_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case UNREGISTER_OP: errConnection=unregister_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case DISCONNECT_OP: errConnection=disconnect_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case CREATEGROUP_OP: errConnection=creategroup_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case ADDGROUP_OP: errConnection=addgroup_op(fd,msg,usrmngr,configurazione,chattystats,writingLock);
                                break;
            case DELGROUP_OP: errConnection=delfromgroup_op(fd,msg,usrmngr,configurazione,chattystats, writingLock);
                                break;
            case DELALLGROUP_OP: errConnection=delgroup_op(fd,msg,usrmngr,configurazione,chattystats, writingLock);
                                break;
            default:
                setHeader(&(reply_err.hdr),OP_NOT_EXISTS,"");
                setData(&(reply_err.data),"",NULL,0);
                MUTEXARRAYLOCK((*writingLock),fd);
                if(sendRequest(fd,&reply_err)<=0) errConnection=-1;
                MUTEXARRAYUNLOCK((*writingLock),fd);
                upderrors(chattystats,1);
                //richiesta non riconosciuta. trasmette messaggio di errore

        }
        free(msg.data.buf);
    }
    if(msg.hdr.op==UNREGISTER_OP || msg.hdr.op==DISCONNECT_OP) return -1; //se è stata richiesta unregister_op o disconnect_op la connessione è automaticamente chiusa
    if(errConnection== -1){
        if(disconnectUser(usrmngr,fd)==OP_OK)
            updonline(chattystats,-1);
            //errore nella connessione, disconnette il client.
    }

    return errConnection; //stato connessione
}
