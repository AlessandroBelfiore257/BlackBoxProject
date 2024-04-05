# Segnalazione allarmi

## Introduzione
Il meccanismo di segnalazione allarmi prevede tre diversi tipi di dinamiche:
- Precauzione da incidente
- Assistenza cliente
- Precauzione da alcuni sistemi non funzionanti / segnalazione di alcune situazioni anomale

1) La precauzione da incidente tratta la dinamica relativa ad un incidente stradale, inviando le coordinate GPS ed un insieme di informazioni utili ad una stazione remota dell'autonoleggio, sarà proprio lei ad avvisare i soccorsi che poi andranno a recarsi nel preciso punto di impatto.
2) L'assistenza cliente include due semplici funzionalità: la prima di Sos dove il cliente si mette in contatto con un operatore dell'autonoleggio per qualsiasi necessità/bisogno, la seconda prevede la chiamata ad un carro attrezzi con il relativo invio di coordinate GPS.
3) Per concludere, l'ultima dinamica è quella più interessante che consiste nel rilevamento di alcuni malfunzionamenti del veicolo e di alcuni comportamenti anomali del cliente a bordo dell'autovettura. Di seguito approfondiremo quest'ultima dinamica in modo più dettagliato

## Piccola overview del sistema proposto
Per gestire le situazioni relative all'ultima dinamica, il sistema (black box) viene dotato di 3 LED (verdo, giallo, rosso) per indicare diversi tipi di situazioni e per suggerire al cliente il corretto comportamento da intraprendere nel caso dell'accensione di una spia (led) indicante una situazione di avvertimento/pericolo:
1. Led verde: il veicolo funziona correttamente e il comportamneto del conducente è corretto.
2. Led giallo: indica un AVVERTIMENTO, se vengono eseguite azioni non lecite o vengono rilevati problemi di poca rilevanza viene azionato il led; dopo un breve periodo ad esempio 1/2 giorni nel quale continua a persistere l'azione o il problema, viene avvertita la centrale operativa. Ad esempio considerando che non è possibile fumare all'interno dei veicoli dell'autonoleggio e se viene rilevata una solo volta la presenza di fumo all'interno dell'abitacolo non viene avvisata la centrale; per cui il motto di questa spia è: se un problema di poca rilevanza o un'azione persistono, avverto la centrale operativa.
3. Led rosso: indica una situazione di PERICOLO, in questo caso la centrale operativa viene avvertita immediatamente.

Di seguito verranno proposte le dinamiche delle situazioni per le quali viene attivata una spia gialla piuttosto che una rossa:

__Situazioni azionamento led giallo:__
- Divieto di fumo all'interno dell'abitacolo per il mantenimento di un ambiente pulito e privo di odori sgradevoli per i successivi clienti. La presenza di fumo può essere rilevata da un sensore.
- Rilevamento guida su terreni non adatti: fuoristrada o percorsi non asfaltati sono vietati al conducente per evitare danni al veicolo. La presenza di terreni disconnessi può essere rilevata con l'accelerometro utilizzando l'asse verticale (z).
- Rilevamento guida su aree geografiche non percorribili: non posso recarmi ovunque con il mio veicolo. La segnalazione di aree non  percorribili può essere rilevata grazie al GPS.
- Rilevamento attività di trasporto/rimorchio: non posso trainare nulla con il veicolo a noleggio. E' possibile rilevare l'attività con alcuni parametri provenienti dalla rete CAN-BUS ad esempio carico motore e velocità.
- (?) Se la black box viene alimentata da una pila è necessario monitorarne la sua durata, se viene alimentata con la corrente della macchina non esiste alcun tipo di problema e può essere omessa questa casistica.

