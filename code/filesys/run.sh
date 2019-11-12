#!/bin/bash
echo ""
echo "Chequeando FileSystem(Cantidad de archivos)"
./filesys/nachos -f -cp filesys/test/small /small1 -ls
./filesys/nachos -cp filesys/test/small /small2 -ls
./filesys/nachos -cp filesys/test/small /small3 -ls
./filesys/nachos -cp filesys/test/small /small4 -ls
./filesys/nachos -cp filesys/test/small /small5 -ls
./filesys/nachos -cp filesys/test/small /small6 -ls
./filesys/nachos -cp filesys/test/small /small7 -ls
./filesys/nachos -cp filesys/test/small /small8 -ls
./filesys/nachos -cp filesys/test/small /small9 -ls
./filesys/nachos -cp filesys/test/small /small10 -ls
./filesys/nachos -cp filesys/test/small /small11 -ls
./filesys/nachos -D
echo './filesys/nachos -rm /small2'
./filesys/nachos -rm /small2 -ls
echo './filesys/nachos -rm /small1'
./filesys/nachos -rm /small1 -ls
echo './filesys/nachos -rm /small3'
./filesys/nachos -rm /small3 -ls
echo './filesys/nachos -rm /small4'
./filesys/nachos -rm /small4 -ls
echo './filesys/nachos -rm /small5'
./filesys/nachos -rm /small5 -ls
echo './filesys/nachos -rm /small6'
./filesys/nachos -rm /small6 -ls
echo './filesys/nachos -rm /small7'
./filesys/nachos -rm /small7 -ls
echo './filesys/nachos -rm /small8'
./filesys/nachos -rm /small8 -ls
echo './filesys/nachos -rm /small9'
./filesys/nachos -rm /small9 -ls
echo './filesys/nachos -rm /small10'
./filesys/nachos -rm /small10 -ls
echo './filesys/nachos -rm /small11'
./filesys/nachos -rm /small11 -ls
./filesys/nachos -D

echo ""
echo "Chequeando FileSystem(Creacion de carpetas)"
echo './filesys/nachos -f -mkd /folder -ls'
./filesys/nachos -f -mkd /folder -ls
echo './filesys/nachos -cp filesys/test/small /small -ls'
./filesys/nachos -cp filesys/test/small /small -ls
echo './filesys/nachos -cp filesys/test/medium /medium -ls'
./filesys/nachos -cp filesys/test/medium /medium -ls
echo './filesys/nachos -cp filesys/test/big /big -ls'
./filesys/nachos -cp filesys/test/big /big -ls
echo './filesys/nachos -pr /medium -ls'
./filesys/nachos -pr /medium -ls
echo './filesys/nachos -D'
./filesys/nachos -D
echo './filesys/nachos -rm /small -ls'
./filesys/nachos -rm /small -ls
echo './filesys/nachos -rm /big -ls'
./filesys/nachos -rm /big -ls
echo './filesys/nachos -D'
./filesys/nachos -D
echo './filesys/nachos -cp filesys/test/huge /huge -ls'
./filesys/nachos -cp filesys/test/huge /huge -ls
echo './filesys/nachos -D'
./filesys/nachos -D
echo './filesys/nachos -rm /huge -ls'
./filesys/nachos -rm /huge -ls
echo './filesys/nachos -D'
./filesys/nachos -D
echo './filesys/nachos -cp filesys/test/small /folder/small -ls'
./filesys/nachos -cp filesys/test/small /folder/small -ls
echo './filesys/nachos -mkd /folder/folderB -ls'
./filesys/nachos -mkd /folder/folderB -ls
echo './filesys/nachos -cp filesys/test/medium /folder/folderB/medium -ls'
./filesys/nachos -cp filesys/test/medium /folder/folderB/medium -ls
echo './filesys/nachos -pr /folder/small -ls'
./filesys/nachos -pr /folder/small -ls
echo './filesys/nachos -rm /folder/small -ls'
./filesys/nachos -rm /folder/small -ls
echo './filesys/nachos -pr /medium -ls'
./filesys/nachos -pr /medium -ls
echo './filesys/nachos -pr /folder/folderB/medium -ls'
./filesys/nachos -pr /folder/folderB/medium -ls
echo './filesys/nachos -ls /folder/'
./filesys/nachos -ls /folder/
echo './filesys/nachos -cd /folder/ -ls'
./filesys/nachos -cd /folder/ -ls
echo './filesys/nachos -rm /folder -ls'
./filesys/nachos -rm /folder -ls
echo './filesys/nachos -D'
./filesys/nachos -D
echo './filesys/nachos -rm /medium -ls'
./filesys/nachos -rm /medium -ls
echo './filesys/nachos -D'
./filesys/nachos -D

echo ""
echo "Chequeando FileSystem(Ejecucion de programas)"
./filesys/nachos -f -mkd /userland -ls
echo './filesys/nachos -cp userland/create userland/create -ls'
./filesys/nachos -cp userland/create userland/create -ls
echo './filesys/nachos -cp userland/cat userland/cat -ls'
./filesys/nachos -cp userland/cat userland/cat -ls
echo './filesys/nachos -cp userland/filetest userland/filetest -ls'
./filesys/nachos -cp userland/filetest userland/filetest -ls
echo './filesys/nachos -cp userland/shell userland/shell -ls'
./filesys/nachos -cp userland/shell userland/shell -ls
echo './filesys/nachos -cp userland/TestWrite userland/TestWrite -ls'
./filesys/nachos -cp userland/TestWrite userland/TestWrite -ls
echo './filesys/nachos -cp userland/TestWriteAux userland/TestWriteAux -ls'
./filesys/nachos -cp userland/TestWriteAux userland/TestWriteAux -ls
echo "Todos los programas se encuentran fueron copiados al disco"
./filesys/nachos -ls
echo "./filesys/nachos -x userland/create"
./filesys/nachos -x userland/create
./filesys/nachos -pr A.txt -ls
./filesys/nachos -x userland/TestWrite
./filesys/nachos -pr File.txt -ls
echo "Puede ejecutar los programas con ./filesys/nachos -x PROG"
exit
