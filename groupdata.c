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
#include <groupdata.h>
#include<stdlib.h>
#include<string.h>

/**
 * @function initializeGroup
 * @brief inizializza il gruppo
 * @param group puntatore al gruppo da inizializzare
 * @param creator creatore del gruppo
 * @param dim dimensione della tabella con i nickname
*/
void initializeGroup(groupdata** group,char* creator, int dim){
    char*newnick;
    *group=malloc(sizeof(groupdata));
    MEMORYCHECK(*group);
    //alloca la struttra groupdata

    newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,creator,MAX_NAME_LENGTH*sizeof(char));
    //copio la stringa creator in newnick

    (*group)->admin=newnick;
    (*group)->users=icl_hash_create(dim,hash_pjw,string_compare); //crea la tabella che conterrà gli utenti inseriti nel gruppo
    MEMORYCHECK((*group)->users);
}

/**
 * @function addMember
 * @brief aggiunge un membro al gruppo
 * @param group gruppo a cui aggiungere il membro
 * @param nickname membro da aggiungere
 * @return 1 successo, 0 membro già esistente, -1 errore
*/
int addMember(groupdata* group, char* nickname){
    char* newnick=malloc((MAX_NAME_LENGTH+1)*sizeof(char));
    MEMORYCHECK(newnick);
    memset(newnick,'\0',(MAX_NAME_LENGTH+1)*sizeof(char));
    strncpy(newnick,nickname,MAX_NAME_LENGTH*sizeof(char));
    //copia nickname in newnick

    return icl_hash_insert(group->users,newnick,newnick); //inserisce newnick nella tabella degli utenti iscritti al gruppo
}

/**
 * @function kick
 * @brief elimina un membro dal gruppo
 * @param group gruppo da cui eliminare il membro
 * @param nickname membro da eliminare
 * @return 1 successo, 0 membro non presente, -1 errore
*/
int kick(groupdata* group,char* nickname){
    return icl_hash_delete(group->users, nickname, free,NULL);
}

/**
 * @function freeGroup
 * @brief libera la struttura dati del gruppo
 * @param group gruppo da eliminare
*/
void freeGroup(void* g){
    groupdata* group=(groupdata*)g;
    free(group->admin); //libera la stringa admin
    icl_hash_destroy(group->users,free,NULL); //elimina la tabella hash
    free(group); //libera il resto della struttura dati
}
