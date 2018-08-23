/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file history.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include <history.h>
#include <config.h>
#include <stdlib.h>


#ifndef _history_c_
#define _history_c_

/**
 * @function initializeHistory
 * @brief procedura per inizializzare la history
 * @param storia puntatore alla history da inizializzare
 * @param size dimensione della history
*/
void initializeHistory(history** storia, int size){
    *storia=malloc(sizeof(history));
    MEMORYCHECK(*storia);
    memset(*storia,0,sizeof(history));
    //alloca la struttura history e la inizializza

    (*storia)->data=malloc(size*sizeof(message_t*));
    MEMORYCHECK((*storia)->data);
    memset((*storia)->data,0,size*sizeof(message_t*));
    //alloca l'array di messaggi e lo inizializza

    (*storia)->pending=malloc(size*sizeof(int));
    MEMORYCHECK((*storia)->pending);
    memset((*storia)->pending,0,size*sizeof(int));
    //alloca l'array per segnare i messaggi spediti e non

    (*storia)->size=size;
    (*storia)->first=0;
    (*storia)->last=size-1;
    (*storia)->nel=0;
}

/**
 * @function addMessage
 * @brief aggiunge un messaggio alla history
 * @param storia puntatore alla history in cui inserire
 * @param mex messaggio da inserire
 * @param fd -1 se è offline, altr. è online
 * @return 0 successo, -1 errore
*/
int addMessage(history* storia, message_t* mex,int fd){
    if(mex==NULL || storia==NULL) return -1;
    int i=(storia->last+1)%storia->size; //calcola la posizione nel quale inserire
    if(storia->data[i]!=NULL){ //se esiste già un messaggio in quella posizione, lo cancella e segna il messaggio più vecchio come i+1;
        free(storia->data[i]->data.buf);
        free(storia->data[i]);
        storia->first=(i+1)%storia->size;
    }

    if(storia->nel!=storia->size)++storia->nel; //la history non è ancora piena, aumenta il numero dei messaggi inseriti

    if(fd==-1) storia->pending[i]=-1;
    else storia->pending[i]=0;
    //segna se è stato spedito o meno

    storia->data[i]=mex;
    storia->last=i;
    //inserisce il nuovo messaggio
    return 0;
}

/**
 * @fuction copyHistory
 * @brief copia la storia dei messaggi, ordinandoli dal meno recente al più recente
 * @param storia history da copiare
 * @return NULL in caso di errore, puntatore alla nuova history in caso di successo
*/
history* copyHistory(history* storia){
    history* newHistory = NULL; //nuova history in cui copiare il contenuto di storia
    if(storia){
        initializeHistory(&newHistory, storia->size);
        int i=storia->first;
        int j=0;
        int el=0;
        while(el<storia->nel){
            if(storia->data[i]){
                newHistory->data[j]=copymex(*storia->data[i]);
                newHistory->pending[j]=storia->pending[i];
                newHistory->last=j;
                j=(j+1)%newHistory->size;
                ++(newHistory->nel);
            }
            i=(i+1)%newHistory->size;
            ++el;
        }
        //copia i messaggi e li inserisce in ordine in newHistory
    }

    return newHistory;
}

/**
 * @function resetPending
 * @brief segna tutti i messaggi nella history come spediti
 * @param storia history da aggiornare
*/
void resetPending(history* storia){
    memset(storia->pending,0,(storia->size)*sizeof(int)); //setta di nuovo tutto a 0
}

/**
 * @function freeHistory
 * @brief libera lo spazio allocato dalla History
 * @param storia puntatore alla history da liberare
*/
void freeHistory(history* storia){
    if(storia!=NULL){
        int i=storia->first;
        int el=0;
        while(el<storia->nel){
            if(storia->data[i]){
                free(storia->data[i]->data.buf);
                free(storia->data[i]);
                //libero buffer e messaggio di quelli inseriti nella history
            }
            i=(i+1)%storia->size;
            ++el;
        }
        free(storia->data);
        free(storia->pending);
        free(storia);
        //libero gli array e il resto della struttura
    }
}

#endif //_history_c_
