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




int register_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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

    if(sendRequest(fd,&reply)<=0) err=-1;
    if(result==OP_OK) freeStringList(users);
    return err;
}


int connect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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

    if(sendRequest(fd,&reply)<=0) err=-1;
    if(result==OP_OK) freeStringList(users);
    return err;
}


op_t notifymex(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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
            if(sendRequest(fd_receiver,&msg)<=0) printf("Errore Comunicazione");
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
            if(strcmp(msg.hdr.sender,tmp)){
                notify=copymex(msg);
                fd_receiver=storeMessage(usrmngr,tmp,notify);
                if(msg.hdr.op==FILE_MESSAGE)updnfile(chattystats,1);
                if(fd_receiver==-1 && msg.hdr.op==TXT_MESSAGE) updndelivered(chattystats,1);
                else if(fd_receiver>-1){
                    if(sendRequest(fd_receiver,&msg)<=0) printf("Errore Comunicazione");
                    if(msg.hdr.op==TXT_MESSAGE)updelivered(chattystats,1);
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
    else {
        result=OP_NO_PERMISSION;
        upderrors(chattystats,1);
   }


   if(result!=OP_OK){
       upderrors(chattystats,1);
   }
    return result;
}

int posttxt_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    msg.hdr.op=TXT_MESSAGE;
    result=notifymex(fd,msg,usrmngr,configurazione,chattystats);
    setHeader(&(reply.hdr),result,"");
    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    return err;
}

int posttextall_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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
                    if(sendRequest(fd_receiver,&msg)<=0) err=-1;
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
    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    return err;
}

int createInfoFile(char* completepath, char* sender){
    int ret=0;
    char* completepathInfo;
    int infoFile;
    int dim=strlen(completepath);
    completepathInfo=malloc((dim+6)*sizeof(char));
    MEMORYCHECK(completepathInfo);
    memset(completepathInfo,'\0',(dim+6)*sizeof(char));
    strncat(completepathInfo,completepath,dim);
    strncat(completepathInfo,".info",5);
    if((infoFile=open(completepathInfo,O_WRONLY| O_CREAT |O_TRUNC,00700))==-1){
        perror("Apertura File");
        ret=-1;
    }
    else{
        char* position=sender;
        int len=MAX_NAME_LENGTH+1;
        int byte_wrote=0;
        while(len>0){
            if((byte_wrote=write(infoFile,position,len))<=0){
                len=0;
                ret=-1;
            }
            else{
                len -=byte_wrote;
                position +=byte_wrote;
            }
        }
        close(infoFile);
    }
    free(completepathInfo);
    return ret;
}

int readInfoFile (manager* usrmngr, char* completepath, char* receiver,int* isPending){
    int ret=0;
    char* completepathInfo;
    struct stat statInfoFile;
    int infoFile;
    int dim=strlen(completepath);
    completepathInfo=malloc((dim+6)*sizeof(char));
    MEMORYCHECK(completepathInfo);
    memset(completepathInfo,'\0',(dim+6)*sizeof(char));
    strncat(completepathInfo,completepath,dim);
    strncat(completepathInfo,".info",5);
    if(stat(completepathInfo,&statInfoFile)){
        perror("Apertura File");
        ret=-1;
    }
    else if((infoFile=open(completepathInfo,O_RDONLY))==-1){
        perror("Apertura File");
        ret=-1;
    }
    else{
        char* fileData=NULL;
        char* currNick=NULL;
        int nUsername=0;
        nUsername=statInfoFile.st_size/(MAX_NAME_LENGTH+1);
        fileData=malloc(statInfoFile.st_size);
        MEMORYCHECK(fileData);
        memset(fileData,'\0',statInfoFile.st_size);
        currNick=fileData;
        for(int i=0;i<nUsername;i++){
            char* position=currNick;
            int len=MAX_NAME_LENGTH+1;
            int byte_read;
            while(len>0){
                if((byte_read=read(infoFile,position,len))<=0){
                    perror("Lettura File");
                    ret=-1;
                    i=nUsername;
                }
                len-=byte_read;
                position+=byte_read;
            }

            if(i==0){
                if(strcmp(currNick,receiver)==0 || isingroup(usrmngr,receiver, currNick)){
                    *isPending=1;
                    ret=0;
                }
                else ret=1;
            }
            else if(strcmp(currNick,receiver)==0) *isPending=0;

            currNick+=MAX_NAME_LENGTH+1;
        }
        free(fileData);
        close(infoFile);

    }
    free(completepathInfo);
    return ret;
}

