/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** @file groupdata.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/
#include<config.h>

void initializeGroup(groupdata** group,char* creator, int dim){
    char*newnick;
    *group=malloc(sizeof(groupdata));
    MEMORYCHECK(*group);
    newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,/0,(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,creator,MAX_NAME_LENGTH*sizeof(char));
    (*group)->admin=newnick;
    (*group)->users=icl_hash_create(dim,hash_pjw,string_compare);
    MEMORYCHECK((*group)->users);
}

int addMember(groupdata* group, char* nickname){
    char* newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,/0,(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
    return icl_hash_insert(group->users,char* newnick,char* newnick);
}

int kick(groupdata* group,char* nickname){
    return icl_hash_delete(group->users, nickname, free,NULL);
}

void freeGroup(void* group){
    free(group->creator);
    icl_hash_destroy(group->users,free,NULL);
} 