__Situazioni azionamento led rosso:__
- Distanza di frenata anomala: analizzando i dati di accelerazione e velocità del veicolo è possibile rilevare situazioni in cui la distanza di frenata è SIGNIFICATIVAMENTE più lunga, ciò può indicare un problema al sistema frenante o all'aderenza dei pneumatici. Si può rilevare tale situazione utilizzando i dati della rete CAN-BUS.
- Consumo anomalo di carburante: monitorando il consumo di carburante del veicolo e confrontandolo con i modelli di consumo attesi per determinate condizioni di guida (città, autostrada...) è possibile rilevare anomalie che potrebbero indicare perdite di carburante o problemi al motore, anche questa situazione può essere rilevata considerando i dati della rete CAN-BUS.
- Monitoraggio della qualità dellaria nell'abitacolo: utilizzando sensori di qualità dell'aria il veicolo può rilevare la presenza di gas nocivi. Attività che può essere rilevata con lo stesso sensore utilizzato per rilevare i fumi.
- Rilevamento presenza di alcohol: non è consentito al conducente mettersi alla guida in stato di ebrezza. Questa attività può essere monitorata attraverso un sensore che rileva la presenza di etanolo/alcohol nell'ambiente. Punto debole: non posso trasportare sul sedile del passeggero qualcuno che abbia bevuto.
__Di seguito alcune situazioni meno interessanti, ma comunque rilevanti per la sicurezza:__
- Temperatura anomala del motore: il motore non lavora nel suo range di temperatura e potrebbe guastarsi. Sensoristica: CAN-BUS.
- Temperatura refrigerante: la temperatura del liquido di raffreddamento del motore, solitamente espressa in gradi Celsius può essere critica. Sensoristica: CAN-BUS.
- Pressione collettore di entrata: la pressione del collettore di aspirazione del motore, solitamente espressa in kPa o PSI può portare a mancanza di potenza del veicolo, mancato avvio, rallentamenti irregolari, aumento dei consumi di carburante...
Sensoristica: CAN-BUS.
- Temperatura aria di entrata del collettore: ovvero la temperatura dell'aria aspirata nel collettore d'aspirazione se errata può provare riduzioni di potenza del motore, detonazione, aumento dei consumi di carburante, riduzione dell'affidabilità, prestazioni ridotte in condizioni estreme...
Sensoristica: CAN-BUS.
- Rilevamento della pressione dell'olio: monitorare la pressione dell'olio del motore per individuare situazioni in cui il motore è sovra-sforzo o con una bassa lubrificazione, potenzialmente causando danni al motore.
Sensoristica: CAN-BUS.

## Configurazione
### - Incidente
Sostanzialmente il sistema si divide in due fasi: la fase di RILEVAMENTO e quella di NOTIFICA.

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggVERcbmFbU3RhcnRdIC0tPiBiKFJlYWQgdGhlIHN0YXR1cyBvZiBzZW5zb3IpXG5iIC0tPiBjKFJlYWQgdGhlIGRhdGEgZnJvbSBHUFMpXG5jIC0tPiBke1NlbnNvciBpcyB0cmlnZ2VyZWQ_fVxuZCAtLT4gfE5vfCBmXG5kIC0tPiB8WWVzfCBlKFNlbmQgU01TIHVzaW5nIEdTTSlcbmUgLS0-IGYoRU5EKVxuZiAtLT4gYlxuICAgICAgICAgICAgICAiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggVERcbmFbU3RhcnRdIC0tPiBiKFJlYWQgdGhlIHN0YXR1cyBvZiBzZW5zb3IpXG5iIC0tPiBjKFJlYWQgdGhlIGRhdGEgZnJvbSBHUFMpXG5jIC0tPiBke1NlbnNvciBpcyB0cmlnZ2VyZWQ_fVxuZCAtLT4gfE5vfCBmXG5kIC0tPiB8WWVzfCBlKFNlbmQgU01TIHVzaW5nIEdTTSlcbmUgLS0-IGYoRU5EKVxuZiAtLT4gYlxuICAgICAgICAgICAgICAiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)

__Accelerometro__: nella fase di rilevamento l'accelerometro continua ad estrarre informazioni relative alla forza G (forza di accelerazione) sperimentata dagli occupanti.
__Microfono__: nella fase di rilevamento viene usato per rilevare valori di decibel decisivamente alti dovuti ad alcuni eventi ad esempio l'impatto o lo scoppio di un airbag. Viene usato come secondo filtro per ridurre i falsi positivi. (threshold > 140 db)
__GPS__: nella fase di notifica è utile per inviare le coordinate GPS alla stazione remota.
__VELOCITA' VEICOLO__: nella fase di notifica viene mandata alla stazione remota e può essere usata per future ricostruzioni/analisi dell'incidente.

