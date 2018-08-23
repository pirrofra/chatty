/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file stringlist.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include <stringlist.h>
#include <string.h>
#include <stdlib.h>

/**
 * @function initializeStringList
 * @brief inizializza stringlist
 * @param strlst stringlist da inizializzare
 * @param dim dimensione dell'array
 * @param str_dim dimensione delle stringhe
*/
void initializeStringList(stringlist** strlst, int dim, int str_dim){
        (*strlst)=malloc(sizeof(stringlist));
        MEMORYCHECK(*strlst);
        //alloca la memoria per stringlist
        if(dim<=0|| str_dim<=0) (*strlst)->str=NULL;
        else {
            (*strlst)->str=malloc(dim*sizeof(char)*(str_dim+1));
            MEMORYCHECK((*strlst)->str);
            memset((*strlst)->str,'\0',dim*sizeof(char)*(str_dim+1));
            //alloca e inizializza a 0 la stringa
        }
        (*strlst)->lenght=dim;
        (*strlst)->str_dim=str_dim;
}

/**
 * @function addString
 * @brief aggiunge la stringa alla lista
 * @param strlst stringlist a cui aggiungere
 * @param i posizione
 * @param stringa stringa da aggiungere
 * @return 0 successo, -1 errore
*/
int addString(stringlist** strlst,int i, char* stringa){
    char* tmp;
    char* tmpStringList=NULL;
    if(i>(*strlst)->lenght||strlst==NULL||stringa==NULL) return -1; //parametri non validi
    if((*strlst)->str==NULL) return 0; //stringa non inizializzata correttamente
    if(i==(*strlst)->lenght){
        //stringa puo' essere aggiunta in coda.
        if((tmpStringList=realloc((*strlst)->str,(((*strlst)->lenght)+1)*((*strlst)->str_dim)))){
            (*strlst)->str=tmpStringList;
            ++(*strlst)->lenght;
        }
        else exit(EXIT_FAILURE);
        //realloc di di strlst->str in modo da avere una posizione in più
    }
    tmp=((*strlst)->str)+(i*((*strlst)->str_dim+1));
    memset(tmp,'\0',(*strlst)->str_dim*sizeof(char));
    strncat(tmp,stringa,((*strlst)->str_dim));
    //aggiunge stringa alla posizione richiesta
    return 0;
}

/**
 * @function freeStringList
 * @brief libera la stringlist
 * @param strlst stringlist da liberare
*/
void freeStringList(stringlist* strlst){
    if(strlst!=NULL){
        if(strlst->str) free(strlst->str);
        free(strlst);
        //libera memoria allocata
    }
}
