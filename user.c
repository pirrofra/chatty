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



void freeunregisterUser(void* data){
    userdata* tmp= (userdata*) data;
    freeHistory(tmp->user_history);
    free(data);
}


unsigned int simpleHash(void* key){
    return *(int*)key;
}

int simpleCompare(void* a, void* b){
    return *(int*) a == *(int*)b;
}

int initializeManager(manager* usrmngr, int max_user, int dim_history){
    if(max_user<=0||dim_history<=0) return -1;
    (usrmngr)->lockr=calloc(NUMMUTEX,sizeof(pthread_mutex_t));
    MEMORYCHECK((usrmngr)->lockr);
    (usrmngr)->lockg=calloc(NUMMUTEX, sizeof(pthread_mutex_t));
    MEMORYCHECK((usrmngr)->lockg);
    for(int i=0;i<NUMMUTEX;i++){
        SYSCALLCHECK(pthread_mutex_init(&((usrmngr)->lockr[i]),NULL),"Inizializzazzione Mutex");
    }

    SYSCALLCHECK(pthread_mutex_init(&((usrmngr)->lockc),NULL),"Inizializzazzione Mutex");

    for(int i=0;i<NUMMUTEX;i++){
        SYSCALLCHECK(pthread_mutex_init(&((usrmngr)->lockg[i]),NULL),"Inizializzazzione Mutex");
    }
    (usrmngr)->max_connected_user=max_user;
    (usrmngr)->history_size=dim_history;
    (usrmngr)->registred_user=icl_hash_create(USERTABLEDIM,hash_pjw,string_compare);
    MEMORYCHECK((usrmngr)->registred_user);
    (usrmngr)->connected_user=icl_hash_create(max_user,simpleHash, simpleCompare);
    MEMORYCHECK((usrmngr)->connected_user);
    (usrmngr)->groups=icl_hash_create(GTABLEDIM,hash_pjw,string_compare);
    MEMORYCHECK((usrmngr)->groups);
    return 0;
}

