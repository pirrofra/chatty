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
#include<requestexec.h>
#include<connections.h>
#include<stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>




int register_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    stringlist* users;
    op_t result=OP_FAIL;
    result=registerUser(usrmngr,msg.hdr.sender,fd);
    setHeader(&(reply.hdr),result,"");
    if(result==OP_OK){
        users=connectedUserList(usrmngr);
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->lenght);
        updusers(chattystats,1);
        updonline(chattystats,1);
    }
    else{
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
    }
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    if(result==OP_OK) freeStringList(users);
    return err;
}


int connect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    stringlist* users=NULL;
    op_t result=OP_FAIL;
    result=connectUser(usrmngr,msg.hdr.sender,fd);
    setHeader(&(reply.hdr),result,"");
    if(result==OP_OK)users=connectedUserList(usrmngr);
    if(users){
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->lenght);
        updonline(chattystats,1);
    }
    else {
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
    }
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    if(result==OP_OK) freeStringList(users);
    return err;
}


op_t notifymex(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    op_t result=OP_FAIL;
    int fd_receiver;
    stringlist* users;
    message_t* notify;
    if (msg.hdr.op==TXT_MESSAGE && msg.data.hdr.len>configurazione->MaxMsgSize) result=OP_MSG_TOOLONG;
    else if(msg.hdr.op==FILE_MESSAGE && msg.data.hdr.len>configurazione->MaxFileSize) result= OP_MSG_TOOLONG;
    else if(!groupexist(usrmngr,msg.data.hdr.receiver)){
        notify=copymex(msg);
        fd_receiver=storeMessage(usrmngr,msg.data.hdr.receiver,notify);
        if(fd_receiver==-3) {
            result=OP_NICK_UNKNOWN;
            free((notify->data).buf);
            free(notify);
        }
        else if(fd_receiver==-2) {
            result=OP_FAIL;
            free((notify->data).buf);
            free(notify);
        }
        else if(fd_receiver>-1){
            MUTEXLOCK(*lock);
            if(sendRequest(fd_receiver,&msg)<=0) printf("Errore Comunicazione");
            MUTEXUNLOCK(*lock);
            if(msg.hdr.op==TXT_MESSAGE) updelivered(chattystats,1);
        }

        if(msg.hdr.op==TXT_MESSAGE) {
            updndelivered(chattystats,1);
            result=OP_OK;
        }
        else if(msg.hdr.op==FILE_MESSAGE) {
            updnfile(chattystats,1);
            result=OP_OK;
        }
    }
    else if(isingroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver)){
        char* tmp;
        users=userGroupList(usrmngr,msg.data.hdr.receiver);
        tmp=users->str;
        for(int i=0;i<users->lenght;i++){
                notify=copymex(msg);
                fd_receiver=storeMessage(usrmngr,tmp,notify);
                if(msg.hdr.op==FILE_MESSAGE)updnfile(chattystats,1);
                if(fd_receiver==-1 && msg.hdr.op==TXT_MESSAGE) updndelivered(chattystats,1);
                else if(fd_receiver>-1){
                    MUTEXLOCK(*lock);
                    if(sendRequest(fd_receiver,&msg)<=0) printf("Errore Comunicazione");
                    MUTEXUNLOCK(*lock);
                    if(msg.hdr.op==TXT_MESSAGE)updelivered(chattystats,1);
                }
                else{
                    free((notify->data).buf);
                    free(notify);
                }

            tmp+=(users->str_dim+1);
        }
        freeStringList(users);
        result=OP_OK;
    }
    else {
        result=OP_NICK_UNKNOWN;
        upderrors(chattystats,1);
   }


   if(result!=OP_OK){
       upderrors(chattystats,1);
   }
    return result;
}

