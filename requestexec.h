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
 * @brief procedura che registra un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void register_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function connect_op
 * @brief procedura che connette un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void connect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function posttxt_op
 * @brief procedura che manda un messaggio ad un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void posttxt_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function posttextall_op
 * @brief procedura che manda un messaggio a tutti gli utenti
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void posttextall_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function postfile_op
 * @brief procedura che invia un file all'utente 
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);

/**
 * @function getfile_op
 * @brief procedura che recupera un file inviato all'utente 
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function getprevmsgs_op
 * @brief procedura che invia la history dei messaggi all'utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void getprevmsgs_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function usrlist_op
 * @brief procedura che invia la lista degli utenti connessi al momento
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void usrlist_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function unregister_op
 * @brief procedura che deregistra un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void unregister_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function disconnect_op_op
 * @brief procedura che disconnette un utente
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void disconnect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function creategroup_op
 * @brief procedura che  crea un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void creategroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function addgroup_op
 * @brief procedura che aggiunge un utente ad un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void addgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function delfromgroup_op
 * @brief procedura che elimina un utente da un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void delfromgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);


/**
 * @function delgroup_op
 * @brief procedura che elimina un gruppo
 * @param fd file descriptor del socket su cui avviene la connessione
 * @param msg messaggio inviato al server sul socket fd
 * @param usrmngr gestore degli utenti
 * @param configurazione configurazione del server
 * @param chattystats statistiche del server
*/
void delgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats);

#endif //_requestexec_h_
