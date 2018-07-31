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
    if(dim<=0 || str_dim <=0) return -1;
    (*strlst)=malloc(sizeof(stringlist));
    MEMORYCHECK(*strlst);
    (*strlst)->array=malloc(dim*sizeof(char*));
    MEMORYCHECK((*strlst)->array);
    memset((*strlst)->array,0,dim*sizeof(char*));
    for(int i=0;i<dim;i++){
        (*strlst)->array[i]=malloc((str_dim+1)*sizeof(char)+1);
        MEMORYCHECK((*strlst)->array[i]);
        memset((*strlst)->array[i],'\0',(str_dim+1)*sizeof(char));
    }
    (*strlst)->lenght=dim;
    (*strlst)->str_dim=str_dim;
}

int addString(stringlist* strlst,int i, char* stringa){
    if(i>=strlst->lenght||strlst==NULL||stringa==NULL) return -1;
    strncpy(strlst->array[i],stringa,(strlst->str_dim+1));
    return 0;
}

char* transform(stringlist* strlst){
    if(strlst==NULL) return NULL;
    char* ret;
    char separatore[1]={' '};
    ret=malloc(strlst->lenght*(strlst->str_dim+1)*sizeof(char));
    MEMORYCHECK(ret);
    memset(ret,'\0',strlst->lenght*(strlst->str_dim+1));
    for(int i=0;i<strlst->lenght;i++){
        strncat(ret,strlst->array[i],(strlst->str_dim));
        strncat(ret,separatore,sizeof(char));
    }
    ret=realloc(ret,(strlen (ret)+1)*sizeof(char));
    
    return ret;
}

void freeStringList(stringlist* strlst){
    if(strlst!=NULL){
        for(int i=0;i<strlst->lenght;i++) free(strlst->array[i]);
        free(strlst->array);
        free(strlst);
    }
}