int posttxt_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    msg.hdr.op=TXT_MESSAGE;
    result=notifymex(fd,msg,usrmngr,configurazione,chattystats,lock);
    setHeader(&(reply.hdr),result,"");
    MUTEXLOCK(*lock);
    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int posttextall_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    stringlist* users;
    op_t result=OP_FAIL;
    char* tmp;
    message_t* notify=NULL;
    message_t reply;
    int fd_receiver;
    msg.hdr.op=TXT_MESSAGE;
    if (msg.data.hdr.len>configurazione->MaxMsgSize) {
        result=OP_MSG_TOOLONG;
        upderrors(chattystats, 1);
    }
    else{
        users=registredUserList(usrmngr);
        tmp=users->str;
        for(int i=0;i<users->lenght;i++){
            if(strcmp(msg.hdr.sender,tmp)){
                notify=copymex(msg);
                fd_receiver=storeMessage(usrmngr,tmp,notify);
                if(fd_receiver==-1) updndelivered(chattystats,1);
                else if(fd_receiver>-1){
                    MUTEXLOCK(*lock);
                    if(sendRequest(fd_receiver,&msg)<=0) err=-1;
                    MUTEXUNLOCK(*lock);
                    updelivered(chattystats,1);
                }
                else{
                    free((notify->data).buf);
                    free(notify);
                }
            }
            tmp+=(users->str_dim+1);
        }
        freeStringList(users);
        result=OP_OK;
    }
    setHeader(&(reply.hdr),result,"");
    MUTEXLOCK(*lock);
    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}


int postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    op_t result=OP_FAIL;
    int byte_wrote=0;
    message_t reply;
    message_data_t file;
    int newFile;
    readData(fd,&file);
    msg.hdr.op=FILE_MESSAGE;
    if(file.hdr.len>(configurazione->MaxFileSize)){
        result=OP_MSG_TOOLONG;
        upderrors(chattystats,1);
    }
    else{
        int dim=strlen(configurazione->DirName)+msg.data.hdr.len;
        char* completepath;
        completepath=malloc(sizeof(char)*(dim+1));
        MEMORYCHECK(completepath);
        memset(completepath,'\0',(dim+1)*sizeof(char));
        strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
        strncat(completepath,msg.data.buf,msg.data.hdr.len);

        if((newFile=open(completepath,O_WRONLY| O_CREAT,00700))==-1)
            perror("Apertura del File");

        else{
            char* position=file.buf;
            int len=file.hdr.len;
            while(len>0){
                byte_wrote=write(newFile,file.buf, len);
                if(byte_wrote<0){
                    len=0;
                    perror("scrittura file");
                }
                len-=byte_wrote;
                position+=byte_wrote;

            }
            if(byte_wrote<0){
                result=OP_FAIL;
                upderrors(chattystats,1);
            }

            else result=notifymex(fd,msg,usrmngr,configurazione,chattystats,lock);
            close(newFile);
        }
        free(completepath);
    }
    free(file.buf);

    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);

    MUTEXLOCK(*lock);
    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    int fileToMap;
    struct stat s_file;
    op_t result=OP_FAIL;
    message_t reply;
    int dim=strlen(configurazione->DirName)+(msg.data.hdr.len);
    char* fileData=NULL;
    char* completepath;
    completepath=malloc(sizeof(char)*(dim+1));
    MEMORYCHECK(completepath);
    memset(completepath,'\0',(dim+1)*sizeof(char));
    strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
    strncat(completepath,msg.data.buf,msg.data.hdr.len);
    if(stat(completepath,&s_file)){
        result=OP_NO_SUCH_FILE;
    }
    else if(!(fileToMap=open(completepath, O_RDONLY))){
        perror("Apertura del File");
    }
    else {
        int isPending=0;
        result=OP_OK;
        fileData=mmap(NULL,s_file.st_size,PROT_READ,MAP_PRIVATE,fileToMap,0);
        setHeader(&(reply.hdr),result,"");
        setData(&reply.data,"",fileData,s_file.st_size);
        MUTEXLOCK(*lock);
        if(sendRequest(fd,&reply)<=0) err=-1;
        else{
            if(isPending==1) updnfile(chattystats,-1);
            updfile(chattystats,1);
        }
        MUTEXUNLOCK(*lock);
    }

    free(completepath);
    if(result!=OP_OK){
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        MUTEXLOCK(*lock);
        if(sendRequest(fd,&reply)<=0) err=-1;
        MUTEXUNLOCK(*lock);
        upderrors(chattystats,1);
    }
    return err;
}

