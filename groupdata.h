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

/**
 * @struct groupdata
 * @brief struttura dati per gestire un gruppo
 * @var name nome del gruppo
 * @var users puntatore alla tabella hash che contiene i nomi degli utenti
*/
typedef struct{
    char* admin;
    icl_hash_t* users;
}groupdata;
 
 
/**
 * @function initializeGroup  
 * @brief inizializza il gruppo
 * @param group puntatore al gruppo da inizializzare
 * @param creator creatore del gruppo
 * @param dim dimensione della tabella con i nickname
 * @return 0 successo, -1 errore
*/
int initializeGroup(groupdata** group,char* creator, int dim);
 
/**
 * @function addMember
 * @brief aggiunge un membro al gruppo
 * @param group gruppo a cui aggiungere il membro
 * @param nickname membro da aggiungere
 * @return 1 successo, 0 membro già esistente, -1 errore
*/
int addMember(groupdata* group, char* nickname);

/**
 * @function kick
 * @brief elimina un membro dal gruppo
 * @param group gruppo da cui eliminare il membro
 * @param nickname membro da eliminare
 * @return 1 successo, 0 membro non presente, -1 errore
*/
int kick(groupdata* group,char* nickname);

/**
 * @function freeGroup
 * @brief libera la struttura dati del gruppo
 * @param group gruppo da eliminare
*/
void freeGroup(void* group);

