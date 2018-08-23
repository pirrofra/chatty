/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file user.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include <user.h>
#include <message.h>
#include <icl_hash.h>
#include <history.h>
#include <ops.h>
#include <pthread.h>
#include <stdlib.h>
#include <groupdata.h>
#include <config.h>
#include <string.h>
#include <stringlist.h>

/**
 * @function freeunregisterUser
 * @brief funzione che libera lo spazio occupato da data
 * @param data spazio da liberare
*/
void freeunregisterUser(void* data){
    userdata* tmp= (userdata*) data; //casting necessario
    freeHistory(tmp->user_history);
    free(data);
}

/**
 * @function simpleHash
 * @brief funzione hash per gli interi
 * @param key chiave di cui calcolare l'hash
 * @return il valore della funzione hash calcolato su key
*/
unsigned int simpleHash(void* key){
    return *(int*)key;
}

/**
 * @function simpleCompare
 * @brief semplice funzione compare
 * @param a primo valore intero da comparare
 * @param b secondo valore intero da comparare
 * @return 1 se a==b, 0 altrimenti
*/
int simpleCompare(void* a, void* b){
    return *(int*) a == *(int*)b;
}

/**
 * @function initializeManager
 * @brief inizializza il gestore degli Utenti
 * @param usrmngr puntantore al manager da inizializzare
 * @param max_user massimo utenti connessi
 * @param dim_history numero massimo di messaggi salvabili
 * @param numMutex numero di Mutex per le hashtable
 * @return 0 operazione riuscita, -1 errore
*/
int initializeManager(manager* usrmngr, int max_user, int dim_history, int numMutex){
    if(max_user<=0||dim_history<=0||numMutex<=0) return -1; //parametri invalidi

    SYSCALLCHECK(pthread_mutex_init(&((usrmngr)->lockc),NULL),"Inizializzazzione Mutex"); //inizializzazione unico lock per connected_user

    if(initializeArrayLock(&usrmngr->lockr, numMutex)==-1) return -1; //arrayLock per registred_user
    if(initializeArrayLock(&usrmngr->lockg, numMutex)==-1) return -1; //arrayLock per groups

    (usrmngr)->max_connected_user=max_user;
    (usrmngr)->history_size=dim_history;
    (usrmngr)->registred_user=icl_hash_create(USERTABLEDIM,hash_pjw,string_compare);
    MEMORYCHECK((usrmngr)->registred_user);
    //crea  hashtable registred_user
    (usrmngr)->connected_user=icl_hash_create(max_user,simpleHash, simpleCompare);
    MEMORYCHECK((usrmngr)->connected_user);
    //crea hashtable connected_user
    (usrmngr)->groups=icl_hash_create(GTABLEDIM,hash_pjw,string_compare);
    MEMORYCHECK((usrmngr)->groups);
    //crea hashtable groups
    return 0;
}

