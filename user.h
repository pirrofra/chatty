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
#include <message.h>
#include <icl_hash.h>
#include <history.h>
#include <ops.h>
#include <pthread.h>
#include <stringlist.h>


#ifndef _user_h_
#define _user_h_

/**
 * @struct userdata
 * @brief struttura per conservare informazioni utili sugli utenti.
 * @var user_history hisory dei messaggi collegati all'utente.
 * @var  fd file descriptor del client connesso con quel nickname, se -1 utente disconnesso
*/
typedef struct {
    history* user_history;
    int fd;
} userdata;

/**
 * @struct manager
 * @brief struttura dati per il gestore degli utenti. Mutua esclusione sui singoli bucket delle tabelle Hash. max_connected_user e history_size sono costanti.
 * @var registred_user hashtable usata per conservare dati sugli utenti registrati
 * @var connected hashtable usata per conservare dati sugli utenti connessi
 * @var groups hashtable usata per conservare dati sui gruppi creati
 * @var max_connected_user numero massimo di utenti che si possono avere connessi
 * @var history_size numero massimo di messaggi da salvare per utente
 * @var lockr array di mutex lock (uno per ogni bucket di registred_user)
 * @var lockc array di mutex lock (uno per ogni bucket di connected_user)
 * @var lockg array di mutex lock (uno per ogni bucket di groups)
 *
*/
typedef struct{
    icl_hash_t* registred_user;
    icl_hash_t* connected_user;
    icl_hash_t* groups;
    int max_connected_user;
    int history_size;
    pthread_mutex_t* lockr;
    pthread_mutex_t lockc;
    pthread_mutex_t* lockg;
} manager;

/**
 * @function initializeManager
 * @brief inizializza il gestore degli Utenti
 * @param usrmngr puntantore al manager da inizializzare
 * @param max_user massimo utenti connessi
 * @param dim_history numero massimo di messaggi salvabili
 * @return 0 operazione riuscita, -1 errore
*/
int initializeManager(manager* usrmngr, int max_user, int dim_history);


/**
 * @function registerUser
 * @brief registra un nuovo utente
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nickname dell'utente da registrare
 * @param fd socke del client da associare
 * @return op_t esito operazione
*/
op_t registerUser(manager* usrmngr, char* nickname,int fd);

/**
 * @function connectUser
 * @brief connette un client con un nickname
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nickname utente
 * @param fd socket del client da associare
 * @return op_t esito operazione
*/
op_t connectUser(manager* usrmngr, char* nickname, int fd);

/**
 * @function unregisterUser
 * @brief deregistra un utente
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nickname dell'utente da deregistrare
 * @return op_t esito operazione
*/
op_t unregisterUser(manager* usrmngr, char* nickname);

/**
 * @function disconnectUser
 * @brief disconnette utente  dal server
 * @param usrmngr puntatore al gestore degli utenti
 * @param fd file descriptor del client da disconnettere
 * @return op_t esito operazione
*/
op_t disconnectUser(manager* usrmngr, int fd);

/**
 * @function freeunregisterUser
 * @brief funzione che libera lo spazio occupato da data
 * @param data spazio da liberare
*/
void freeunregisterUser(void* data);

/**
 * @function simpleHash
 * @brief funzione hash per gli interi
 * @param key chiave di cui calcolare l'hash
 * @return il valore della funzione hash calcolato su key
*/
unsigned int simpleHash(void* key);

/**
 * @function simpleCompare
 * @brief semplice funzione compare
 * @param a primo valore intero da comparare
 * @param b secondo valore intero da comparare
 * @return a-b
*/
int simpleCompare(void* a, void* b);

/**
 * @function storeMessage
 * @brief conserva il messaggio nella history
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname utente che ha ricevuto il messaggio
 * @param msg messaggio spedito
 * @return -3 Nickname inesistente, -2 errore, fd dell'utente
*/
int storeMessage(manager* usrmngr, char* nickname ,message_t* msg);


op_t prevMessage(manager* usrmngr, char* nickname,history** newHistory);
/**
 * @function createGroup
 * @brief crea un nuovo gruppo
 * @param usrmngr puntantore al gestore degli utenti
 * @param creator nickname del creatore
 * @param name nome del gruppo
 * @return op_t esito operazione
*/
op_t createGroup(manager* usrmngr, char* creator, char* name);

/**
 * @function addtoGroup
 * @brief aggiunge un utente a un gruppo
 * @param usrmngr puntantore al gestore degli utenti
 * @param nickname utente da aggiungere al gruppo
 * @param groupname nome del gruppo
 * @return op_t esito operazione
*/
op_t addtoGroup(manager* usrmngr, char*nickname, char* groupname);

/**
 * @function deletefromGroup
 * @brief elimina un utente dal gruppo
 * @param usrmngr puntatore al gestore degli utenti
 * @param nickname nome dell'utente da bannare
 * @param groupname nome del gruppo
 * @return op_t esito dell'operazione
*/
op_t deletefromGroup(manager* usrmngr, char*nickname, char* groupname);

/**
 * @function deleteGroup
 * @brief elimina un intero gruppo
 * @param usrmngr puntatore al gestore degli utenti
 * @param groupname nome del gruppo da eliminare
 * @param nickname nickname dell'utente che richiede l'operazione
 * @return op_t esito dell'operazione
*/
op_t deleteGroup(manager* usrmngr, char* nickname,char* groupname);

/**
 * @function userGroupList
 * @brief ritorna la lista degli utenti in un gruppo
 * @param usrmngr puntatore al gestore degli utenti
 * @param groupname nome del gruppo di cui restituire gli utenti
 * @return lista degli utenti iscritti al gruppo
*/
stringlist* userGroupList(manager* usrmngr, char* groupname);

/**
 * @function registredUserList
 * @brief ritorna la lista degli utenti registrati
 * @param usrmngr puntatore al gestore degli utenti
 * @return lista degli utenti registrati
*/
stringlist* registredUserList(manager* usrmngr);

/**
 * @function connectedUserList
 * @brief ritorna la lista degli utenti connessi
 * @param usrmngr puntatore al gestore degli utenti
 * @return lista degli utenti connessi
*/
stringlist* connectedUserList(manager* usrmngr);


int groupexist(manager* usrmngr, char* name);

int isingroup(manager* usrmngr, char* nickname,char* groupname);

void destroy(manager* usrmngr);

#endif //_user_h_
