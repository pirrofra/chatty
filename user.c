/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** @file user.h
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
    return *(int*) a - *(int*)b; 
}

int initializeManager(manager** usrmngr, int dimr,int dimg, int max_user, int dim_history){
    int check=1;
    *usrmngr=malloc(sizeof(manager));
    MEMORYCHECK(*usrmngr);
    (*usrmngr)->lockr=calloc(NUMMUTEX,sizeof(pthread_mutex_t));
    MEMORYCHECK((*usrmngr)->lockr);
    (*usrmngr)->lockg=calloc(NUMMUTEX, sizeof(pthread_mutex_t));
    MEMORYCHECK((*usrmngr)->lockg)
    for(int i=0;i<NUMMUTEX;i++){
        SYSCALLCHECK(pthread_mutex_init(&((*usrmngr)->lockr[i]),NULL),"Inizializzazzione Mutex");
    }
    
    SYSCALLCHECK(pthread_mutex_init(&((*usrmngr)->lockc),NULL),"Inizializzazzione Mutex");
    
    for(int i=0;i<NUMMUTEX;i++){
        SYSCALLCHECK(pthread_mutex_init(&((*usrmngr)->lockg[i]),NULL),"Inizializzazzione Mutex");
    }
    (*usrmngr)->max_connected_user=max_user;
    (*usrmngr)->history_size=dim_history;
    (*usrmngr)->registred_user=icl_hash_create(dimr,hash_pjw,string_compare);
    MEMORYCHECK((*usrmngr)->registred_user);
    (*usrmngr)->connected_user=icl_hash_create(max_user,simpleHash, simpleCompare);
    MEMORYCHECK((*usrmngr)->connected_user);
    (*usrmngr)->groups=icl_hash_create(dimg,hash_pjw,string_compare);
    MEMORYCHECK((*usrmngr)->groups);
    return 0;
}

op_t registerUser(manager* usrmngr, char* nickname){
    int err=0;
    int i=hash_pjw((void*) nickname)%NUMMUTEX;
    char* newnick;
    op_t result;
    userdata* data;
    data=malloc(sizeof(userdata));
    MEMORYCHECK(data);
    newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
    pthread_mutex_lock(&(usrmngr->lockr[i]));
    err=icl_hash_insert(usrmngr->registred_user,newnick, data);
    if(err==-1) {
        result=OP_FAIL;
        free(newnick);
        free(data);
    }
    else if(err==0){
        result=OP_NICK_ALREADY;
        free(newnick);
        free(data);
    }
    else{
        data->fd=-1;
        err=initializeHistory(&(data->user_history),usrmngr->history_size);
        if(err==-1){
            icl_hash_delete(usrmngr->registred_user, nickname, free,free);
            result=OP_FAIL;
        }
        result=OP_OK;
    }
    pthread_mutex_unlock(&(usrmngr->lockr[i]));
    return result;
}

