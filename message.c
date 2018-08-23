/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
 /** @file message.c
   * @author Francesco Pirrò 544539
   * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
 */


#include <message.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <config.h>
#include <ops.h>


/**
 * @function copymex
 * @brief crea un duplicato del messaggio allocandolo in memoria dinamica
 * @param mex messaggio da copiare
 * @return puntatore al messaggio copiato, NULL in caso di errore
*/
message_t* copymex(message_t mex){
    message_t* newmex=NULL;
    char* newbuff=NULL;
    newmex=malloc(sizeof(message_t));
    MEMORYCHECK(newmex);
    //alloca messagio
    setHeader(&(newmex->hdr),mex.hdr.op,mex.hdr.sender);
    //copia header
    newbuff=malloc(sizeof(char)*(mex.data.hdr.len+1));
    MEMORYCHECK(newbuff);
    memset(newbuff,'\0',(mex.data.hdr.len+1)*(sizeof(char)));
    strncpy(newbuff,mex.data.buf,(mex.data.hdr.len));
    //alloca e copia il buffer
    setData(&(newmex->data),mex.data.hdr.receiver,newbuff,mex.data.hdr.len);
    //copia l'header di data e inserisce il nuovo buffer
    return newmex;
}
