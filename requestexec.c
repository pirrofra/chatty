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
        updusers(chattystats,1);
    }
    else {
        setData(&(reply.data),"",NULL,0);
        upderrors(chattystats,1);
    }

    if(sendRequest(fd,&reply)<=0) err=-1;
    if(result==OP_OK) freeStringList(users);
    return err;
}

message_t* copymex(message_t mex){
    message_t* newmex=NULL;
    char* newbuff=NULL;
    newmex=malloc(sizeof(message_t));
    MEMORYCHECK(newmex);
    setHeader(&(newmex->hdr),mex.hdr.op,mex.hdr.sender);
    newbuff=malloc(sizeof(char)*(mex.data.hdr.len+1));
    MEMORYCHECK(newbuff);
    memset(newbuff,'\0',(mex.data.hdr.len+1)*(sizeof(char)));
    strncpy(newbuff,mex.data.buf,(mex.data.hdr.len));
    setData(&(newmex->data),mex.data.hdr.receiver,newbuff,mex.data.hdr.len);
    return newmex;
}

op_t notifymex(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    op_t result=OP_FAIL;
    int fd_receiver;
    stringlist* users;
    message_t* notify;
    if (msg.hdr.op==TXT_MESSAGE && msg.data.hdr.len>configurazione->MaxMsgSize) result=OP_MSG_TOOLONG;
    else if(msg.hdr.op==FILE_MESSAGE && msg.data.hdr.len>configurazione->MaxFileSize) result= OP_FILE_TOOBIG;
    else if(!groupexist(usrmngr,msg.data.hdr.receiver)){
        notify=copymex(msg);
        fd_receiver=storeMessage(usrmngr,msg.data.hdr.receiver,notify);
        if(fd_receiver==-3) {
            result=OP_NICK_UNKNOWN;
            free((notify->data).buf);
            free(notify);
            upderrors(chattystats,1);
        }
        else if(fd_receiver==-2) {
            result=OP_FAIL;
            free((notify->data).buf);
            free(notify);
            upderrors(chattystats,1);
        }
        else if(fd_receiver>-1){
            if(sendRequest(fd_receiver,&msg)<=0) printf("Errore Comunicazione");
            if(msg.hdr.op==TXT_MESSAGE) updelivered(chattystats,1);
        }
        else if(msg.hdr.op==TXT_MESSAGE) {
            updndelivered(chattystats,1);
            result=OP_OK;
        }
        if(msg.hdr.op==FILE_MESSAGE) {
            updnfile(chattystats,1);
            result=OP_OK;
        }
    }
    else if(isingroup(usrmngr,msg.data.hdr.receiver,msg.hdr.sender)){
        char* tmp;
        users=userGroupList(usrmngr,msg.data.hdr.receiver);
        tmp=users->str;
        for(int i=0;i<users->lenght;i++){
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
            tmp+=(users->str_dim+1);
        }
        result=OP_OK;
    }
    else {
        result=OP_NO_PERMISSION;
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
    if (msg.data.hdr.len>configurazione->MaxMsgSize) result=OP_MSG_TOOLONG;
    else{
        users=registredUserList(usrmngr);
        tmp=users->str;
        for(int i=0;i<users->lenght;i++){
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
            tmp+=(users->str_dim+1);
        }
    }
    setHeader(&(reply.hdr),result,"");
    if(sendHeader(fd,&reply.hdr)<=0) err=-1;
    return err;
}


int postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    op_t result=OP_FAIL;
    int byte_wrote=0;
    message_t reply;
    struct stat s_file;
    message_data_t file;
    int new_file;
    readData(fd,&file);
    msg.hdr.op=FILE_MESSAGE;
    if(file.hdr.len>configurazione->MaxFileSize){
        result=OP_FILE_TOOBIG;
        upderrors(chattystats,1);
    }
    else{
        int dim=strlen(configurazione->DirName)+msg.data.hdr.len;
        char* completepath;
        completepath=malloc(sizeof(char)*dim+1);
        MEMORYCHECK(completepath);
        memset(completepath,'\0',(dim+1)*sizeof(char));
        strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
        strncat(completepath,msg.data.buf,msg.data.hdr.len);
        if(stat(completepath,&s_file)==0){
            result=OP_FILE_EXISTS;
            upderrors(chattystats,1);
        }
        else if(!(new_file=open(completepath,O_WRONLY| O_CREAT))) {
            perror("Apertura del File");
            upderrors(chattystats,1);
        }
        else{
            char* position=file.buf;
            int len=file.hdr.len;
            while(len>0){
                byte_wrote=write(new_file,file.buf, len);
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
            else result=notifymex(fd,msg,usrmngr,configurazione,chattystats);
            close(new_file);
        }

        free(completepath);
    }
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data),"",NULL,0);

    if(sendRequest(fd,&reply)<=0) err=-1;
    return err;
}

int getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    int new_file;
    struct stat s_file;
    op_t result=OP_FAIL;
    message_t reply;
    int dim=strlen(configurazione->DirName)+(msg.data.hdr.len);
    char* completepath;
    completepath=malloc(sizeof(char)*(dim+1));
    MEMORYCHECK(completepath);
    memset(completepath,'\0',(dim+1)*sizeof(char));
    strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
    strncat(completepath,msg.data.buf,msg.data.hdr.len);
    if(stat(completepath,&s_file)){
        result=OP_NO_SUCH_FILE;
        upderrors(chattystats,1);
    }
    else{

    }
    return err;
}

int getprevmsgs_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct statistics* chattystats){
    int err=0;
    op_t result=OP_FAIL;
    message_t reply;
    userdata* user;
    int i=hash_pjw((void*) (msg.hdr).sender)%NUMMUTEX;
    MUTEXLOCK(usrmngr->lockr[i]);
    user=icl_hash_find(usrmngr->registred_user,(msg.hdr).sender);
    if(user){
        int j=(user->user_history)->first;
        int el=0;
        result=OP_OK;
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"", (char*)&(user->user_history)->nel,sizeof(size_t));
        if(sendRequest(fd,&reply)<=0) err=-1;
        while(el<(user->user_history)->nel){
            if(sendRequest(fd,(user->user_history)->data[j])<=0){
                result=OP_FAIL;
                el=-1;
            }
            else{
                if(((user->user_history)->data[j])->hdr.op==TXT_MESSAGE){
                    updelivered(chattystats,1);
                    if((user->user_history)->pending[j]==1){
                        updndelivered(chattystats,1);
                        (user->user_history)->pending[j]=0;
                    }
                }
                ++el;
                j=(j+1)%(user->user_history)->size;
            }
        }
    }
    else result=OP_NICK_UNKNOWN;
    MUTEXUNLOCK(usrmngr->lockr[i]);
    if(result!=OP_OK){
        upderrors(chattystats,1);
        setHeader(&(reply.hdr),result,"");
        setData(&(reply.data),"",NULL,0);
        if(sendRequest(fd,&reply)<=0) err=-1;
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
    result=createGroup(usrmngr,msg.hdr.sender,msg.data.buf);
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
    result=addtoGroup(usrmngr,msg.hdr.sender,msg.data.buf);
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
    result=deletefromGroup(usrmngr,msg.hdr.sender,msg.data.buf);
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
    result=deleteGroup(usrmngr,msg.hdr.sender,msg.data.buf);
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