op_t connectUser(manager* usrmngr, char* nickname, int fd){ 
    int err=0;   
    int i=hash_pjw((void*) nickname)%NUMMUTEX;
    char* newnick;
    op_t result;
    userdata* data;
    newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    if(newnick==NULL) return OP_FAIL;
    memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
    pthread_mutex_lock(&(usrmngr->lockr[i]));
    data=icl_hash_find(usrmngr->registred_user, nickname);
    if(data==NULL){
        result=OP_NICK_UNKNOWN;
        free(newnick);
    }
    else if(data->fd!=-1) {
        result=OP_USR_ALREADY_CONNECTED;
        free(newnick);
    }
    else{
        pthread_mutex_lock(&(usrmngr->lockc));
        int* key=malloc(sizeof(int));
        if(usrmngr->max_connected_user>=(usrmngr->connected_user)->nentries) result=OP_TOO_MANY_CLIENT;
        else if(key!=NULL){
            *key=fd;
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
        else result=OP_FAIL;
        pthread_mutex_unlock(&(usrmngr->lockc));
    }
    pthread_mutex_unlock(&(usrmngr->lockr[i]));
    return result;
}

op_t unregisterUser(manager* usrmngr, char* nickname){
    userdata* data=NULL;
    op_t result;
    groupdata* gdata=NULL;
    int i=0;
    char* key=NULL;
    icl_entry_t* j=NULL;
    int h1=0;
    int h2=0;
    h2=hash_pjw((void*) nickname)%NUMMUTEX;
    pthread_mutex_lock(&(usrmngr->lockr[h2]));
    for (i=0;i<(usrmngr->groups)->nbuckets; i++){
            h1=i%NUMMUTEX;
            pthread_mutex_lock(&(usrmngr->lockg[h1]));
            for (j=(usrmngr->groups)->buckets[i];j!=NULL&&((key=j->key)!=NULL)&&((gdata=j->data)!=NULL);j=j->next){
                kick(gdata,nickname);
            }
            pthread_mutex_unlock(&(usrmngr->lockg[h1]));
    } 
    data=icl_hash_find(usrmngr->registred_user,nickname);
    if(data==NULL) result= OP_NICK_UNKNOWN;
    else{
        pthread_mutex_lock(&(usrmngr->lockc));
        icl_hash_delete(usrmngr->connected_user,&(data->fd),free,free);
        pthread_mutex_unlock(&(usrmngr->lockc));
        icl_hash_delete(usrmngr->registred_user, nickname,free, freeunregisterUser);
        result=OP_OK;
    }
    pthread_mutex_unlock(&(usrmngr->lockr[h2]));
    return result;
}

op_t disconnectUser(manager* usrmngr, int fd){
    int tmp=0;
    userdata* data;
    char* name;
    op_t result;
    pthread_mutex_lock(&(usrmngr->lockc));
    name=icl_hash_find(usrmngr->connected_user,&(fd));
    tmp=icl_hash_delete(usrmngr->connected_user,&(fd),free,NULL);
    pthread_mutex_unlock(&(usrmngr->lockc));
    if(tmp==0) result=OP_USR_NOT_CONNECTED;
    else if(tmp==-1) result=OP_FAIL;
    else{
        int i=hash_pjw((void*) name)%NUMMUTEX;
        pthread_mutex_lock(&(usrmngr->lockr[i]));
        data=icl_hash_find(usrmngr->registred_user,name);
        data->fd=-1;
        pthread_mutex_unlock(&(usrmngr->lockr[i]));
        free(name);
        result=OP_OK;
    }
    return result;
}

int storeMessage(manager* usrmngr, char* nickname ,message_t* msg){
    int result=0;
    int tmp=0;
    int i=hash_pjw((void*) nickname)%NUMMUTEX;
    userdata* data;
    pthread_mutex_lock(&(usrmngr->lockr[i]));
    data=icl_hash_find(usrmngr->registred_user, nickname);
    if(data==NULL) result=-3;
    else{
        tmp=addMessage(data->user_history, msg);
        if(tmp==-1)
            result=tmp;
        else result=data->fd;
    }
    pthread_mutex_unlock(&(usrmngr->lockr[i]));
    return result;
}



op_t createGroup(manager* usrmngr, char* creator, char* name){
    op_t result;
    groupdata* group;
    char* newname;
    int tmp;
    tmp=initializeGroup(&group,creator,GTABLEDIM);
    if(tmp==-1) result=OP_FAIL;
    else{
        newname=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
        if(newname==NULL) {
            freeGroup(group);
            result=OP_FAIL;
        }
        else{
            memset(newname,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
            strncpy(newname,name,MAX_NAME_LENGTH*sizeof(char));
            int h=0;
            h=hash_pjw((void*) name)%NUMMUTEX;
            pthread_mutex_lock(&(usrmngr->lockg[h]));
            tmp=icl_hash_insert(usrmngr->groups, newname, group);
            if(tmp==-1) {
                result=OP_FAIL;
                freeGroup(group);
                free(newname);
            }
            else if(tmp==0){
                result=OP_NICK_ALREADY;
                free(newname);
                freeGroup(group);
            }
            else result=OP_OK;
            pthread_mutex_unlock(&(usrmngr->lockg[h]));
            
        }
    }
    return result;
}

op_t addtoGroup(manager* usrmngr, char*nickname, char* groupname){
    op_t result;
    groupdata* group;
    userdata* data;
    int tmp=0;
    int h1=0;
    int h2;
    h1=hash_pjw((void*) groupname)%NUMMUTEX;
    h2=hash_pjw((void*) nickname)%NUMMUTEX;
    pthread_mutex_lock(&(usrmngr->lockr[h2]));
    data=icl_hash_find(usrmngr->registred_user,nickname);
    if(data!=NULL){
        pthread_mutex_lock(&(usrmngr->lockg[h1]));
        group=icl_hash_find(usrmngr->groups,groupname);
        if(group==NULL) result=OP_NICK_UNKNOWN;
        else{
            tmp=addMember(group, nickname);
            if(tmp==-1)result=OP_FAIL;
            else if(tmp==0) result=OP_USR_ALREADY_IN_GROUP;
            else result=OP_OK;
        }
        pthread_mutex_unlock(&(usrmngr->lockg[h1]));
    }
    else result=OP_NICK_UNKNOWN;
    pthread_mutex_unlock(&(usrmngr->lockr[h2]));
    return result;
}

op_t deletefromGroup(manager* usrmngr, char*nickname, char* groupname){
    op_t result;
    groupdata* group;
    int tmp=0;
    int h=0;
    h=hash_pjw((void*) groupname)%NUMMUTEX;
    pthread_mutex_lock(&(usrmngr->lockg[h]));
    group=icl_hash_find(usrmngr->groups,groupname);
    if(group==NULL) result=OP_NICK_UNKNOWN;
    else if (!string_compare((void*)group->admin,(void*)nickname)){
        result=deleteGroup(usrmngr,groupname,nickname);
    }
    else{
        tmp=kick(group,nickname);
        if(tmp==-1) result=OP_FAIL;
        else if(tmp==0) result=OP_NICK_UNKNOWN;
        else result=OP_OK;
    }
    pthread_mutex_unlock(&(usrmngr->lockg[h]));
    return result;
}

op_t deleteGroup(manager* usrmngr, char* groupname,char*nickname){
    groupdata* group;
    int tmp;
    op_t result;
    int h=0;
    h=hash_pjw((void*) groupname)%NUMMUTEX;
    pthread_mutex_lock(&(usrmngr->lockg[h]));
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
    pthread_mutex_unlock(&(usrmngr->lockg[h]));
    return result;
}

stringlist* userGroupList(manager* usrmngr, char* groupname){
    int err=0;
    int p=0;
    groupdata* group;
    stringlist* ret;
    int h=0;
    char* kp;
    char* dp;
    int i=0;
    icl_entry_t* j;
    h=hash_pjw((void*) groupname)%NUMMUTEX;
    pthread_mutex_lock(&(usrmngr->lockg[h]));
    group=icl_hash_find(usrmngr->groups,groupname);
    if(group==NULL) ret=NULL;
    else{
        err=initializeStringList(&ret,(group->users)->nentries,MAX_NAME_LENGTH);
        if(err) ret=NULL;
        else{
            icl_hash_foreach(group->users, i, j, kp, dp){
                err=err||(addString(ret,p,kp));
                ++p;
            }
        }
    }
    pthread_mutex_unlock(&(usrmngr->lockg[h]));
    if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
    }
    return ret;
}

stringlist* connectedUserList(manager* usrmngr){
    int err=0;
    int p=0;
    stringlist* ret;
    int* kp;
    char* dp;
    int i=0;
    icl_entry_t* j;
    pthread_mutex_lock(&(usrmngr->lockc));
    err=initializeStringList(&ret,(usrmngr->connected_user)->nentries,MAX_NAME_LENGTH);
    if(err) ret=NULL;
    else{
         icl_hash_foreach((usrmngr->connected_user), i, j, kp, dp){
         err=err||(addString(ret,p,dp));
         ++p;
        }
    }
    pthread_mutex_unlock(&(usrmngr->lockc));
    if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
    }
    return ret;
}

stringlist* registredUserList(manager* usrmngr){
    int err=0;
    int p=0;
    stringlist* ret;
    userdata* data;
    int i;
    char* key;
    icl_entry_t* j;
    for(i=0;i<NUMMUTEX;i++){
        pthread_mutex_lock(&(usrmngr->lockr[i]));
    }
    err=initializeStringList(&ret,(usrmngr->registred_user)->nentries,MAX_NAME_LENGTH);
    for (i=0;i<(usrmngr->registred_user)->nbuckets; i++){
            for (j=(usrmngr->registred_user)->buckets[i];j!=NULL&&((key=j->key)!=NULL)&&((data=j->data)!=NULL);j=j->next){
                err=err ||(addString(ret,p,key));
                ++p;
            }
    }
    for(i=0;i<NUMMUTEX;i++){
        pthread_mutex_unlock(&(usrmngr->lockr[i]));
    }
     if(ret!=NULL&&err){
        freeStringList(ret);
        ret=NULL;
    }      
    return ret;
}