int getprevmsgs_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    op_t result=OP_FAIL;
    message_t reply;
    history* newHistory=NULL;
    result=prevMessage(usrmngr, msg.hdr.sender, &newHistory);
    if(result!=OP_OK){
        upderrors(chattystats,1);
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        MUTEXLOCK(*lock);
        if(sendRequest(fd,&reply)<=0) err=-1;
        MUTEXUNLOCK(*lock);
    }
    else{
        size_t nMex=newHistory->nel;
        memset(&reply,0,sizeof(message_t));
        setHeader(&reply.hdr,result,"");
        setData(&reply.data,"",(char*)&(nMex),sizeof(size_t));
        MUTEXLOCK(*lock);
        if(sendRequest(fd,&reply)<=0) err=-1;
        else{
            int i=newHistory->first;
            int el=0;
            while(el<newHistory->nel){
                if(sendRequest(fd,newHistory->data[i])<=0) err=-1;
                else{
                    if(newHistory->data[i]->hdr.op==TXT_MESSAGE){
                        if(newHistory->pending[i]==1)
                            updndelivered(chattystats, -1);
                        updelivered(chattystats, 1);
                    }
                }
                ++el;
                i=(i+1)%newHistory->size;
            }
        }
        MUTEXUNLOCK(*lock);
        freeHistory(newHistory);
    }
    return err;
}


int usrlist_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    stringlist* users;
    op_t result=OP_FAIL;
    users=connectedUserList(usrmngr);
    if(users){
        result=OP_OK;
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->lenght);
    }
    else {
        result=OP_FAIL;
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
    }
    setHeader(&(reply.hdr),result,"");
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=1) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int unregister_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    op_t result=OP_FAIL;
    message_t reply;
    result=unregisterUser(usrmngr,msg.hdr.sender);
    if(result==OP_OK){
        updusers(chattystats,-1);
        updonline(chattystats,-1);
    }
    else upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXLOCK(*lock);
    sendRequest(fd,&reply);
    MUTEXUNLOCK(*lock);
    return -1;
}

int disconnect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    op_t result=OP_FAIL;
    result=disconnectUser(usrmngr,fd);
    if(result==OP_OK){
        updonline(chattystats,-1);
    }
    else upderrors(chattystats,1);
    return -1;
}

int creategroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=createGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int addgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=addtoGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int delfromgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=deletefromGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int delgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=deleteGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    MUTEXLOCK(*lock);
    if(sendRequest(fd,&reply)<=0) err=-1;
    MUTEXUNLOCK(*lock);
    return err;
}

int execute(int fd, manager* usrmngr, configs* configurazione,struct statistics* chattystats,pthread_mutex_t* lock){
    int err=0;
    message_t msg;
    message_t reply_err;
    if(readMsg(fd,&msg)<=0){
        msg.hdr.op=OP_FAIL;
         err=-1;
    }
    else{
        switch(msg.hdr.op){
            case REGISTER_OP: err=register_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case CONNECT_OP: err=connect_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case POSTTXT_OP: err=posttxt_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case POSTTXTALL_OP: err=posttextall_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case POSTFILE_OP: err=postfile_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case GETFILE_OP: err=getfile_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case GETPREVMSGS_OP: err=getprevmsgs_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case USRLIST_OP: err=usrlist_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case UNREGISTER_OP: err=unregister_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case DISCONNECT_OP: err=disconnect_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case CREATEGROUP_OP: err=creategroup_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case ADDGROUP_OP: err=addgroup_op(fd,msg,usrmngr,configurazione,chattystats,lock);
                                break;
            case DELGROUP_OP: err=delfromgroup_op(fd,msg,usrmngr,configurazione,chattystats, lock);
                                break;
            case DELALLGROUP_OP: err=delgroup_op(fd,msg,usrmngr,configurazione,chattystats, lock);
                                break;
            default:
                setHeader(&(reply_err.hdr),OP_NOT_EXISTS,"");
                setData(&(reply_err.data),"",NULL,0);
                if(sendRequest(fd,&reply_err)<=0) err=-1;
                upderrors(chattystats,1);

        }
        free(msg.data.buf);
    }
    if(msg.hdr.op==UNREGISTER_OP || msg.hdr.op==DISCONNECT_OP) return -1;
    if(err== -1){
        if(disconnectUser(usrmngr,fd)==OP_OK)
            updonline(chattystats,-1);
    }

    return err;
}
