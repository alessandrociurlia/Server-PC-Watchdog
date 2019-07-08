#!/bin/bash

# Arduino Watchdog
# Alessandro Ciurlia


# setto la porta seriale del mio Arduino
ARDUINO=/dev/ttyACM0

# funzione che verifica se Arduino è connesso 
function check {
	if [ ! -c $ARDUINO ]
	then
		echo "Errore: Arduino non è connesso!"
		exit 1
	fi
}

# funzione che invia i dati tramite seriale all'Arduino
function echoserial {
    if [ -c $ARDUINO ]
	then
		echo "$1" > $ARDUINO
	fi
}

# funzione che stampa la data e l'ora
function echodate {
	echo -n "`date +"%d/%m/%Y %H:%M:%S"` "
}

# funzione che attiva il watchdog con tempo limite di blocco di 3 min
function activate {
	echoserial "act 180"
	echo -n "."
}

# funzione che attiva il falso watchdog con tempo limite di blocco di 3 min
function fakeactivate {
	echoserial "fakeact 180"
	echo -n "."
}

# funzione che disattiva il watchdog
function deactivate {
	echodate
	echo "Sto disattivando il watchdog"
	check
	echoserial "deact"
	data
	exit
}

# funzione che richiede ad Arduino i dati del watchdog
function data {
	echodate
	timeout 1s head -n1 $ARDUINO & 
	sleep 0.1
	echoserial "data"
	sleep 0.1
}

# funzione che pulisce la memoria EEPROM di Arduino
function clearmemory {
	echoserial "clear"
}

# se il comando immesso è "act" attivo il watchdog, stampo i dati e invio costantemente un segnale
if [ "$1" == "act" ]
then
	echodate
	echo "Sto attivando il watchdog"
	check
	sleep 0.5
	printdata=true
	trap deactivate SIGINT SIGTERM
	while [ 1 = 1 ]
	do
		activate
		if [ "$printdata" == "true" ]
		then
			echo ""
			data
			printdata=false
		fi
		sleep 1
	done
# lo stesso di sopra ma con "fakeact" invece di "activate"
elif [ "$1" == "fakeact" ] 
then
	echodate
	echo "Sto attivando il falso watchdog"
	check
	sleep 0.5
	printdata=true
	trap deactivate SIGINT SIGTERM
	while [ 1 = 1 ]
	do
		fakeactivate
		if [ "$printdata" == "true" ]
		then
			echo ""
			data
			printdata=false
		fi
		sleep 1
	done
# se il comando immesso è "data" richiedo lo stato attuale del watchdog e lo stampo
elif [ "$1" == "data" ]
then
	echodate
	echo "Sto richiedendo i dati del watchdog"
	check
	data
# se il comando immesso è "clear" pulisco la memoria del watchdog
elif [ "$1" == "clear" ]
then
	echodate
	echo "Sto pulendo la memoria del watchdog"
	check
    clearmemory
	data 
else
	echo ""
	echo "Arduino Watchdog - Alessandro Ciurlia"
	echo ""
	echo "Istruzioni: "
	echo "watchdog [ act | fakeact | data | clear ]"
	echo "-> act     - attiva il watchdog"
	echo "             CTRL+C per disattivarlo e terminare lo script"
	echo "-> fakeact - attiva un falso watchdog che in caso di blocco non riavvia il server ma salva in memoria l'evento"
	echo "             CTRL+C per disattivarlo e terminare lo script"
	echo "-> data    - stampa i dati del watchdog"
	echo "-> clear   - pulisce la memoria del watchdog"
	echo ""
fi