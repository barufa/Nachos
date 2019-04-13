#!/bin/bash

rango=$(seq 1000)
pass=1

echo "Compilando.."
make polish > /dev/null
make > /dev/null
echo "Ejecutando.."

for i in $rango
do
	.//threads/nachos -rs $i > /dev/null
	if [ $? != 0 ]
	then
		pass=0
		echo "Semilla $i falla con salida $?"
	fi
done

if [ $pass = 1 ]
then
	echo "Nacho paso todos los casos con salida 0"
fi
