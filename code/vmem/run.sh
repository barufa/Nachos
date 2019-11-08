#!/bin/bash
echo "./vmem/nachos -x userland/create"
./vmem/nachos -x userland/create
echo ""
cat A.txt
echo ""
echo "./vmem/nachos -x userland/exec"
./vmem/nachos -x userland/exec -rs 19
echo ""
cat friends.txt
echo ""
echo "./vmem/nachos -x userland/TestWrite -rs 17"
./vmem/nachos -x userland/TestWrite -rs 17
echo ""
cat File.txt
echo ""
echo """
Ejecutar:
->cat
->echo A B C
->min
->\$?
->matmult
->\$?
->sort
->\$?
->exit
"""
echo "./vmem/nachos -x userland/shell"
./vmem/nachos -x userland/shell
exit