/**
 * @function registerUser
 * @brief registra un nuovo utente
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nickname dell'utente da registrare
 * @param fd socke del client da associare
 * @return op_t esito operazione
*/
op_t registerUser(manager* usrmngr, char* nickname,int fd){
    int retInsert=0; //valore di ritorno della insert
    int i=hash(usrmngr->registred_user,(void*) nickname); //funzione hash calcolata sul nickname
    char* newnick;
    char* newnick2;
    //ha bisogno di copiare due volte lo stesso nickname
    int* key;
    int retInsert2=0; //valore di ritorno della seconda insert
    op_t result=OP_FAIL;
    userdata* data;
    if(usrmngr==NULL||nickname==NULL) return OP_FAIL;
    data=malloc(sizeof(userdata));
    MEMORYCHECK(data);
    //alloca userdata
    newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
    //copia nickname in newnick

    MUTEXARRAYLOCK(usrmngr->lockr,i); //mutex lock sull'arrayLock
    retInsert=icl_hash_insert(usrmngr->registred_user,newnick, data);
    if(retInsert==-1) //insert fallimento generico
        result=OP_FAIL;
    else if(retInsert==0||groupexist(usrmngr,nickname)) //nickname esiste già
        result=OP_NICK_ALREADY;
    else{
        data->fd=fd;
        initializeHistory(&(data->user_history),usrmngr->history_size);
        //inizializza la history per il nuovo utente
        LOCKACQUIRE(usrmngr->lockc);
        key=malloc(sizeof(int));
        MEMORYCHECK(key);
        *key=fd;
        //crea chiave per connected_user
        newnick2=malloc(sizeof(char)*(MAX_NAME_LENGTH+1));
        MEMORYCHECK(newnick2);
        memset(newnick2,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
        strncpy(newnick2,nickname,MAX_NAME_LENGTH*sizeof(char));
        //copia nickname in newnick2
        retInsert2=icl_hash_insert(usrmngr->connected_user,key,newnick2);
        if(retInsert2==-1){ //fallimento generico
                free(key);
                free(newnick2);
                result=OP_FAIL;
            }
       else if(retInsert2==0){ //client sul socket fd già connesso
                free(key);
                free(newnick2);
                result=OP_CLNT_ALREADY_CONNECTED;
            }
       else result=OP_OK; //esito positivo
        LOCKRELEASE(usrmngr->lockc);
    }
    MUTEXARRAYUNLOCK(usrmngr->lockr,i);

    if(result==OP_FAIL || result==OP_NICK_ALREADY){
        free(newnick);
        free(data);
        //libero in caso di fallimento
    }
    return result; //risultato operazione
}

/**
 * @function connectUser
 * @brief connette un client con un nickname
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nickname utente
 * @param fd socket del client da associare
 * @return op_t esito operazione
*/
op_t connectUser(manager* usrmngr, char* nickname, int fd){
    int retInsert=0;
    int i=hash(usrmngr->registred_user,(void*) nickname);
    char* newnick;
    int* key;
    op_t result=OP_FAIL;
    userdata* data;
    if(usrmngr==NULL||nickname==NULL||fd<0) return OP_FAIL; //parametri invalidi
    MUTEXARRAYLOCK(usrmngr->lockr,i); //lock con arrayLock
    data=icl_hash_find(usrmngr->registred_user, nickname); //trova l'userdata in registred_user con chiave nickname
    if(data==NULL) //utente non registrato
        result=OP_NICK_UNKNOWN;
    else if(data->fd!=-1) //utente già connesso
        result=OP_USR_ALREADY_CONNECTED;
    else{
        newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
        MEMORYCHECK(newnick);
        memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
        strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
        //copia nickname in newnick
        key=malloc(sizeof(int));
        MEMORYCHECK(key);
        *key=fd;
        //crea nuova chiave per connected_user

        LOCKACQUIRE(usrmngr->lockc);
        if(usrmngr->max_connected_user<=icl_hash_dimension(usrmngr->connected_user)) {
            result=OP_TOO_MANY_CLIENT;
            free(key);
            free(newnick);
            //troppi utenti connessi
        }
        else{
            retInsert=icl_hash_insert(usrmngr->connected_user,key,newnick); //inserimento in connected_user
            if(retInsert==-1){ //fallimento generico
                free(key);
                free(newnick);
                result=OP_FAIL;
            }
            else if(retInsert==0){ //esiste una entry di chiave key, client già connesso
                free(key);
                free(newnick);
                result=OP_CLNT_ALREADY_CONNECTED;
            }
            else{
                //inserimento con successo
                data->fd=*key;
                result=OP_OK;
            }
        }
        LOCKRELEASE(usrmngr->lockc);
    }
    MUTEXARRAYUNLOCK(usrmngr->lockr,i);
    return result; //esito operazione
}

/**
 * @function unregisterUser
 * @brief deregistra un utente
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nickname dell'utente da deregistrare
 * @return op_t esito operazione
*/
op_t unregisterUser(manager* usrmngr, char* nickname){
    userdata* data=NULL;
    op_t result=OP_FAIL;
    groupdata* gdata=NULL;
    int i=0;
    char* key=NULL;
    icl_entry_t* j=NULL;
    int h1=0;
    int h2=0;
    if(usrmngr==NULL||nickname==NULL)return OP_FAIL;
    h2=hash(usrmngr->registred_user,(void*) nickname); //hash sul nickname dell'utente da deregistrare
    MUTEXARRAYLOCK(usrmngr->lockr,h2);

    for (i=0;i<(usrmngr->groups)->nbuckets; i++){ //itero su tutte le entry di groups
            h1=i;
            MUTEXARRAYLOCK(usrmngr->lockg,h1);
            for (j=(usrmngr->groups)->buckets[i];j!=NULL&&((key=j->key)!=NULL)&&((gdata=j->data)!=NULL);j=j->next){
                kick(gdata,nickname);
            }
            MUTEXARRAYUNLOCK(usrmngr->lockg,h1);
    }
    //l'utente è eliminato da tutti i gruppi a cui potrebbe appartenere

    data=icl_hash_find(usrmngr->registred_user,nickname);
    if(data==NULL) result= OP_NICK_UNKNOWN; //l'utente non esiste
    else{
        LOCKACQUIRE(usrmngr->lockc);
        icl_hash_delete(usrmngr->connected_user,&(data->fd),free,free); //l'utente è disconnesso
        LOCKRELEASE(usrmngr->lockc);
        icl_hash_delete(usrmngr->registred_user, nickname,free, freeunregisterUser); //l'utente è eliminato dagli utenti registrati
        result=OP_OK;
    }
    MUTEXARRAYUNLOCK(usrmngr->lockr,h2);
    return result; //esisto operazione
}

/**
 * @function disconnectUser
 * @brief disconnette utente  dal server
 * @param usrmngr puntatore al gestore degli utenti
 * @param fd file descriptor del client da disconnettere
 * @return op_t esito operazione
*/
op_t disconnectUser(manager* usrmngr, int fd){
    int retDelete=0; //valore di ritorno della delete
    userdata* data;
    char* name;
    op_t result=OP_FAIL;
    if(usrmngr==NULL||fd<0) return OP_FAIL; //parametri invalidi

    LOCKACQUIRE(usrmngr->lockc);
    name=icl_hash_find(usrmngr->connected_user,&(fd)); //nickname dell'utente da disconnettere
    retDelete=icl_hash_delete(usrmngr->connected_user,&(fd),free,NULL); //eliminazione dell'utente da connected_user
    LOCKRELEASE(usrmngr->lockc);
    if(retDelete==0) result=OP_USR_NOT_CONNECTED; //utente non connesso
    else if(retDelete==-1) result=OP_FAIL; //fallimento generio
    else{
        //utente disconnesso con successo
        int i=hash(usrmngr->registred_user,(void*) name);
        MUTEXARRAYLOCK(usrmngr->lockr,i);
        data=icl_hash_find(usrmngr->registred_user,name);
        data->fd=-1; //modifica della  copia del fd salvato in registred_user
        MUTEXARRAYUNLOCK(usrmngr->lockr,i);
        free(name);
        result=OP_OK;
    }
    return result; //esito operazione
}

/**
 * @function storeMessage
 * @brief conserva il messaggio nella history
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname utente che ha ricevuto il messaggio
 * @param msg messaggio spedito
 * @return -3 Nickname inesistente, -2 errore, fd dell'utente
*/
int storeMessage(manager* usrmngr, char* nickname ,message_t* msg){
    int result=-2;
    int retAddMessage=0;
    int i=hash(usrmngr->registred_user,(void*) nickname); //hash del nickname
    userdata* data;
    if(usrmngr==NULL||nickname==NULL||msg==NULL) return -2; //parametri non validi
    MUTEXARRAYLOCK(usrmngr->lockr,i);
    data=icl_hash_find(usrmngr->registred_user, nickname); //ricerca di nickname in registred_user
    if(data==NULL) result=-3;
    else{
        retAddMessage=addMessage(data->user_history, msg,data->fd); //aggiunge il messaggio alla history
        if(retAddMessage==-1) //fallimento generico
            result=-2;
        else
            result=data->fd; //fd dell'utente
    }
    MUTEXARRAYUNLOCK(usrmngr->lockr,i);
    return result; //fd dell'utente
}

/**
 * @function prevMessage
 * @brief copia la history dei messaggi dell'utente in ordine dal più recente al meno recente
 * @param usrmngr puntatore al gestore degli Utenti
 * @param nickname utente di cui copiare la history
 * @param newHistory puntatore al puntatore della nuova history
 * @return op_t esito operazione
*/
op_t prevMessage(manager* usrmngr, char* nickname,history** newHistory){
    op_t result=OP_FAIL;
    userdata* data;
    int i=hash(usrmngr->registred_user,(void*) nickname); //hash di nickname
    *newHistory=NULL;
    MUTEXARRAYLOCK(usrmngr->lockr,i);
    data=icl_hash_find(usrmngr->registred_user,nickname); //trova nickname in registred_user
    if(data){
        //nickname trovato
        *newHistory=copyHistory(data->user_history); //copia la history di nickname in newHistory
        if(*newHistory) {
            result=OP_OK;
            resetPending(data->user_history);
            //esito positivo, pending è resettato
        }
        else result=OP_FAIL; //fallimento generico
    }
    else result=OP_NICK_UNKNOWN; //nickname non trovato
    MUTEXARRAYUNLOCK(usrmngr->lockr,i);
    return result; //esito operazione
}

/**
 * @function createGroup
 * @brief crea un nuovo gruppo
 * @param usrmngr puntantore al gestore degli utenti
 * @param creator nickname del creatore
 * @param name nome del gruppo
 * @return op_t esito operazione
*/
op_t createGroup(manager* usrmngr, char* creator, char* name){
    op_t result=OP_FAIL;
    groupdata* group;
    userdata* data;
    char* newname;
    int retInsert;
    if(usrmngr==NULL||creator==NULL||name==NULL) return OP_FAIL; //paramentri non validi
    initializeGroup(&group,creator,GTABLEDIM);
    //inizializzazione gruppo
    newname=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newname);
    memset(newname,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newname,name,MAX_NAME_LENGTH*sizeof(char));
    //name copiato in newname (gruppo)
    int h1=0;
    int h2=0;
    h1=hash(usrmngr->groups,(void*) name); //hash di name su groups
    h2=hash(usrmngr->registred_user,(void*)name); //hash di name su registred_user
    MUTEXARRAYLOCK(usrmngr->lockr,h2);
    data=icl_hash_find(usrmngr->registred_user,name); //cerca se esiste già un utente di nome name
    if(!data){
        //utente non esiste, puo' essere creato il gruppo
        MUTEXARRAYLOCK(usrmngr->lockg,h1);
        retInsert=icl_hash_insert(usrmngr->groups, newname, group); //inserimento gruppo
        if(retInsert==-1) //fallimento generico
            result=OP_FAIL;
        else if(retInsert==0) // gruppo name esiste già
            result=OP_NICK_ALREADY;
        else {
            addMember(group, creator);
            result=OP_OK;
            //il creatore è aggiunto al gruppo, esito positivo
        }
        MUTEXARRAYUNLOCK(usrmngr->lockg,h1);
    }
    else result=OP_NICK_ALREADY; //name già preso

    MUTEXARRAYUNLOCK(usrmngr->lockr,h2); //conserva il lock fino alla fine, per evitare che qualcuno aggiunga un utente di nome name
    if(result==OP_FAIL || result==OP_NICK_ALREADY){
        free(newname);
        freeGroup(group);
        //in caso di fallimento libero la memoria
    }
    return result; //esito operazione
}

