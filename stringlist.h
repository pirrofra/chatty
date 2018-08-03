/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/** @file stringlist.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#ifndef _stringlist_h_
#define _stringlist_h_

typedef struct{
    int lenght;
    int str_dim;
    char* str;
} stringlist;

/**
 * @function initializeStringList
 * @brief inizializza stringlist
 * @param strlst stringlist da inizializzare
 * @param dim dimensione dell'array
 * @param str_dim dimensione delle stringhe
*/
void initializeStringList(stringlist** strlst, int dim, int str_dim);

/**
 * @function addString
 * @brief aggiunge la stringa alla lista
 * @param strlst stringlist a cui aggiungere
 * @param i posizione 
 * @param stringa stringa da aggiungere
 * @return 0 successo, -1 errore
*/
int addString(stringlist* strlst, int i, char* stringa);


/**
 * @function freeStringList
 * @brief libera la stringlist
 * @param strlst stringlist da liberare
*/
void freeStringList(stringlist* strlst);

#endif //_stringlist_h_