op_t registerUser(manager* usrmngr, char* nickname,int fd){
    int err=0;
    int i=hash_pjw((void*) nickname)%NUMMUTEX;
    char* newnick;
    char* newnick2;
    int* key;
    int tmp=0;
    op_t result=OP_FAIL;
    userdata* data;
    if(usrmngr==NULL||nickname==NULL) return OP_FAIL;
    data=malloc(sizeof(userdata));
    MEMORYCHECK(data);
    newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
    MUTEXLOCK(usrmngr->lockr[i]);
    err=icl_hash_insert(usrmngr->registred_user,newnick, data);
    if(err==-1)
        result=OP_FAIL;
    else if(err==0||groupexist(usrmngr,nickname))
        result=OP_NICK_ALREADY;
    else{
        data->fd=fd;
        initializeHistory(&(data->user_history),usrmngr->history_size);
        MUTEXLOCK(usrmngr->lockc);
        key=malloc(sizeof(int));
        MEMORYCHECK(key);
        *key=fd;
        newnick2=malloc(sizeof(char)*(MAX_NAME_LENGTH+1));
        MEMORYCHECK(newnick2);
        memset(newnick2,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
        strncpy(newnick2,nickname,MAX_NAME_LENGTH*sizeof(char));
        tmp=icl_hash_insert(usrmngr->connected_user,key,newnick2);
        if(tmp==-1){
                free(key);
                free(newnick2);
                result=OP_FAIL;
            }
       else if(tmp==0){
                free(key);
                free(newnick2);
                result=OP_CLNT_ALREADY_CONNECTED;
            }
       else result=OP_OK;
        MUTEXUNLOCK(usrmngr->lockc);
    }
    MUTEXUNLOCK(usrmngr->lockr[i]);
    if(result==OP_FAIL || result==OP_NICK_ALREADY){
        free(newnick);
        free(data);
    }
    return result;
}

op_t connectUser(manager* usrmngr, char* nickname, int fd){
    int err=0;
    int i=hash_pjw((void*) nickname)%NUMMUTEX;
    char* newnick=NULL;
    int* key;
    op_t result=OP_FAIL;
    userdata* data;
    if(usrmngr==NULL||nickname==NULL||fd<0) return OP_FAIL;
    MUTEXLOCK(usrmngr->lockr[i]);
    data=icl_hash_find(usrmngr->registred_user, nickname);
    if(data==NULL)
        result=OP_NICK_UNKNOWN;
    else if(data->fd!=-1)
        result=OP_USR_ALREADY_CONNECTED;
    else{
        newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
        MEMORYCHECK(newnick);
        memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
        strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
        key=malloc(sizeof(int));
        MEMORYCHECK(key);
        *key=fd;
        MUTEXLOCK(usrmngr->lockc);

        if(usrmngr->max_connected_user<=(usrmngr->connected_user)->nentries) {
            result=OP_TOO_MANY_CLIENT;
            free(key);
            free(newnick);
        }
        else{
            err=icl_hash_insert(usrmngr->connected_user,key,newnick);
            if(err==-1){
                free(key);
                free(newnick);
                result=OP_FAIL;
            }
            else if(err==0){
                free(key);
                free(newnick);
                result=OP_CLNT_ALREADY_CONNECTED;
            }
            else{
                data->fd=*key;
                result=OP_OK;
            }
        }
        MUTEXUNLOCK(usrmngr->lockc);
    }
    MUTEXUNLOCK(usrmngr->lockr[i]);
    return result;
}

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
    h2=hash_pjw((void*) nickname)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockr[h2]);
    for (i=0;i<(usrmngr->groups)->nbuckets; i++){
            h1=i%NUMMUTEX;
            MUTEXLOCK(usrmngr->lockg[h1]);
            for (j=(usrmngr->groups)->buckets[i];j!=NULL&&((key=j->key)!=NULL)&&((gdata=j->data)!=NULL);j=j->next){
                kick(gdata,nickname);
            }
            MUTEXUNLOCK(usrmngr->lockg[h1]);
    }
    data=icl_hash_find(usrmngr->registred_user,nickname);
    if(data==NULL) result= OP_NICK_UNKNOWN;
    else{
        MUTEXLOCK(usrmngr->lockc);
        icl_hash_delete(usrmngr->connected_user,&(data->fd),free,free);
        MUTEXUNLOCK(usrmngr->lockc);
        icl_hash_delete(usrmngr->registred_user, nickname,free, freeunregisterUser);
        result=OP_OK;
    }
    MUTEXUNLOCK(usrmngr->lockr[h2]);
    return result;
}

op_t disconnectUser(manager* usrmngr, int fd){
    int tmp=0;
    userdata* data;
    char* name;
    op_t result=OP_FAIL;
    if(usrmngr==NULL||fd<0) return OP_FAIL;
    MUTEXLOCK(usrmngr->lockc);
    name=icl_hash_find(usrmngr->connected_user,&(fd));
    tmp=icl_hash_delete(usrmngr->connected_user,&(fd),free,NULL);
    MUTEXUNLOCK(usrmngr->lockc);
    if(tmp==0) result=OP_USR_NOT_CONNECTED;
    else if(tmp==-1) result=OP_FAIL;
    else{
        int i=hash_pjw((void*) name)%NUMMUTEX;
        MUTEXLOCK(usrmngr->lockr[i]);
        data=icl_hash_find(usrmngr->registred_user,name);
        data->fd=-1;
        MUTEXUNLOCK(usrmngr->lockr[i]);
        free(name);
        result=OP_OK;
    }
    return result;
}

int storeMessage(manager* usrmngr, char* nickname ,message_t* msg){
    int result=-2;
    int tmp=0;
    int i=hash_pjw((void*) nickname)%NUMMUTEX;
    userdata* data;
    if(usrmngr==NULL||nickname==NULL||msg==NULL) return -2;
    MUTEXLOCK(usrmngr->lockr[i]);
    data=icl_hash_find(usrmngr->registred_user, nickname);
    if(data==NULL) result=-3;
    else{
        tmp=addMessage(data->user_history, msg,data->fd);
        if(tmp==-1)
            result=-2;
        else
            result=data->fd;
    }
    MUTEXUNLOCK(usrmngr->lockr[i]);
    return result;
}