/**
 * @function addtoGroup
 * @brief aggiunge un utente a un gruppo
 * @param usrmngr puntantore al gestore degli utenti
 * @param nickname utente da aggiungere al gruppo
 * @param groupname nome del gruppo
 * @return op_t esito operazione
*/
op_t addtoGroup(manager* usrmngr, char*nickname, char* groupname){
    op_t result=OP_FAIL;
    groupdata* group;
    userdata* data;
    int retInsert=0;
    int h1=0;
    int h2=0;
    if(usrmngr==NULL||nickname==NULL||groupname==NULL) return OP_FAIL;
    h1=hash(usrmngr->groups,(void*) groupname); //hash sul gruppo
    h2=hash(usrmngr->registred_user, (void*) nickname); //hash sul nickname dell'utente
    MUTEXARRAYLOCK(usrmngr->lockr,h2);
    data=icl_hash_find(usrmngr->registred_user,nickname);//cerca l'utente
    if(data!=NULL){
        //l'utente nickname esiste
        MUTEXARRAYLOCK(usrmngr->lockg,h1);
        group=icl_hash_find(usrmngr->groups,groupname); //cerca il gruppo
        if(group==NULL) result=OP_NICK_UNKNOWN;  //gruppo inesistente
        else{
            retInsert=addMember(group, nickname); //aggiunge utente al gruppo
            if(retInsert==-1)result=OP_FAIL; //fallimento generico
            else if(retInsert==0) result=OP_USR_ALREADY_IN_GROUP; //utente già inserito nel gruppo
            else result=OP_OK; //esito positivo
        }
        MUTEXARRAYUNLOCK(usrmngr->lockg,h1);
    }
    else result=OP_NICK_UNKNOWN; //utente inesistente
    MUTEXARRAYUNLOCK(usrmngr->lockr,h2);
    return result; //esito operazione
}

