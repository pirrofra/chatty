/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file requestexec.h
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include<fileconfig.h>
#include<stats.h>
#include<message.h>
#include<user.h>

#ifndef _requestexec_h_
#define _requestexec_h_

/**
 * @function register_op
 * @brief funzione che registra un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int register_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);

/**
 * @function connect_op
 * @brief funzione che connette un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int connect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);

/**
 * @function copymex
 * @brief funzione che copia un messaggio;
 * @param mex messaggio da copiare
 * @return NULL in caso di fallimento, o un puntatore ad una copia di mex
*/
message_t* copymex(message_t mex);

/**
 * @function posttxt_op
 * @brief funzione che notifica l'arrivo di un messaggio ad un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return op_t estio dell'operazione
*/
op_t notifymex(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);

/**
 * @function posttxt_op
 * @brief funzione che manda un messaggio ad un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int posttxt_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function posttextall_op
 * @brief funzione che manda un messaggio a tutti gli utenti
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int posttextall_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function postfile_op
 * @brief funzione che invia un file all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);

/**
 * @function getfile_op
 * @brief funzione che recupera un file inviato all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function getprevmsgs_op
 * @brief funzione che invia la history dei messaggi all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int getprevmsgs_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function usrlist_op
 * @brief funzione che invia la lista degli utenti connessi al momento
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int usrlist_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function unregister_op
 * @brief funzione che deregistra un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int unregister_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function disconnect_op_op
 * @brief funzione che disconnette un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int disconnect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function creategroup_op
 * @brief funzione che  crea un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int creategroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function addgroup_op
 * @brief funzione che aggiunge un utente ad un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int addgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function delfromgroup_op
 * @brief funzione che elimina un utente da un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int delfromgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);


/**
 * @function delgroup_op
 * @brief funzione che elimina un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int delgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);

/**
 * @function execute
 * @brief funzione che legge un messaggio inviato al server sul socket fd e ne esegue la richiesta
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
 * @return 0 se la connessione è ancora attiva, -1 se il client si è disconesso.
*/
int execute(int fd, manager* usrmngr, configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock);

#endif //_requestexec_h_
