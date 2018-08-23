/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 *
 */
/** @file fileconfig.c
  * @author Francesco Pirrò 544539
  * si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
*/

#include<fileconfig.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<config.h>

/**
 * @function readConfig
 * @param path path del file di configurazione
 * @return NULL errore, puntatore a una struttra configs in caso di successo
*/
configs* readConfig(char* path){
    configs* configurazione;
    char* line;
    char* s1;
    FILE* fd;
    int size=0;
    fd=fopen(path,"r");
    if(fd==NULL){
        perror("Apertura File");
        return NULL;
    }
    //apre il file di Configurazione

    fseek(fd,0,SEEK_END);
    size=ftell(fd);
    rewind(fd);
    //calcola la dimensione del file

    line=malloc((size+1)*sizeof(char));
    MEMORYCHECK(line);
    memset(line,'\0',(size+1)*sizeof(char));
    //alloca la linea con la dimensione massima di tutto il file

    configurazione=initializeConfig(); //inizializzo la struttura dati per salvare i dati
    MEMORYCHECK(configurazione);

    s1=malloc(15*sizeof(char));
    MEMORYCHECK(s1);
    memset(s1,'\0',15*sizeof(char));
    //s1 è il primo frammento della linea, prima di =

    while(fgets(line,size*sizeof(char),fd)!=NULL){ //legge fino al primo newline
        if(line[0]!='#' && strlen(line)>1){ //ignoro se il primo carattere è un # o la linea è vuota
            int len;
            char* s2;
            len=strlen(line);
            s2=malloc(len*sizeof(char));
            MEMORYCHECK(s2);
            memset(s2,'\0', len*sizeof(char));
            //s2 è il secondo frammento della linea, dopo =

            sscanf(line,"%s = %s",s1,s2); //sscanf per fare da parser alla linea letta
            if(strlen(s1)==0 || strlen(s2)==0) continue; //sscanf non ha trovato nulla

            else if(strcmp(s1,"UnixPath")==0){
                char* path;
                path=malloc((strlen(s2)+1)*sizeof(char));
                MEMORYCHECK(path);
                memset(path,'\0',(strlen(s2)+1)*sizeof(char));
                strncpy(path,s2,strlen(s2)*sizeof(char));
                configurazione->UnixPath=path;
                //s1 è UnixPath, quindi s2 contiene il path da salvare

            }
            else if(strcmp(s1,"DirName")==0){
               char* path;
                path=malloc((strlen(s2)+2)*sizeof(char));
                MEMORYCHECK(path);
                memset(path,'\0',(strlen(s2)+2)*sizeof(char));
                strncpy(path,s2,strlen(s2)*sizeof(char));
                if(path[strlen(path)-1]!='/') strncat(path,"/",sizeof(char)); //se il carattere finale non è un slash, lo aggiunge
                configurazione->DirName=path;
                //s1 è DirName, quindi s2 contiene il path da salvare

            }
            else if(strcmp(s1,"StatFileName")==0){
                char* path;
                path=malloc((strlen(s2)+1)*sizeof(char));
                MEMORYCHECK(path);
                memset(path,'\0',(strlen(s2)+1)*sizeof(char));
                strncpy(path,s2,strlen(s2)*sizeof(char));
                configurazione->StatFileName=path;
                //s1 è StatFileName, quindi s2 contiene il path da salvare

            }
            else if(strcmp(s1,"MaxConnections")==0)configurazione->MaxConnections=(strtoul(s2,NULL,10)*sizeof(char));
            else if(strcmp(s1,"MaxFileSize")==0) configurazione->MaxFileSize=(strtoul(s2,NULL,10)*1024);
            else if(strcmp(s1,"MaxMsgSize")==0) configurazione->MaxMsgSize=strtoul(s2,NULL,10);
            else if(strcmp(s1, "MaxHistMsgs")==0) configurazione->MaxHistMsgs=strtoul(s2,NULL,10);
            else if(strcmp(s1,"ThreadsInPool")==0) configurazione->ThreadsInPool=strtoul(s2,NULL,10);
            //s2 contiene un intero salvato come stringa, lo converte e lo salva

            free(s2); //libero s2
        }
    }
    fclose(fd);
    free(line);
    free(s1);
    //libera la memoria allocata e chiude il file

    if(checkC(configurazione)==0){ //assicura che configurazione sia completa e contenga valori validi
        freeC(configurazione);
        printf("File Configurazione Incompleto\n");
        return NULL;
    }
    return configurazione;
}

/*
 * @function initializeConfig
 * @brief inizializza una stuttura configs
 * @return puntatore alla struttura configs
*/
configs* initializeConfig(){
    configs* ret;
    ret=malloc(sizeof(configs));
    MEMORYCHECK(ret);
    ret->UnixPath=NULL;
    ret->DirName=NULL;
    ret->StatFileName=NULL;
    ret->MaxConnections=0;
    ret->MaxFileSize=0;
    ret->MaxMsgSize=0;
    ret->MaxHistMsgs=0;
    return ret;
}


/**
 * @function checkC
 * @brief controlla che tutte le informazioni necessarie siano state inserite in configs
 * @return 1 se configs è completo, 0 altrimenti
*/
int checkC(configs* configurazione){
    return (configurazione->UnixPath!=NULL)&&(configurazione->DirName!=NULL)&&(configurazione->StatFileName!=NULL)&&(configurazione->MaxConnections>0)&&(configurazione->MaxFileSize>0)&&(configurazione->MaxMsgSize>0)&&(configurazione->MaxHistMsgs>0);
}

/**
 * @function freeC
 * @brief dealloca la struttura dati configs
*/
void freeC(configs* configurazione){
    if(configurazione->UnixPath!=NULL) free(configurazione->UnixPath);
    if(configurazione->DirName!=NULL) free(configurazione->DirName);
    if(configurazione->StatFileName!=NULL) free(configurazione->StatFileName);
    //libera stringhe
    free(configurazione);
    //libera il resto della struttura
}
