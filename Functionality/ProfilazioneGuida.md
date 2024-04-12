# Profilazione guida
## Introduzione
Nel quadro del nostro progetto di analisi della profilazione guida, sfruttiamo tecnologie di acquisizione dati per ottenere una comprensione dettagliata del comportamento del guidatore. A tale scopo, facciamo uso di due principali fonti di informazioni: i dati provenienti dal sistema di diagnostica a bordo (OBD-II) e quelli registrati da un accelerometro. Questi dispositivi forniscono una vasta gamma di dati in tempo reale, quali ad esempio velocità, accelerazione, decelerazione (frenata) e angoli di sterzata, consentendo di valutare numerosi aspetti relativi alla guida. Attraverso l'analisi approfondita di tali dati, siamo in grado di creare profili individuali dei guidatori e identificare modelli di comportamento distintivi che influenzano la sicurezza, l'efficienza e l'affidabilità del veicolo. Nel corso di questa relazione, esploreremo come l'integrazione di queste tecnologie ci permetta di ottenere informazioni preziose per migliorare l'esperienza di guida e ottimizzare le operazioni di gestione della flotta.

## Piccola overview del sistema proposto
I principali parametri che vengono catturati per l'analisi del guidatore sono: **rpm** ovvero i giri motore al minuto, la **velocità dell'auto**, il **carico del motore** e la **valvola a farfalla** provenienti dalla rete CAN-BUS presente all'interno del veicolo, **accelerazione**, **decelerazione** (frenata) e **sterzate** provengono invece dall'accelerometro. 
Una volta che la black box riceve i valori dei parametri, essa è in grado di analizzarli e riconoscere situazioni riconducibili ad una guida impropria raccogliendo il numero in cui si verificano tali situazioni all'interno di una struttura dati, dopo un periodo di 24h i dati vengono utilizzati per attribuire al guidatore un punteggio compreso tra [0-1].
Ecco le principali fasi del sistema proposto: 

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHNlbnNpbmcpXG4gIGIoRGF0YSBhY3F1aXNpdGlvbilcbiAgYyhEYXRhIHByb2Nlc3NpbmcpXG4gIGQoRGF0YSBzdG9yYWdlKVxuXG4gIGEtLT5iXG4gIGItLT5jXG4gIGMtLT5kXG4iLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHNlbnNpbmcpXG4gIGIoRGF0YSBhY3F1aXNpdGlvbilcbiAgYyhEYXRhIHByb2Nlc3NpbmcpXG4gIGQoRGF0YSBzdG9yYWdlKVxuXG4gIGEtLT5iXG4gIGItLT5jXG4gIGMtLT5kXG4iLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)
1. Data sensing: i dati provenienti dal motore della macchina ad esempio rpm, velocità, accelerazione... vengono percepiti dall'unità di controllo ECU
2. Data acquisition: i dati acquisiti dalla ECU vengono trasferiti via Bluetooth alla board prescelta utilizzando un adattatore OBD-II
3. Data processing: i dati trasferiti attraverso l'OBD-II sono processati dalla board di interesse
4. Data storage: vengono memorizzati in una struttura dati i valori di alcuni parametri che vengono ritenuti significativi per la valutazione finale del guidatore

ECU è l'unità centrale di controllo del veicolo dove sono collegati tutti i vari sensori presenti all'interno del motore dell'auto, aria condizionata, livello carburante, corpo dell'auto... . L'adapter OBD-II trasmette i dati dalla ECU alla board di interesse via Bluetooth. La board processa i dati ed è in grado di generare uno score del guidatore.
Il punteggio [0,1] viene calcolato considerando:
-   Il numero di eccessi di velocità dati dalla differenza tra i limiti imposti dalla legge e quelli reali del veicolo
-   Le accelerazioni e decelerazioni brusche
-   Le sterzate sia a sinistra che a destra brusche
-   Il rapporto (relativo) di velocità del veicolo con i giri del motore che deve essere compreso tra i valori di 0.9-1.3 per una buona guida 
-   Il rapporto (relativo) tra la valvola a farfalla e i giri del motore che deve essere compreso tra i valori di 0.9-1.3 per una buona guida 
-   Il carico del motore che deve essere compreso tra il 20% e il 50% per una buona guida 

