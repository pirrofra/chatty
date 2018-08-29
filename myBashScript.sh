#!/bin/bash
#
# chatterbox Progetto del corso di LSO 2017/2018
#
# Dipartimento di Informatica Università di Pisa
# Docenti: Prencipe, Torquati
#
#
# @file myBashScript.sh
# @author Francesco Pirrò 544539
# si dichiara che il contenuto di questo file è in ogni sua parte opera originale  dell'autore
#

usage="Uso del file: $0 <file configurazione> <tempo in minuti>
\nCerca nel file di configurazione la directory associata a DirName
\nIn DirName tutti i file più vecchi di t minuti sono eliminati e inseriti nell'archivio selectedFiles.tar.gz
\nSe il tempo è 0, stampa l'elenco di tutti i file e non ne modifica il contenuto"
#stringa messaggio di uso dello script

for val in "$@"
do
    if [ $val = "-help" ]; then
        echo -e $usage
        exit 1
    fi
done
#cerca -help tra i parametri e stampa il messaggio di uso. Se lo trova termina lo script

if [ ! $# = 2 ]; then
    echo -e $usage
    exit 1
#Numero di parametri incorretto. Stampa il messaggio di uso e termina

elif [ ! -f $1 ]; then
    echo "File \"$1\" inesistente"
    exit 1
#File nel primo paramtro inesistente. Termina

elif [ $2 -lt 0 ]; then
    echo "Intero negativo non accettabile"
    exit 1
#Numero Negativo passato come parametro. Termina

fi

dirName=$(grep -v '^ *#' $1 | grep 'DirName * = *')
#Rimuove i commenti dal file, e trova la riga che contiene DirName
dirName=${dirName/*'DirName' *= '/'/ '/'}
#Elimina "DirName =" dalla stringa
dirName=${dirName/%[[:space:]]/}
dirName=${dirName/#[[:space:]]/}
#elimina eventuali spazi bianchi alla fine o all'inizio

if [ -z $dirName ]; then
    echo "parametro DirName non specificato nel file config"
    exit 1
    #dirName vuoto, parametro non specificato nel file config. Termina

elif [ ! -d $dirName ]; then
    echo "Directory \"$dirName\" inesistente"
    exit 1
    #dirName non esiste. Termina

elif [ -z "$(ls $dirName)" ]; then
    echo "Directory \"$dirName\" vuota"
    exit 0
    #dirName vuota. Termina

elif [ $2 = 0 ]; then
    #tempo = 0. Stampa i file contenuti in dirName
    echo "File in ${dirName}:"
    for file in ${dirName}/*
    do
        if [[ -f $file ]]; then
            echo -${file##*/}
        fi
        #Itera sui file contenuti in dirName. Stampa solo i file e non le directory.
    done
    exit 0

else
    #tempo > 0 .
    timeInSeconds=$((${2} * 60))
    #trasforma il tempo da minuti in secondi
    arc=selectedFiles.tar.gz
    #nome dell'archivio
    created=0
    for file in ${dirName}/*
    do
        #Itera sui file contenuti in dirName
        fileAge=$(($(date +%s) - $(stat -c%Y $file)))
        #calcola l'età del file in minuti
        if [ $fileAge -gt $timeInSeconds ]; then
            #età file è più grande del tempo specificato come parametro
            if [ $created = 0 ];then
                tar -cPf $arc -C $dirName ${file##*/}
                created=1
                #primo file da inserire nell'archivio. Creo un nuovo archivio
             else
                tar -rPf $arc -C $dirName ${file##*/}
                #aggiugno ad un archivio già esistente se non è il primo file
             fi

             rm -i -R $file
             #rimuovo il file, per sicurezza chiedo la conferma all'utente
        fi
    done
    if [ $created = 0 ]; then
        echo "Nessun file compresso e/o rimosso"
    fi
    exit 0
fi
