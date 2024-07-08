_A seguito di una particolare situazione riconosciuta dai sensori viene descritto il funzionamento/comportamento atteso della black box_

## Azione-reazione
### Le funzionalità sono 3:
#### Tracking:
Invio coordinate GPS (latitudine e longitudine e direzione) ad un server remoto, immagazzino i dati e dopo averne raccolti abbastanza provare a ricostruire il tragitto percorso.
#### Seganalazione allarmi:
- Assistenza incidente:
Simulare una dinamica di incidente e verificare che venga notificata la centrale operativa con tutte le informazioni utili (G-force, velocità, latitudine, longitudine e momento dell'impatto).
- Assistenza cliente:
Simulare entrambe le features (Invio notifica a server remoto per qualsiasi bisogno e invio notifica a server remoto per necessità di carroattrezzi), con "invio notifica" si intende che il server remoto riceva una notifica da un qualunque veicolo dell'autonoleggio con un opportuno messaggio che ne indica la descrizione del problema e con le informazioni utili ad esempio coordinate gps in caso di notifica per carroattrezzi.
- Avvertimento di alcuni sistemi non funzionanti / segnalazione di alcune situazioni anomalie: 

_*LED GIALLO:*_

1) Simulare la presenza di fumo nell'abitacolo, verificare l'accensione del led giallo e la notifica al conducente, se ne viene rilevata la presenza per un periodo prolungato e continuo (2/3 giorni) avvisare la centrale del comportamento scorretto del cliente.

2) Rilevamento guida su terreni non asfaltati con la modifica di alcuni dati ottenuti dall'accelerometro, verificare l'accensione del led giallo e la notifica al conducente, se viene rilevata un'attività frequente in fuori strada viene contattata la centrale.

3) Rilevamento guida in zona non autorizzata grazie alle coordinate GPS, verificare l'accensione del led giallo e la relativa notifica al conducente. Se la posizione del veicolo continua ad essere al di fuori del range voluto, viene inviata una notifica alla centrale operativa.

4) Rilevamento attività di rimorchio, se viene rilevata un'attività di trasporto viene azionato il led giallo e mandata una notifica al conducente, dopo un periodo prestabilito viene avvisata la centrale dell'attività non lecita.

_*LED ROSSO:*_  

1) A seguito di più rilevamenti di una frenata anomala, viene immediatamente segnalato alla centrale il guasto, inoltre il led rosso viene acceso. 

2) Il consumo anomalo di carburante viene rilevato a seguito del trasferimento dei dati ad un server remoto e a seguito di un'accurata analisi di essi, lo stato del led risulta essere rosso.

3) La qualità dell'aria e l'alcool possono essere simulate con la relativa presenza di scarsa Co2 o presenza di alcool, per verificarne il corretto funzionamento è necessario che il led assuma lo stato rosso e che venga segnalata la situazione con un opportuno messaggio al cliente e alla centrale.

4) Modificando i dati della rete can bus: temperatura del liquido di raffreddamento, temperatura dell'olio motore e la coppia del motore con opportuni valori critici, il sistema deve reagire inviando un opportuno segnale al conducente e alla centrale operativa e con l'azionamento del led rosso.

#### Profilazione guida
Si propone di eseguire ripetutamente lo stesso percorso in automobile, affrontandolo con tre distinti stili di guida: tranquillo, moderato e brusco, al fine di valutare e confrontare gli effetti di tali stili sulla dinamica di guida. Si osserverà un diverso score ottenuto dalla board nei tre casi distinti.