## Configurazione
Per quanto riguarda il secondo e il terzo punto è possibile utilizzare un accelerometro a 3 assi utile per monitorare le accelerazioni/decelerazioni e le sterzate improvvise:

| Asse | Direzione | Azione/Evento |
| ------ | ------ | ------ | 
| x | sinistra/destra | sterzata
| y | anteriore/posteriore | accelerazione/decelerazione (frenata)
| z | su/giù | strada dissestata

![Accelerometro](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcRD7ePphShh30lDGfMYCU6N-NpKGUrzpNGroEUhe6-lpAqAL8NibGitXKTDsnjMunm4pt0&usqp=CAU)

Per quanto riguarda i punti 4 e 5 è possibile calcolarli per via analitica conoscendo i dati provenienti dall'OBD-II, mentre per l'ultimo punto (carico motore espresso in %) non si ha bisogno di alcuna manipolazione dei dati in quanto esso proviene direttamente dalla rete CAN-BUS.

### Tabella valutativa
Per valutare il comportamento del guidatore di seguito vengono mostrati i vari livelli di punteggio del guidatore che posso essere ottenuto dopo un periodo di 24h di monitoraggio guida

| Driver score | Driving behaviour |
|--------------|-------------------|
| < 0.10       | Safe              |
| 0.1 - 0.19   | Modest            |
| 0.19 - 0.38  | Moderate Risk     |
| 0.38 - 0.56  | Risk              |
| 0.56 - 0.75  | Reckless          |
| > 0.75       | Critical          |

_*Il punteggio del guidatore è inteso come la probabilità di rischio di procurare un incidente a seguito del viaggio osservato, pertanto sarà compreso tra 0 e 1*_.

Per concludere il nostro studio possiamo affiancare alla capacità della board di valutazione del guidatore, tecniche di Machine Learning relative sia all'apprendimento supervisionato che non supervisionato, consentendoci di esaminare più approfonditamente la natura della nostra flotta.
In questo contesto, ci concentreremo su due principali tecniche di Machine Learning: la _*classificazione*_ e il _*clustering*_. La classificazione ci permette di assegnare ai guidatori delle etichette, come ad esempio 'prudente' o 'negligente', in base ai loro comportamenti al volante, mentre il clustering ci consente di raggruppare i guidatori in base a similitudini nel comportamento di guida. Esploreremo queste tecniche nel dettaglio, evidenziando le loro applicazioni specifiche e i benefici che possono apportare all'analisi dei guidatori e alla gestione dell'autonoleggio.
## Machine Learning nell'analisi dei guidatori: applicazioni nel nostro contesto
Una volta processati i dati relativi all'analisi di profilazione del guidatore, essi possono essere inviati ad un server remoto. Dopodichè è possibile usare varie tecniche di machine learning di apprendiamento supervisionato ad esempio la **classificazione** o di apprendimento non supervisionato ad esempio il **clustering**, per assegnare ai clienti dell'autonoleggio una certa etichetta di classificazione (prudenti o negligenti) o raggrupparli in cluster e osservare il comportamento della flotta.
### Apprendimento supervisionato
La **classificazione** è una tecnica di apprendimento supervisionato, l'obiettivo è quello di predire la classe di appartenenza di un'istanza di input in un insieme discreto di classi. Ad esempio, classificare se una mail è spam o non spam, o se un'immagine contiene un gatto o un cane. Nel nosto caso di studio vorremmo poter classificare i clienti come guidatori prudenti o negligenti in base ai valori di valutazione studiati durante l'analisi di profilazione. Questa tecnica di ML prevede la suddivisione dei dati in due insiemi distinti: un insieme di addestramento e un insieme di test. L'insieme di addestramento viene utilizzato per addestrare il modello, mentre l'insieme di test viene utilizzato per valutarne le prestazioni su dati non visti. Per addestrare un modello di apprendimento supervisionato, vengono utilizzati diversi algoritmi in base al tipo di problema e al tipo di dati. Alcuni esempi di algoritmi comuni includono Support Vector Machines (SVM), Random Forest, Decision Trees, Neural Networks, e Regressione Lineare. Dopo l'addestramento, il modello viene valutato utilizzando l'insieme di test. Una volta soddisfatti delle prestazioni del modello, esso può essere pubblicato, implementato in un'applicazione o integrato in un sistema più ampio per fare previsioni su nuovi dati.