/**
 * @function deletefromGroup
 * @brief elimina un utente dal gruppo
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nome dell'utente da bannare
 * @param groupname nome del gruppo
 * @return op_t esito dell'operazione
*/
op_t deletefromGroup(manager* usrmngr, char*nickname, char* groupname){
    op_t result=OP_FAIL;
    groupdata* group;
    int retDelete=0;
    int h=0;
    if(usrmngr==NULL||nickname==NULL||groupname==NULL) return OP_FAIL; //parametri invalidi
    h=hash(usrmngr->groups,(void*) groupname); //hash sul gruppo
    MUTEXARRAYLOCK(usrmngr->lockg,h);
    group=icl_hash_find(usrmngr->groups,groupname); //ricerca del gruppo
    if(group==NULL) result=OP_NICK_UNKNOWN; //gruppo inesistente
    else if (string_compare((void*)group->admin,(void*)nickname)){
        retDelete=icl_hash_delete(usrmngr->groups,groupname,free,freeGroup);
        if(retDelete==-1) result=OP_FAIL; //fallimento generico
        else result=OP_OK;
        //utente che vuole uscire è il creatore
        //il gruppo è cancellato
    }
    else{
        retDelete=kick(group,nickname); //utente eliminato dal gruppo
        if(retDelete==-1) result=OP_FAIL; //fallimento generico
        else if(retDelete==0) result=OP_NICK_UNKNOWN; //utente non presente nel gruppo
        else result=OP_OK; //esito positivo
    }
    MUTEXARRAYUNLOCK(usrmngr->lockg,h);
    return result; //esito operazione
}

