# Real-time vehicle tracking

## Introduzione
Questa funzionalità del sistema viene usata per monitorare e localizzare i veicoli della flotta ovunque si trovino. Il sistema di tracking
è in grado di generare la posizione esatta del veicolo usando due precisi parametri, latitudine e longitudine;
i dati di geolocalizzazione possono poi essere trasferiti e memorizzati all'interno di un computer per analisi future,
ad esempio visualizzare i percorsi abituali del cliente o il percorso effettuato in un preciso giorno. Il sistema di tracking può essere molto utile anche in caso di furto del veicolo.

## Piccola overview del sistema proposto
L'antenna GPS e il ricevitore GPS ricevono informazioni dal satellite nel formato NMEA (National Marine Electronics Association),
queste vengono processate dalla board di interesse e trasmesse al server via SMS (Short message service) o GPRS (General Package radio service). Una tra queste due comunicazioni provvederà ad aprire una connessione HTTP con il server di tracking usando la rete GSM (modulo provvisto di antenna) con i relativi comandi Hayes command (AT Command), il server una volta ricevuti i dati li memorizza in un qualunque database per analisi future.

Schema generale:
Antenna GPS + modulo GPS (ricezione coordinate) -> microcontrollore (board utilizzata per processare i dati)
->  Antenna GSM + modulo GSM (comunicazione informazioni ad un server remoto qualsiasi).

## Configurazione
Possibile componentistica hardware utilizzabile: modulo GPS NEO 6M, board prescelta e modulo SIM800A.
La SIM800A ha bisogno di 2A, vieni quindi collegata una alimentazione esterna di 12V.
Il GPS lo si può connettere a dei semplici pin digitali di input/output della board.
SIM800A è connesso anche lui a dei pin digitali di input/output.

```mermaid
graph LR
  a(GPS __ricezione dati__)
  b(microcontrollore __i dati vengono processati__)
  c(GSM __trasferisce i dati via GPRS o SMS__)
  
  a-->b
  b-->c
```

## Imprecisione
In molti casi il GPS potrebbe darci una risposta errata o non sufficientemente precisa, dovuta a molti fattori esterni, ad esempio:
condizioni del tempo atmosferico e all'ambiente che circonda il veicolo.

## Feedback del corretto funzionamento
Provare a percorrere una tragitto in auto e considerando piccoli slot temporali provare a ricostruirlo, visualizzandolo su una mappa.
