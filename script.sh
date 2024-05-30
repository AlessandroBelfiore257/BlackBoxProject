#!/bin/bash

# Script per ricezione dati dedicato al server remoto, il quale deve restare sempre in ascolto
while true
do
   nc -l 8090 >> data.txt
done
