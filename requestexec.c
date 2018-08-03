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

void register_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    message_t reply;
    stringlist* users;
    op_t result=OP_FAIL;
    result=registerUser(usrmngr,msg.sender,fd);
    setHeader(&(reply.hdr),result,"");
    if(result==OP_OK){
        users=connectedUserList(usrmngr);
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->dim);
        updusers(configurazione,1);
        updonline(configurazione,1);
    }
    else{
        setData(&(reply.data)"",NULL,0);
        upderrors(chattystats,1);
    }
    
    if(sendRequest(fd,&reply)==-1) printf("Errore comunicazione");
    if(result==OP_OK) freeStringList(users);
}


void connect_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    message_t reply;
    stringlist* users;
    op_t result=OP_FAIL;
    result=connectUser(usrmngr,msg.hdr.sender,fd);
    setHeader(&(reply.hdr),result,"");
    if(result==OP_OK){
        users=connectedUserList(usrmngr);
        setData(&(reply.data),"",users->str,(users->str_dim+1)*users->dim);
        updusers(configurazione,1);
    }
    else {
        setData(&(reply.data)"",NULL,0);
        upderrors(chattystats,1);
    }
    
    if(sendRequest(fd,&reply)==-1) printf("Errore comunicazione");
    if(result==OP_OK) freeStringList(users);
}

message_t* copymex(message_t mex){
    message_t* newmex=NULL;
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

op_t notifymex(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    op_t result=OP_FAIL;
    int fd_receiver;
    stringlist* users;
    message_t* notify;
    if (msg.hdr.op=TXT_MESSAGE && msg.data.hdr.len>configurazione->MaxMsgSize) result=OP_MSG_TOOLONG;
    else if(msg.hdr.op=FILE_MESSAGE && msg.data.hdr.len>configurazione->MaxFileSize) result= OP_FILE_TOOBIG;
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
            if(sendRequest(fd_receiver,&msg)==-1) printf("Errore Comunicazione");
            if(msg.hdr.op==TXT_MESSAGE) updelivered(chattystats,1);
        }
        else if(msg.hdr.op==TXT_MESSAGE) updndelivered(chattystats,1);
        if(msg.hdr.op==FILE_MESSAGE) updnfile(chattystats,1);
    }
    else if(isingroup(usrmngr,msg.data.hdr.receiver,msg.hdr.sender)){
        char* tmp;
        users=userGroupList(usrmngr,msg.data.hdr.receiver);
        tmp=users;
        for(int i=0;i<users->lenght;i++){
            notify=copymex(msg);
            fd_receiver=storeMessage(usrmngr,tmp,notify);
            if(msg.hdr.op=FILE_MESSAGE)updnfile(chattystats,1);
            if(fd_receiver==-1 && msg.hdr.op==TXT_MESSAGE) updndelivered(chattystats,1);
            else if(fd_receiver>-1){
                if(sendRequest(fd_receiver,&msg)==-1) printf("Errore Comunicazione");
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

void posttxt_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    message_t reply;
    op_t result=OP_FAIL;
    msg.hdr.op=TXT_MESSAGE;
    result=notifymex(fd,msg,usrmngr,configurazione,chattystats);
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data)"",NULL,0);
    if(sendRequest(fd,&reply)==-1) printf("Errore Comunicazione");
}

void posttextall_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    char* users;
    op_t result=OP_FAIL;
    message_t reply;
    int fd_receiver;
    msg.hdr.op=TXT_MESSAGE;
    if (msg.data.hdr.len>configurazione->MaxMsgSize) result=OP_MSG_TOOLONG;
    else{
        users=registredUserList(usrmngr);
        for(int i=0;i<users->lenght;i++){
            notify=copymex(msg);
            fd_receiver=storeMessage(usrmngr,tmp,notify);
            if(fd_receiver==-1) updndelivered(chattystats,1);
            else if(fd_receiver>-1){
                if(sendRequest(fd_receiver,&msg)==-1) printf("Errore Comunicazione");
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
    setData(&(reply.data)"",NULL,0);
    if(sendRequest(fd,&reply)==-1) printf("Errore Comunicazione");
}


void postfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    result=OP_FAIL;
    char* tmp;
    int bytewrote=0;
    message_t reply;
    message_data_t file;
    int new_file;
    readData(fd,&file);
    int fd_receiver;
    msg.hdr.op=FILE_MESSAGE;
    if(file.hdr.len>configurazione->MaxFileSize){
        result=OP_FILE_TOOBIG;
        upderrors(configurazione,1);   
    }
    else{
        struct stat s_file;
        int dim=strlen(configurazione->DirName)+strlen(msg.data.hdr.len);
        char* completepath;
        completepath=malloc(sizeof(char)*dim+1);
        MEMORYCHECK(completepath);
        memset(completepath,'\0',(dim+1)*sizeof(char));
        strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
        strncat(completepath,msg.data.buf,msg.data.hdr.len);
        if(stat(completepath,&s_file)==0) result OP_FILE_EXISTS;
        else if(!new_file=open(completepath,O_WRONLY| O_CREAT);) perror("Apertura del File");
        else{
            char* postion=file.buf;
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
                upderrors(configurazione,1);   
            }
            result=notifymex(fd,msg,usrmngr,configurazione,chattystats);
            fclose(new_file);
        }
        
        free(completepath);
    }
    setHeader(&(reply.hdr),result,"");
    setData(&(reply.data)"",NULL,0);
    
    if(sendRequest(fd,&reply)==-1) printf("Errore Comunicazione");
}

void getfile_op(int fd, message_t msg, manager* usrmngr,configs* configurazione,struct stats* chattystats){
    int new_file;
    result=OP_FAIL;
    message_t reply;
    int dim=strlen(configurazione->DirName)+strlen(msg.data.hdr.len);
    char* completepath;
    completepath=malloc(sizeof(char)*(dim+1));
    MEMORYCHECK(completepath);
    memset(completepath,'\0',(dim+1)*sizeof(char));
    strncat(completepath,configurazione->DirName,strlen(configurazione->DirName));
    strncat(completepath,msg.data.buf,strlen(msg.data.hdr.len));
    
}