Posso usare un accelerometro, in un evento del genere la black box subirà la stessa accelerazione degli occupanti del veicolo. La forza G che si ricava dall'accelerazione deve essere ragionevolmente alta per un incidente, per questo non vengono considerati incidenti se tutte le accelerazioni sono inferiori a 4/5G. Infatti sulle auto gli airbag non vengono altro che azionati dal superamento di una certa soglia di accelerazione.
Come secondo filtro viene utilizzato un microfono per cercare di ridurre i falsi allarmi.
Per un ulteriore riduzione di situazioni di falso allarme, all'utente vengono concessi 30 secondi per annullare il rilevamento della situazione di incidente; terminati i 30sec la chiamata partirà in automatico, in caso di annullamento si ritornerà ad una situazione di partenza

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggVERcbmFbU3RhcnRdIC0tPiBiKFJlYWQgZGF0YSlcbmIgLS0-IGN7RyBmb3JjZSB0cmlnZ2VyZWR9XG5jIC0tPiB8WWVzfCBke01pY3JvcGhvbmUgdHJpZ2dlcmVkfVxuYyAtLT4gfE5vfCBiXG5kIC0tPiB8WWVzfCBlKFJpbGV2YW1lbnRvIGluY2lkZW50ZSlcbmQgLS0-IHxOb3wgYlxuZSAtLT4gZnt1c2VyIGFib3J0fVxuZiAtLT4gfFllc3wgYlxuZiAtLT4gfE5vfCBnKFNlbmQgYWNjaWRlbnQgU01TKVxuZyAtLT4gaChFTkQpIiwibWVybWFpZCI6eyJ0aGVtZSI6ImRlZmF1bHQifSwidXBkYXRlRWRpdG9yIjpmYWxzZX0)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggVERcbmFbU3RhcnRdIC0tPiBiKFJlYWQgZGF0YSlcbmIgLS0-IGN7RyBmb3JjZSB0cmlnZ2VyZWR9XG5jIC0tPiB8WWVzfCBke01pY3JvcGhvbmUgdHJpZ2dlcmVkfVxuYyAtLT4gfE5vfCBiXG5kIC0tPiB8WWVzfCBlKFJpbGV2YW1lbnRvIGluY2lkZW50ZSlcbmQgLS0-IHxOb3wgYlxuZSAtLT4gZnt1c2VyIGFib3J0fVxuZiAtLT4gfFllc3wgYlxuZiAtLT4gfE5vfCBnKFNlbmQgYWNjaWRlbnQgU01TKVxuZyAtLT4gaChFTkQpIiwibWVybWFpZCI6eyJ0aGVtZSI6ImRlZmF1bHQifSwidXBkYXRlRWRpdG9yIjpmYWxzZX0)

I = 1 se (Acc/4o5G + SU/140db) >= Accident-threshold (in questo caso At = 2), 0 altrimenti
```sh
Incidente = I
Accident-threshold  = 2
I = 1 se ( (Accelerazione/4G) + (RumoreUrto/140db) ) >= Accident-threshold 
I = 0 altrimenti
```

A seguito dell'incidente le informazioni che possono essere trasportate sono: G-force per capire quanto effettivamente è stato forte l'impatto, la velocità del veicolo per capire a quanto stava andando, le coordinate GPS di latitudine e longitudine per capire dove è avvenuto l'incidente e in che momento è avvenuto (time).

Punto di debolezza: in caso di incidente estremo se la black box viene distrutta non è possibile contattare i soccorsi.
### - Assistenza cliente
Utilizzo di un bottone per gestire sia Sos che la chiamata al carro attrezzi, premendo il bottone per una durata di 5 sec mi metto in contatto con la centrale operativa, premendolo invece per 10 sec mi metto in contatto con il carro attrezzi (+ invio coordinate GPS).
### - Precauzione di alcuni sistemi non funzionanti / segnalazione di alcune situazioni anomale
Per ogni casistica utilizzo opportuni sensori per catturare i possibili problemi del veicolo o comportamenti scorretti del cliente:
comportamento/problema : sensore di rilevamento
- Fumo : rilevatore di fumi
- Guida in fuoristrada : accelerometro
- Guida fuori dal range permesso : GPS
- Rimorchio/traino : dati CAN-BUS
- Distanza di frenata anomala : dati CAN-BUS
- Consumo anomalo di carburante : dati CAN-BUS
- Monitoraggio qualità aria abitacolo : rilevatore gas
- Rilevamento presenza di alcohol nell'abitacolo : rilevatore etanolo/alcohol
- Temperatura motore : dati CAN-BUS
- Temperatura refrigerante : dati CAN-BUS
- Pressione collettore di entrata : dati CAN-BUS
- Temperatura aria di entrata del collettore : dati CAN-BUS
- Rilevamento pressione olio motore : dati CAN-BUS

## Feedback del corretto funzionamento
Dove sono presenti errori proveniente dalla rete CAN-BUS, bisogna agire sui valore dei vari campi e verificare che la segnalazione d'errore venga effettivamente fatta, dove sono presenti sensori bisogna simulare le varie situaioni ad esempio fumo, gas, zone non visitabili... e verificarne il corretto rilevamento.