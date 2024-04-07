# Real-time vehicle tracking

## Introduzione
Questa funzionalità del sistema viene usata per monitorare e localizzare i veicoli della flotta ovunque si trovino. Il sistema di tracking
è in grado di generare la posizione esatta del veicolo usando due precisi parametri, latitudine e longitudine;
i dati di geolocalizzazione possono poi essere trasferiti e memorizzati all'interno di un computer per analisi future,
ad esempio visualizzare i percorsi abituali del cliente o il percorso effettuato in un preciso giorno ad
una precisa ora. Il sistema di tracking può essere molto utile anche in caso di furto del veicolo.

## Piccola overview del sistema proposto
L'antenna GPS e il ricevitore GPS ricevono informazioni dal satellite nel formato NMEA (National Marine Electronics Association),
queste vengono processate dalla board di interesse e trasmesse al server via SMS (Short message service) o GPRS (General Package radio service). Una tra queste due comunicazioni provvederà ad aprire una connessione HTTP con il server di tracking usando la rete GSM (modulo provvisto di antenna) con i relativi comandi Hayes command (AT Command), il server una volta ricevuti i dati li memorizza in un qualunque database per analisi future.

Schema generale:
Antenna GPS + modulo GPS (ricezione coordinate) -> microcontrollore (board utilizzata per processare i dati)
->  Antenna GSM + modulo GSM (comunicazione informazioni ad un server remoto qualsiasi).

Proposte:
- installazione di un sensore bussola per identificare la direzione di movimento del veicolo.

## Configurazione
Possibile componentistica hardware utilizzabile: modulo GPS NEO 6M, sensore bussola, board prescelta e modulo SIM800A.
La SIM800A ha bisogno di 2A, vieni quindi collegata una alimentazione esterna di 12V.
Il GPS lo si può connettere a dei semplici pin digitali di input/output della board.
SIM800A è connesso anche lui a dei pin digitali di input/output.

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShHUFMgKyBidXNzb2xhIF9fcmljZXppb25lIGRhdGlfXylcbiAgYihtaWNyb2NvbnRyb2xsb3JlIF9faSBkYXRpIHZlbmdvbm8gcHJvY2Vzc2F0aV9fKVxuICBjKEdTTSBfX3RyYXNmZXJpc2NlIGkgZGF0aSB2aWEgR1BSUyBvIFNNU19fKVxuICBcbiAgYS0tPmJcbiAgYi0tPmMiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShHUFMgKyBidXNzb2xhIF9fcmljZXppb25lIGRhdGlfXylcbiAgYihtaWNyb2NvbnRyb2xsb3JlIF9faSBkYXRpIHZlbmdvbm8gcHJvY2Vzc2F0aV9fKVxuICBjKEdTTSBfX3RyYXNmZXJpc2NlIGkgZGF0aSB2aWEgR1BSUyBvIFNNU19fKVxuICBcbiAgYS0tPmJcbiAgYi0tPmMiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)

## Imprecisione
In molti casi il GPS potrebbe darci una risposta errata o non sufficientemente precisa, dovuta a molti fattori esterni, ad esempio:
condizioni del tempo atmosferico, ambiente che circonda il veicolo, il ricevitore GPS, il sensore bussola e la variazione tra
la corretta direzione nord e la direzione nord magnetico.

## Feedback del corretto funzionamento
Provare a percorrere una tragitto in auto e considerando piccoli slot temporali provare a ricostruirlo, visualizzandolo su una mappa.
