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
    fseek(fd,0,SEEK_END);
    size=ftell(fd);
    rewind(fd);
    line=malloc((size+1)*sizeof(char));
    MEMORYCHECK(line);
    memset(line,'\0',(size+1)*sizeof(char));
    configurazione=initializeConfig();
    MEMORYCHECK(configurazione);
    s1=malloc(15*sizeof(char));
    MEMORYCHECK(s1);
    memset(s1,'\0',15*sizeof(char));
    while(fgets(line,size*sizeof(char),fd)!=NULL){
        if(line[0]!='#' && strlen(line)>1){
            int len;
            char* s2;
            len=strlen(line);
            s2=malloc(len*sizeof(char));
            MEMORYCHECK(s2);
            memset(s2,'\0', len*sizeof(char));
            sscanf(line,"%s = %s",s1,s2);
            if(strlen(s1)==0 || strlen(s2)==0) continue;
            else if(strcmp(s1,"UnixPath")==0){
                char* path;
                path=malloc((strlen(s2)+1)*sizeof(char));
                MEMORYCHECK(path);
                memset(path,'\0',(strlen(s2)+1)*sizeof(char));
                strncpy(path,s2,strlen(s2)*sizeof(char));
                configurazione->UnixPath=path;

            }
            else if(strcmp(s1,"DirName")==0){
               char* path;
                path=malloc((strlen(s2)+2)*sizeof(char));
                MEMORYCHECK(path);
                memset(path,'\0',(strlen(s2)+2)*sizeof(char));
                strncpy(path,s2,strlen(s2)*sizeof(char));
                if(path[strlen(path)-1]!='/') strncat(path,"/",sizeof(char));
                configurazione->DirName=path;

            }
            else if(strcmp(s1,"StatFileName")==0){
                char* path;
                path=malloc((strlen(s2)+1)*sizeof(char));
                MEMORYCHECK(path);
                memset(path,'\0',(strlen(s2)+1)*sizeof(char));
                strncpy(path,s2,strlen(s2)*sizeof(char));
                configurazione->StatFileName=path;

            }
            else if(strcmp(s1,"MaxConnections")==0)configurazione->MaxConnections=(strtoul(s2,NULL,10)*sizeof(char));
            else if(strcmp(s1,"MaxFileSize")==0) configurazione->MaxFileSize=(strtoul(s2,NULL,10)*1024);
            else if(strcmp(s1,"MaxMsgSize")==0) configurazione->MaxMsgSize=strtoul(s2,NULL,10);
            else if(strcmp(s1, "MaxHistMsgs")==0) configurazione->MaxHistMsgs=strtoul(s2,NULL,10);
            else if(strcmp(s1,"ThreadsInPool")==0) configurazione->ThreadsInPool=strtoul(s2,NULL,10);
            free(s2);
        }
    }
    fclose(fd);
    free(line);
    free(s1);
    if(checkC(configurazione)==0){
        freeC(configurazione);
        printf("File Configurazione Incompleto\n");
        return NULL;
    }
    return configurazione;
}

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

int checkC(configs* configurazione){
    return (configurazione->UnixPath!=NULL)&&(configurazione->DirName!=NULL)&&(configurazione->StatFileName!=NULL)&&(configurazione->MaxConnections!=0)&&(configurazione->MaxFileSize!=0)&&(configurazione->MaxMsgSize!=0)&&(configurazione->MaxHistMsgs!=0);
}

void freeC(configs* configurazione){
    if(configurazione->UnixPath!=NULL) free(configurazione->UnixPath);
    if(configurazione->DirName!=NULL) free(configurazione->DirName);
    if(configurazione->StatFileName!=NULL) free(configurazione->StatFileName);
    free(configurazione);
}
