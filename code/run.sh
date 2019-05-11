#!/bin/bash

pass=1

echo "Compilando.."
make polish > /dev/null
make > /dev/null
echo "Ejecutando.."

.//threads/nachos -rs $i -d atpslc -tc > /dev/null
if [ $? != 0 ]
then
	pass=0
	echo "Semilla $i falla con salida $?"
fi

if [ $pass = 1 ]
then
	echo "Nacho paso todos los casos con salida 0"
fi
