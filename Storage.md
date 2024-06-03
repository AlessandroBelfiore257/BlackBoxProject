# Struttura interna del db per la gestione dei dati raccolti 
_Numeri alti indicano priorità alta_

### Tabella principale di memorizzazione dati (sensori/can bus):
|nome_sensore|valore_generico|tipo_di_dato|unità_di_misura|timestamp|sincronizzato|priorità 
|:---:|:---:|:---:|:---:|:---:| :---:|:---:
|TEXT|TEXT|TEXT|TEXT|INTEGER (numero di secondi trascorsi dall'epoc)|INTEGER (boolean)|INTEGER
|"Temperatura liquido refrigerante"|"98.6"|"Float"|"°C"|1715786497|0|3
|"Carico motore"|"47"|"Integer"|"%"|1715269497|1|8

### Tabella tempo di campionamento:
|nome_sensore|tempo_storage (ogni quanto memorizzare il valore)|variabile (indica se il tempo di storage può cambiare)|fattore_incr_decr (di quanto aumentare o diminuire il tempo di storage)|soglia (delimita se i dati campionati sono omogenei "<" o eterogenei ">")|tMinStorage (tempo minimo di storage)|tMaxStorage (tempo massimo di storage)
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|TEXT|INTEGER (espressa in secondi) |INTEGER (boolean)|INTEGER|INTEGER|INTEGER|INTEGER|
|"Temperatura liquido refrigerante"|60|1|30|0.5|60|240
|"Carico motore"|45|0|NULL|NULL|NULL|NULL

### Tabella dati statistici:
|nome_sensore|data (YYYY/MM/GG)|media|mediana|moda|dev.std.|vMin|vMax|sincornizzato
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:--:
|Text|INTEGER|REAL|REAL|REAL|REAL|REAL|REAL|INTEGER (boolean)
|"Temperatura liquido refrigerante"|2024/05/15|120.4|115.8|109|110|45|150|0
|"Carico motore"|2024/05/15|60|56|56|59|0|99|1

## Meccanismo cambio tempo di storage
```mermaid
graph TD
  A[Sensor data] --> |Receving data| B(calculation of average and standard deviation)
  B --> |After n readings changeTimeStorage funcion| C{std. low? (std < soglia)}
  C --> |YES| D[change time storage in db increasing by: fattore_incr_decr]
  C --> |NO| E[change time storage in db decreasing by: fattore_incr_decr]
```
Logica: se dopo una serie di letture (n ad esempio) i dati hanno una bassa deviazione standard, ovvero sono molto simili tra di loro, memorizzo i dati del veicolo con una frequenza minore, aumentando il tempo di storage di un certo fattore (presente nella seconda tabella di questo file); se la deviazione standard è alta, significa che i dati sono abbastanza differenti tra di loro e li monitoro con una frequenza maggiore. Naturalmente vengono posti dei lower e upper bound per i tempo di memorizzazione entro i quali il tempo stesso può variare