/**
 * @function deleteGroup
 * @brief elimina un intero gruppo
 * @param usrmngr puntatore al gestore degli utenti
 * @param groupname nome del gruppo da eliminare
 * @param nickname nickname dell'utente che richiede l'operazione
 * @return op_t esito dell'operazione
*/
op_t deleteGroup(manager* usrmngr,char*nickname,char* groupname){
    groupdata* group;
    int retDelete=0;
    op_t result=OP_FAIL;
    int h=0;
    if(usrmngr==NULL||groupname==NULL||nickname==NULL) return OP_FAIL; //parametri invalidi
    h=hash(usrmngr->groups,(void*) groupname); //hash sul gruppo
    MUTEXARRAYLOCK(usrmngr->lockg,h);
    group=icl_hash_find(usrmngr->groups,groupname); //ricerca del gruppo
    if(group==NULL) result=OP_NICK_UNKNOWN; //gruppo inesistente
    else {
        if(!string_compare((void*)group->admin,(void*)nickname)) result=OP_NO_PERMISSION; //utente che ha richiesto la cancellazione non è il creatore. impossibile procedere con l'eliminazione
        else{
            retDelete=icl_hash_delete(usrmngr->groups,groupname,free,freeGroup);
            if(retDelete==-1) result=OP_FAIL; //fallimento generico
            else result=OP_OK; //esito positivo
            //cancellazione del gruppo
        }
    }
    MUTEXARRAYUNLOCK(usrmngr->lockg,h);
    return result; //esito operazione
}

