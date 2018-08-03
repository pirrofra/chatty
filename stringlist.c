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

void initializeStringList(stringlist** strlst, int dim, int str_dim){
    if(dim<=0||str_dim<=0) *strlst=NULL;    
    else{
        (*strlst)=malloc(sizeof(stringlist));
        MEMORYCHECK(*strlst);
        (*strlst)->str=malloc(dim*sizeof(char)*(str_dim+1));
        MEMORYCHECK((*strlst)->str);
        memset((*strlst)->str,'\0',dim*sizeof(char)*(str_dim+1));
        (*strlst)->lenght=dim;
        (*strlst)->str_dim=str_dim;
    }
}

int addString(stringlist* strlst,int i, char* stringa){
    char* tmp;
    if(i>=strlst->lenght||strlst==NULL||stringa==NULL) return -1;
    tmp=strlst->str+i*(strlst->str_dim+1);
    strncpy(tmp,stringa,(strlst->str_dim+1));
    return 0;
}

void freeStringList(stringlist* strlst){
    if(strlst!=NULL){
        free(strlst->str);
        free(strlst);
    }
}