Di seguito un esempio di classificazione binaria:

![Classificazione](https://www.andreaminini.com/data/andreaminini/apprendimento-supervisionato-esempio-1.gif)

Se nel nostro studio si volesse classificare il guidatore in più classi (prudente, moderato, negligente) ad esempio per stipulare un contratto assicurativo in base allo stile di guida, esiste la classificazione multiclasse dove ci sono più di due classi possibili tra cui scegliere, ogni oggetto può essere assegnato a una sola classe tra le possibili.

Le fasi principali sono 7:

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcgZGVpIGRhdGkpXG4gIGMoRGl2aXNpb25lIGRlaSBkYXRpKVxuICBkKEFkZGVzdHJhbWVudG8gZGVsIG1vZGVsbG8pXG4gIGUoVmFsdXRhemlvbmUgZGVsIG1vZGVsbG8pXG4gIGYoVHVuaW5nIGRlZ2xpIGlwZXJwYXJhbWV0cmkgZSBkZXBsb3kgZGVsIG1vZGVsbG8pXG4gIGcoTW9uaXRvcmFnZ2lvIGUgYWdnaW9ybmFtZW50bylcblxuICBhLS0-YlxuICBiLS0-Y1xuICBjLS0-ZFxuICBkLS0-ZVxuICBlLS0-ZlxuICBmLS0-ZyIsIm1lcm1haWQiOnsidGhlbWUiOiJkZWZhdWx0In0sInVwZGF0ZUVkaXRvciI6ZmFsc2V9)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcgZGVpIGRhdGkpXG4gIGMoRGl2aXNpb25lIGRlaSBkYXRpKVxuICBkKEFkZGVzdHJhbWVudG8gZGVsIG1vZGVsbG8pXG4gIGUoVmFsdXRhemlvbmUgZGVsIG1vZGVsbG8pXG4gIGYoVHVuaW5nIGRlZ2xpIGlwZXJwYXJhbWV0cmkgZSBkZXBsb3kgZGVsIG1vZGVsbG8pXG4gIGcoTW9uaXRvcmFnZ2lvIGUgYWdnaW9ybmFtZW50bylcblxuICBhLS0-YlxuICBiLS0-Y1xuICBjLS0-ZFxuICBkLS0-ZVxuICBlLS0-ZlxuICBmLS0-ZyIsIm1lcm1haWQiOnsidGhlbWUiOiJkZWZhdWx0In0sInVwZGF0ZUVkaXRvciI6ZmFsc2V9)

1. Acquisizione dei dati
2. Pre-processing dei dati, preparare i dati per l'analisi, inclusa la loro pulizia e la preparazione delle feature
3. Selezionare le feature rilevanti e dividere i dati in un insieme di addestramento e un insieme di test
4. Scegliere un modello di classificazione e addestrarlo sull'insieme di addestramento
5. Valutare le prestazioni del modello sull'insieme di test per misurare la sua capacità di generalizzazione
6. Ottimizzare gli iperparametri del modello e deployarlo in produzione per fare previsioni su nuovi dati
7. Monitorare le prestazioni del modello nel tempo e aggiornarlo periodicamente per mantenerlo accurato e rilevante