/**
 * @function userGroupList
 * @brief ritorna la lista degli utenti in un gruppo
 * @param usrmngr puntatore al gestore degli utenti
 * @param groupname nome del gruppo di cui restituire gli utenti
 * @return lista degli utenti iscritti al gruppo
*/
stringlist* userGroupList(manager* usrmngr, char* groupname){
    int err=0; //errore in addString
    int p=0;
    groupdata* group;
    stringlist* ret=NULL;

    int h=0;
    char* kp;
    char* dp;
    int i=0;
    icl_entry_t* j;
    //necessari per icl_hash_foreach

    if(usrmngr==NULL||groupname==NULL) return ret; //parametri invalidi
    h=hash(usrmngr->groups,(void*) groupname); //hash sul gruppo
    MUTEXARRAYLOCK(usrmngr->lockg,h);
    group=icl_hash_find(usrmngr->groups,groupname); //ricerca del gruppo
    if(group==NULL) ret=NULL; //gruppo inesistente
    else{
        initializeStringList(&ret,icl_hash_dimension(group->users),MAX_NAME_LENGTH);
        //inizializzazione stringlist
        icl_hash_foreach(group->users, i, j, kp, dp){
            err=err||(addString(&ret,p,kp));
            ++p;
        }
        //tutti gli utenti nel gruppo sono aggiunti alla stringlist
    }
    MUTEXARRAYUNLOCK(usrmngr->lockg,h);
    if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
        //in caso di errori nella addString libera memoria e restituisce null
    }
    return ret; //stringlist, o NULL in caso di errore
}

/**
 * @function connectedUserList
 * @brief ritorna la lista degli utenti connessi
 * @param usrmngr puntatore al gestore degli utenti
 * @return lista degli utenti connessi
*/
stringlist* connectedUserList(manager* usrmngr){
    int err=0; //errore in addString
    int p=0;
    stringlist* ret=NULL;

    int* kp;
    char* dp;
    int i=0;
    icl_entry_t* j;
    //necessari per icl_hash_foreach
    if(usrmngr==NULL) return ret; //paramentri invalidi
    LOCKACQUIRE(usrmngr->lockc);
    initializeStringList(&ret,icl_hash_dimension(usrmngr->connected_user),MAX_NAME_LENGTH);
    icl_hash_foreach((usrmngr->connected_user), i, j, kp, dp){
        err=err||(addString(&ret,p,dp));
        ++p;
    }
    //aggiunge tutti gli utenti connessi al server in stringlist

    LOCKRELEASE(usrmngr->lockc);
    if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
        //in caso di errore in addString libera la memoria allocata dalla stringlist e restituisce NULL
    }
    return ret;  //stringlist, o NULL in caso di errore
}