op_t createGroup(manager* usrmngr, char* creator, char* name){
    op_t result=OP_FAIL;
    groupdata* group;
    userdata* data;
    char* newname;
    int tmp;
    if(usrmngr==NULL||creator==NULL||name==NULL) return OP_FAIL;
    initializeGroup(&group,creator,GTABLEDIM);
    newname=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newname);
    memset(newname,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newname,name,MAX_NAME_LENGTH*sizeof(char));
    int h=0;
    h=hash_pjw((void*) name)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockr[h]);
    data=icl_hash_find(usrmngr->registred_user,name);
    if(!data){
        MUTEXLOCK(usrmngr->lockg[h]);
        tmp=icl_hash_insert(usrmngr->groups, newname, group);
        if(tmp==-1)
            result=OP_FAIL;
        else if(tmp==0)
            result=OP_NICK_ALREADY;
        else result=OP_OK;
        MUTEXUNLOCK(usrmngr->lockg[h]);
    }
    else result=OP_NICK_ALREADY;

    MUTEXUNLOCK(usrmngr->lockr[h]);
    if(result==OP_FAIL || result==OP_NICK_ALREADY){
        free(newname);
        freeGroup(group);
    }
    return result;
}

op_t addtoGroup(manager* usrmngr, char*nickname, char* groupname){
    op_t result=OP_FAIL;
    groupdata* group;
    userdata* data;
    int tmp=0;
    int h1=0;
    int h2;
    if(usrmngr==NULL||nickname==NULL||groupname==NULL) return OP_FAIL;
    h1=hash_pjw((void*) groupname)%NUMMUTEX;
    h2=hash_pjw((void*) nickname)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockr[h2]);
    data=icl_hash_find(usrmngr->registred_user,nickname);
    if(data!=NULL){
        MUTEXLOCK(usrmngr->lockg[h1]);
        group=icl_hash_find(usrmngr->groups,groupname);
        if(group==NULL) result=OP_NICK_UNKNOWN;
        else{
            tmp=addMember(group, nickname);
            if(tmp==-1)result=OP_FAIL;
            else if(tmp==0) result=OP_USR_ALREADY_IN_GROUP;
            else result=OP_OK;
        }
        MUTEXUNLOCK(usrmngr->lockg[h1]);
    }
    else result=OP_NICK_UNKNOWN;
    MUTEXUNLOCK(usrmngr->lockr[h2]);
    return result;
}

op_t deletefromGroup(manager* usrmngr, char*nickname, char* groupname){
    op_t result=OP_FAIL;
    groupdata* group;
    int tmp=0;
    int h=0;
    if(usrmngr==NULL||nickname==NULL||groupname==NULL) return OP_FAIL;
    h=hash_pjw((void*) groupname)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockg[h]);
    group=icl_hash_find(usrmngr->groups,groupname);
    if(group==NULL) result=OP_NICK_UNKNOWN;
    else if (!string_compare((void*)group->admin,(void*)nickname)){
        result=deleteGroup(usrmngr,nickname,groupname);
    }
    else{
        tmp=kick(group,nickname);
        if(tmp==-1) result=OP_FAIL;
        else if(tmp==0) result=OP_NICK_UNKNOWN;
        else result=OP_OK;
    }
    MUTEXUNLOCK(usrmngr->lockg[h]);
    return result;
}

op_t deleteGroup(manager* usrmngr,char*nickname,char* groupname){
    groupdata* group;
    int tmp;
    op_t result=OP_FAIL;
    int h=0;
    if(usrmngr==NULL||groupname==NULL||nickname==NULL) return OP_FAIL;
    h=hash_pjw((void*) groupname)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockg[h]);
    group=icl_hash_find(usrmngr->groups,groupname);
    if(group==NULL) result=OP_NICK_UNKNOWN;
    else {
        if(!string_compare((void*)group->admin,(void*)nickname)) result=OP_NO_PERMISSION;
        else{
            tmp=icl_hash_delete(usrmngr->groups,groupname,free,freeGroup);
            if(tmp==-1) result=OP_FAIL;
            else result=OP_OK;
        }
    }
    MUTEXUNLOCK(usrmngr->lockg[h]);
    return result;
}

