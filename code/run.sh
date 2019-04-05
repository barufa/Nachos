#!/bin/bash

rango=$(seq 100)

for i in $rango
do
	.//threads/nachos -rs $i
done