/**
 * @function registredUserList
 * @brief ritorna la lista degli utenti registrati
 * @param usrmngr puntatore al gestore degli utenti
 * @param numRegUsers numero di Utenti Registrati
 * @return lista degli utenti registrati
*/
stringlist* registredUserList(manager* usrmngr, int numRegUsers){
    int err=0;
    int p=0;
    stringlist* ret=NULL;

    userdata* data;
    int i;
    char* key;
    icl_entry_t* j;
    //necessari per icl_hash_foreach

    if(usrmngr==NULL) return ret; //paramentri non validi
    for(i=0;i<(usrmngr->lockr.numMutex);i++){
        MUTEXARRAYLOCK(usrmngr->lockr,i)};
    }
    //lock dell'intera tabella. Parentesi necessaria per mantenere il for corretto
    initializeStringList(&ret,icl_hash_dimension(usrmngr->registred_user),MAX_NAME_LENGTH);
    for (i=0;i<(usrmngr->registred_user)->nbuckets; i++){
            for (j=(usrmngr->registred_user)->buckets[i];j!=NULL&&((key=j->key)!=NULL)&&((data=j->data)!=NULL);j=j->next){
                err=err ||(addString(&ret,p,key));
                ++p;
            }
            //aggiunge tutti gli utenti registrati alla stringlist
    }
    for(i=0;i<(usrmngr->lockr.numMutex);i++){
        {MUTEXARRAYUNLOCK(usrmngr->lockr,i);
    }
    //unlock dell'intera tabella. parentesi necessaria per mantenere il for corretto
     if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
        //in caso di errore oon addString libera la memoria allocata dalla stringlist e restituisce NULL
    }
    return ret; //stringlist, o NULL in caso di errore
}

/**
 * @function groupexist
 * @brief verifica l'esistenza di un gruppo di nome name
 * @param usrmngr puntatore al gestore degli Utenti
 * @param name nome del gruppo da controllare
 * @return 0 il gruppo non esiste, 1 esiste
*/
int groupexist(manager* usrmngr, char* name){
    int result=0;
    int i=hash(usrmngr->groups,(void*) name); //hash sul gruppo
    MUTEXARRAYLOCK(usrmngr->lockg,i);
    if(icl_hash_find(usrmngr->groups,name)) result=1; //ricerca del gruppo
    else result=0;
    MUTEXARRAYUNLOCK(usrmngr->lockg,i);
    return result; //esito della ricerca
}

/**
 * @function isingroup
 * @brief verifica la presenza di un utente di nome nickname nel gruppo groupname
 * @param usrmngr puntatore al gestore degli Utenti
 * @param nickname utente da cercare nel gruppo
 * @param groupname gruppo in cui cercare
 * @param 0 l'utente o il gruppo non esiste, 1 l'utente è nel gruppo
*/
int isingroup(manager* usrmngr, char* nickname,char* groupname){
    int result=0;
    groupdata* group;
    int i=hash(usrmngr->groups,(void*) groupname); //hash sul gruppo
    MUTEXARRAYLOCK(usrmngr->lockg,i);
    group=icl_hash_find(usrmngr->groups,groupname); //ricerca del gruppo
    if(group && icl_hash_find(group->users, nickname)) result=1; //ricerca dell'utente nel gruppo
    MUTEXARRAYUNLOCK(usrmngr->lockg,i);
    return result; //esito ricerca
}

/**
 * @function destroy
 * @brief distrugge il gestore degli Utenti
 * @param usrmngr puntatore al gestore degli Utenti
*/
void destroy(manager* usrmngr){
    icl_hash_destroy(usrmngr->registred_user, free,freeunregisterUser);
    icl_hash_destroy(usrmngr->connected_user, NULL,free);
    icl_hash_destroy(usrmngr->groups,free,freeGroup);
    //eliminazione tabelle hash
    if((errno=pthread_mutex_destroy(&usrmngr->lockc))) perror("Cancellamento Mutex"); //eliminazione lock unico
    freeArrayLock(&usrmngr->lockr);
    freeArrayLock(&usrmngr->lockg);
    //eliminazione degli ArrayLock
}