### Apprendimento non supervisionato
Il **clustering** è una tecnica di apprendimento non supervisionato il cui scopo è quello di raggruppare un insieme di oggetti in cluster; gli oggetti dovranno avere un'alta similarità intra-cluster e una bassa inter-cluster. E' una tecnica molto complessa dato che i vari algoritmi imparano osservando i dati e non tramite degli esempi. Grazie a questa tecnica è possibile visualizzare il comportamento dei guidatori della flotta. L'ideale utopico sarebbe quello di avere cluster il cui valore medio che li rappresenta contenga valori di valutazione (sopra elencati) ragionevoli o che non si discostino troppo dai valori attesi del guidatore prudente.

Le fasi principali sono 5: 

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcpXG4gIGMoRGF0YSBtYXRyaXgpXG4gIGQoQ2x1c3RlcmluZyBhbGdvcml0aG1zKVxuICBlKENsdXN0ZXJzKVxuICBcbiAgYS0tPmJcbiAgYi0tPmNcbiAgYy0tSU5QVVQtLT5kXG4gIGQtLU9VVFBVVC0tPmUiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcpXG4gIGMoRGF0YSBtYXRyaXgpXG4gIGQoQ2x1c3RlcmluZyBhbGdvcml0aG1zKVxuICBlKENsdXN0ZXJzKVxuICBcbiAgYS0tPmJcbiAgYi0tPmNcbiAgYy0tSU5QVVQtLT5kXG4gIGQtLU9VVFBVVC0tPmUiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)

1. Acquisizione dei dati
2. Pre-processing composto a sua volta da più fasi, data cleaning, data integration, data reduction e data transformation
3. Ottenimento dei dati sotto forma vettoriale o matriciale
4. Applicazione dei diversi tipi di algoritmi di clustering
5. Suddivisione in cluster degli oggetti appartenenti al campione iniziale

__Di seguito vengono suggerite alcune tecniche che potrebbero essere utilizzate dalla compagnia per clusterizzare i propri clienti__:
- Clustering partizionale
- Clustering gerarchico
- Clustering basato sulla densità

##### Clustering partizionale:
Il clustering partizionale è la tecnica più comunemente utilizzata tra gli algoritmi di clusterizzazione. Questi algoritmi lavorano per minimizzare un determinato criterio di clustering, spostando iterativamente i punti dati tra i cluster fino a raggiungere una partizione ottimale. Questo approccio suddivide gli oggetti in K partizioni, ognuna delle quali rappresenta un cluster. La suddivisione avviene in base a una specifica funzione obiettivo. I cluster vengono formati per ottimizzare un criterio di partizionamento definito, come una funzione di dissimilarità basata sulla distanza, in modo che gli oggetti all'interno di un cluster siano considerati "simili", mentre quelli in cluster diversi siano considerati "dissimili". I metodi di clustering partizionale sono particolarmente adatti per applicazioni in cui è richiesto un numero predefinito di cluster, alcuni esempi di algoritmi di clustering partizionale includono K-means, PAM (Partition Around Medoids) e Clara.