void setDownloaded(char* completepath, char* receiver){
    char* completepathInfo;
    int infoFile;
    int dim=strlen(completepath);
    completepathInfo=malloc((dim+6)*sizeof(char));
    MEMORYCHECK(completepathInfo);
    memset(completepathInfo,'\0',(dim+6)*sizeof(char));
    strncat(completepathInfo,completepath,dim);
    strncat(completepathInfo,".info",5);
    if((infoFile=open(completepathInfo,O_APPEND))==-1)
        perror("Apertura File");

    else{
        char* position=receiver;
        int len=MAX_NAME_LENGTH+1;
        int byte_wrote=0;
        while(len>0){
            if((byte_wrote=write(infoFile,position,len))<=0)
                len=0;

            else{
                len -=byte_wrote;
                position +=byte_wrote;
            }
        }
        close(infoFile);
    }

    free(completepathInfo);
}

int postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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
            if(createInfoFile(completepath,msg.data.hdr.receiver)==-1 && byte_wrote<0){
                result=OP_FAIL;
                upderrors(chattystats,1);
            }

            else result=notifymex(fd,msg,usrmngr,configurazione,chattystats);
            close(newFile);
        }
        free(completepath);
    }
    free(file.buf);

    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);

    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    return err;
}

int getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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
        int infoFile=0;
        int isPending=0;
        infoFile=readInfoFile(usrmngr,completepath, msg.hdr.sender, &isPending);
        if(infoFile==-1)
            result=OP_FAIL;
        else if(infoFile==1)
            result=OP_NO_PERMISSION;
        else{
            result=OP_OK;
            fileData=mmap(NULL,s_file.st_size,PROT_READ,MAP_PRIVATE,fileToMap,0);
            setHeader(&(reply.hdr),result,"");
            setData(&reply.data,"",fileData,s_file.st_size);
            if(sendRequest(fd,&reply)<=0) err=-1;
            else{
                if(isPending==1) updnfile(chattystats,-1);
                else setDownloaded(completepath, msg.hdr.sender);
                updfile(chattystats,1);
            }

        }
    }

    free(completepath);
    if(result!=OP_OK){
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        if(sendRequest(fd,&reply)<=0) err=-1;
        upderrors(chattystats,1);
    }
    return err;
}

int getprevmsgs_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    op_t result=OP_FAIL;
    message_t reply;
    history* newHistory=NULL;
    result=prevMessage(usrmngr, msg.hdr.sender, &newHistory);
    if(result!=OP_OK){
        upderrors(chattystats,1);
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        if(sendRequest(fd,&reply)<=0) err=-1;
    }
    else{
        size_t nMex=newHistory->nel;
        memset(&reply,0,sizeof(message_t));
        setHeader(&reply.hdr,result,"");
        setData(&reply.data,"",(char*)&(nMex),sizeof(size_t));
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
        freeHistory(newHistory);
    }
    return err;
}


int usrlist_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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
    if(sendRequest(fd,&reply)<=1) err=-1;
    return err;
}

int unregister_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
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
    sendRequest(fd,&reply);
    return -1;
}

int disconnect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    op_t result=OP_FAIL;
    result=disconnectUser(usrmngr,fd);
    if(result==OP_OK){
        updonline(chattystats,-1);
    }
    else upderrors(chattystats,1);
    return -1;
}

int creategroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=createGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    if(sendRequest(fd,&reply)<=0) err=-1;
    return err;
}

int addgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=addtoGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    if(sendRequest(fd,&reply)<=0) err=-1;
    return err;
}

int delfromgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=deletefromGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    if(sendRequest(fd,&reply)<=0) err=-1;
    return err;
}

int delgroup_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    message_t reply;
    op_t result=OP_FAIL;
    result=deleteGroup(usrmngr,msg.hdr.sender,msg.data.hdr.receiver);
    if(result!=OP_OK) upderrors(chattystats,1);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);
    if(sendRequest(fd,&reply)<=0) err=-1;
    return err;
}

int execute(int fd, manager* usrmngr, configs* configurazione,struct statistics* chattystats){
    int err=0;
    message_t msg;
    message_t reply_err;
    if(readMsg(fd,&msg)<=0){
        msg.hdr.op=OP_FAIL;
         err=-1;
    }
    else{
        switch(msg.hdr.op){
            case REGISTER_OP: err=register_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case CONNECT_OP: err=connect_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case POSTTXT_OP: err=posttxt_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case POSTTXTALL_OP: err=posttextall_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case POSTFILE_OP: err=postfile_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case GETFILE_OP: err=getfile_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case GETPREVMSGS_OP: err=getprevmsgs_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case USRLIST_OP: err=usrlist_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case UNREGISTER_OP: err=unregister_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case DISCONNECT_OP: err=disconnect_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case CREATEGROUP_OP: err=creategroup_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case ADDGROUP_OP: err=addgroup_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case DELGROUP_OP: err=delfromgroup_op(fd,msg,usrmngr,configurazione,chattystats);
                                break;
            case DELALLGROUP_OP: err=delgroup_op(fd,msg,usrmngr,configurazione,chattystats);
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