stringlist* userGroupList(manager* usrmngr, char* groupname){
    int err=0;
    int p=0;
    groupdata* group;
    stringlist* ret=NULL;
    int h=0;
    char* kp;
    char* dp;
    int i=0;
    icl_entry_t* j;
    if(usrmngr==NULL||groupname==NULL) return ret;
    h=hash_pjw((void*) groupname)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockg[h]);
    group=icl_hash_find(usrmngr->groups,groupname);
    if(group==NULL) ret=NULL;
    else{
        initializeStringList(&ret,(group->users)->nentries,MAX_NAME_LENGTH);
        icl_hash_foreach(group->users, i, j, kp, dp){
            err=err||(addString(ret,p,kp));
            ++p;
        }
    }
    MUTEXUNLOCK(usrmngr->lockg[h]);
    if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
    }
    return ret;
}

stringlist* connectedUserList(manager* usrmngr){
    int err=0;
    int p=0;
    stringlist* ret=NULL;
    int* kp;
    char* dp;
    int i=0;
    icl_entry_t* j;
    if(usrmngr==NULL) return ret;
    MUTEXLOCK(usrmngr->lockc);
    initializeStringList(&ret,(usrmngr->connected_user)->nentries,MAX_NAME_LENGTH);
    icl_hash_foreach((usrmngr->connected_user), i, j, kp, dp){
        err=err||(addString(ret,p,dp));
        ++p;
    }

    MUTEXUNLOCK(usrmngr->lockc);
    if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
    }
    return ret;
}

stringlist* registredUserList(manager* usrmngr){
    int err=0;
    int p=0;
    stringlist* ret=NULL;
    userdata* data;
    int i;
    char* key;
    icl_entry_t* j;
    if(usrmngr==NULL) return ret;
    for(i=0;i<NUMMUTEX;i++){
        MUTEXLOCK(usrmngr->lockr[i])};
    }
    initializeStringList(&ret,(usrmngr->registred_user)->nentries,MAX_NAME_LENGTH);
    for (i=0;i<(usrmngr->registred_user)->nbuckets; i++){
            for (j=(usrmngr->registred_user)->buckets[i];j!=NULL&&((key=j->key)!=NULL)&&((data=j->data)!=NULL);j=j->next){
                err=err ||(addString(ret,p,key));
                ++p;
            }
    }
    for(i=0;i<NUMMUTEX;i++){
        {MUTEXUNLOCK(usrmngr->lockr[i]);
    }
     if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
    }
    return ret;
}

int groupexist(manager* usrmngr, char* name){
    int result=0;
    int i=hash_pjw((void*) name)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockg[i]);
    if(icl_hash_find(usrmngr->groups,name)) result=1;
    else result=0;
    MUTEXUNLOCK(usrmngr->lockg[i]);
    return result;
}

int isingroup(manager* usrmngr, char* nickname,char* groupname){
    int result=0;
    groupdata* group;
    int i=hash_pjw((void*) groupname)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockg[i]);
    group=icl_hash_find(usrmngr->groups,groupname);
    if(group && icl_hash_find(group->users, nickname)) result=1;
    MUTEXUNLOCK(usrmngr->lockg[i]);
    return result;
}
void destroy(manager* usrmngr){
    icl_hash_destroy(usrmngr->registred_user, free,freeunregisterUser);
    icl_hash_destroy(usrmngr->connected_user, NULL,free);
    icl_hash_destroy(usrmngr->groups,free,freeGroup);
    for(int i=0;i<NUMMUTEX;i++){
        if((errno=pthread_mutex_lock(&usrmngr->lockr[i]))) perror("Acquisizione Mutex");
        if((errno=pthread_mutex_destroy(&usrmngr->lockr[i]))) perror("Cancellamento Mutex");
    }
    free(usrmngr->lockr);
    for(int i=0;i<NUMMUTEX;i++){
        if((errno=pthread_mutex_lock(&usrmngr->lockr[i]))) perror("Cancellamento Mutex");
        if((errno=pthread_mutex_destroy(&usrmngr->lockg[i]))) perror("Cancellamento Mutex");
    }
    free(usrmngr->lockg);
    if((errno=pthread_mutex_destroy(&usrmngr->lockc))) perror("Cancellamento Mutex");
}