![Clustering Partizionale](https://www.mathisintheair.org/wp/wp-content/uploads/2016/09/download.jpg)

##### Clustering gerarchico:
Gli algoritmi di clustering gerarchico dividono o uniscono un dataset in una sequenza di partizioni nidificate. La gerarchia delle partizioni nidificate può essere agglomerativa (bottom-up) o divisiva (top-down). Nel metodo agglomerativo, il clustering inizia con ogni singolo oggetto in un singolo cluster e continua a raggruppare le coppie di cluster più vicine fino a quando tutti gli oggetti sono insieme in un unico cluster. Il clustering gerarchico divisivo, d'altra parte, inizia con tutti gli oggetti in un singolo cluster e continua a dividere i cluster più grandi in cluster più piccoli fino a quando tutti gli oggetti sono separati in cluster unitari. Entrambi i metodi gerarchici mostrano il modo naturale di rappresentare i cluster, chiamato dendrogramma. Esempi di questi algoritmi sono ROCK, BIRCH (Balance Iterative Reducing and Clustering using Hierarchies), CURE (Cluster Using REpresentatives).

Una gerarchia di cluster può essere interpretata come un albero binario standard in cui la radice rappresenta tutti gli insiemi di oggetti dati da clusterizzare che formano il livello più alto della gerarchia (livello 0). Ad ogni livello, i nodi che sono il sottoinsieme dell'intero dataset corrispondono al cluster. Gli elementi in ciascuno di questi cluster possono essere determinati attraverso l'attraversamento dell'albero dal nodo del cluster corrente alla base singleton che sono le foglie dell'albero. Questa gerarchia di cluster è chiamata dendrogramma. Il vantaggio fondamentale di avere un metodo di clustering gerarchico è che consente di tagliare la gerarchia al livello desiderato e questa caratteristica lo rende significativamente diverso dagli altri algoritmi di clustering. Esistono diversi algoritmi di clustering agglomerativo che utilizzano misure di similarità diverse e quindi, basati su questo, sono presenti differenti algoritmi di clustering agglomerativo: linkage singolo, linkage completo, linkage medio di gruppo, linkage del centroide, criterio di Ward.

![Clustering Gerarchico](https://www.developersmaggioli.it/wp-content/uploads/2019/06/images.png)

##### Clustering basato sulla densità:
Gli algoritmi di clustering basati sulla densità sono ideati per la creazione di cluster di forma arbitraria. In questo approccio, un cluster di forma arbitraria è considerato come una regione in cui la densità degli oggetti supera una soglia. Nel Clustering density-based, il raggruppamento avviene analizzando l'intorno di ogni punto dello spazio. In particolare, viene considerata la densità di punti in un intorno di raggio fissato. DBSCAN e SSN RDBC sono algoritmi tipici di questo tipo.

![Clustering basato su densità](https://lh3.googleusercontent.com/proxy/aUUxEVBkFfnCgXYt7PPsTX52j9cXBCExuwPBaa9Tpp9dJscXauJR0FUuZznrA7CR1-iIC6pPppgppNnvddc-7sBY-aSrb8QZEmEwDne5a-KeCqXGeKDPkZbVz9aj4bSNGmNWpYB7Wc53hyuGZyMaBHivrOxIeWfXkivzWFcbzgEc1jE)

## Feedback del corretto funzionamento
Si propone di eseguire ripetutamente lo stesso percorso in automobile, affrontandolo con tre distinti stili di guida: tranquillo, moderato e brusco, al fine di valutare e confrontare gli effetti di tali stili sulla dinamica di guida.

## Conclusioni
Lo schema che seguirà di seguito indica che con il nostro sistema è possibile sfuttare la capacità valutativa della board, la quale è in grado di generare uno score del guidatore e di offrire la possibilità di valutarlo in base a questo, la possibilità di giudicarlo utilizzando tecniche di machine learning o la possibilità di utilizzare entrambe le tecniche per avere un quadro completo relativo alla sua guida:

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggVERcbiAgYShFQ1UpXG4gIGIoT0JELUlJIEFkYXB0ZXIpXG4gIGMoRHJpdmVyIFByb2ZpbGluZylcbiAgZChTY29yZSBvYnRlaW5lZCBieSBCb2FyZClcbiAgZShNYWNoaW5lIExlYXJuaW5nIHRlY25pcXVlKVxuICBcbiAgYS0tPmJcbiAgYi0tPmNcbiAgYy0tPmRcbiAgYy0tPmUiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggVERcbiAgYShFQ1UpXG4gIGIoT0JELUlJIEFkYXB0ZXIpXG4gIGMoRHJpdmVyIFByb2ZpbGluZylcbiAgZChTY29yZSBvYnRlaW5lZCBieSBCb2FyZClcbiAgZShNYWNoaW5lIExlYXJuaW5nIHRlY25pcXVlKVxuICBcbiAgYS0tPmJcbiAgYi0tPmNcbiAgYy0tPmRcbiAgYy0tPmUiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)