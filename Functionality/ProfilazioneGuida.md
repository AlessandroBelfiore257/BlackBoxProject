# Profilazione guida
Parte relativa alla profilazione sarà aggiunta in seguito...
## Machine learning:
Una volta processati i dati relativi all'analisi di profilazione del guidatore, essi possono essere inviati ad un server remoto. Dopodichè è possibile usare varie tecniche di machine learning di apprendiamento supervisionato ad esempio la **classificazione** o di apprendimento non supervisionato ad esempio il **clustering**, per assegnare ai clienti dell'autonoleggio una certa etichetta di classificazione (prudenti o negligenti) o raggrupparli in cluster e osservare il comportamento della flotta.
### Apprendimento supervisionato
La **classificazione** è una tecnica di apprendimento supervisionato, l'obiettivo è quello di predire la classe di appartenenza di un'istanza di input in un insieme discreto di classi. Ad esempio, classificare se una mail è spam o non spam, o se un'immagine contiene un gatto o un cane. Nel nosto caso di studio vorremmo poter classificare i clienti come guidatori prudenti o negligenti in base ai valori di valutazione studiati durante l'analisi di profilazione. Questa tecnica di ML prevede la suddivisione dei dati in due insiemi distinti: un insieme di addestramento e un insieme di test. L'insieme di addestramento viene utilizzato per addestrare il modello, mentre l'insieme di test viene utilizzato per valutarne le prestazioni su dati non visti. Per addestrare un modello di apprendimento supervisionato, vengono utilizzati diversi algoritmi in base al tipo di problema e al tipo di dati. Alcuni esempi di algoritmi comuni includono Support Vector Machines (SVM), Random Forest, Decision Trees, Neural Networks, e Regressione Lineare. Dopo l'addestramento, il modello viene valutato utilizzando l'insieme di test. Una volta soddisfatti delle prestazioni del modello, esso può essere pubblicato, implementato in un'applicazione o integrato in un sistema più ampio per fare previsioni su nuovi dati.

Di seguito un esempio di classificazione binaria:
![Classificazione](https://www.andreaminini.com/data/andreaminini/apprendimento-supervisionato-esempio-1.gif)
Se nel nostro studio si volesse classificare il guidatore in più classi (prudente, moderato, negligente) ad esempio per stipulare un contratto assicurativo in base alla guida, esiste la classificazione multiclasse dove ci sono più di due classi possibili tra cui scegliere; ogni oggetto può essere assegnato a una sola classe tra le possibili.

Le fasi principali sono 7:

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcgZGVpIGRhdGkpXG4gIGMoRGl2aXNpb25lIGRlaSBkYXRpKVxuICBkKEFkZGVzdHJhbWVudG8gZGVsIG1vZGVsbG8pXG4gIGUoVmFsdXRhemlvbmUgZGVsIG1vZGVsbG8pXG4gIGYoVHVuaW5nIGRlZ2xpIGlwZXJwYXJhbWV0cmkgZSBkZXBsb3kgZGVsIG1vZGVsbG8pXG4gIGcoTW9uaXRvcmFnZ2lvIGUgYWdnaW9ybmFtZW50bylcblxuICBhLS0-YlxuICBiLS0-Y1xuICBjLS0-ZFxuICBkLS0-ZVxuICBlLS0-ZlxuICBmLS0-ZyIsIm1lcm1haWQiOnsidGhlbWUiOiJkZWZhdWx0In0sInVwZGF0ZUVkaXRvciI6ZmFsc2V9)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcgZGVpIGRhdGkpXG4gIGMoRGl2aXNpb25lIGRlaSBkYXRpKVxuICBkKEFkZGVzdHJhbWVudG8gZGVsIG1vZGVsbG8pXG4gIGUoVmFsdXRhemlvbmUgZGVsIG1vZGVsbG8pXG4gIGYoVHVuaW5nIGRlZ2xpIGlwZXJwYXJhbWV0cmkgZSBkZXBsb3kgZGVsIG1vZGVsbG8pXG4gIGcoTW9uaXRvcmFnZ2lvIGUgYWdnaW9ybmFtZW50bylcblxuICBhLS0-YlxuICBiLS0-Y1xuICBjLS0-ZFxuICBkLS0-ZVxuICBlLS0-ZlxuICBmLS0-ZyIsIm1lcm1haWQiOnsidGhlbWUiOiJkZWZhdWx0In0sInVwZGF0ZUVkaXRvciI6ZmFsc2V9)

1. Acquisizione dei dati
2. Raccolta e pre-processing dei dati: Acquisire e preparare i dati per l'analisi, inclusa la pulizia dei dati e la preparazione delle feature
3. Selezione e divisione dei dati: Selezionare le feature rilevanti e dividere i dati in un insieme di addestramento e un insieme di test
4. Scelta e addestramento del modello: Scegliere un modello di classificazione e addestrarlo sull'insieme di addestramento
5. Valutazione del modello: Valutare le prestazioni del modello sull'insieme di test per misurare la sua capacità di generalizzazione
6. Tuning degli iperparametri e deploy del modello: Ottimizzare gli iperparametri del modello e deployarlo in produzione per fare previsioni su nuovi dati
7. Monitoraggio e aggiornamento: Monitorare le prestazioni del modello nel tempo e aggiornarlo periodicamente per mantenerlo accurato e rilevante

### Apprendimento non supervisionato
Il clustering è una tecnica di apprendimento non supervisionato il cui scopo è quello di raggruppare un insieme di oggetti in cluster; gli oggetti dovranno avere un'alta similarità intra-cluster e una bassa inter-cluster. E' una tecnica molto complessa dato che i vari algoritmi imparano osservando i dati e non tramite degli esempi. Grazie a questa tecnica è possibile visualizzare il comportamento dei clienti della flotta. L'ideale utopico sarebbe quello di avere cluster il cui valore medio che li rappresenta contenga valori di valutazione (sopra elencati) ragionevoli o che non si discostino troppo dai valori attesi del guidatore prudente.

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
- Cluster basato sulla densità

### Clustering partizionale:
Il clustering partizionale è la tecnica più comunemente utilizzata tra gli algoritmi di clusterizzazione. Questi algoritmi lavorano per minimizzare un determinato criterio di clustering, spostando iterativamente i punti dati tra i cluster fino a raggiungere una partizione ottimale. Questo approccio suddivide gli oggetti in K partizioni, ognuna delle quali rappresenta un cluster. La suddivisione avviene in base a una specifica funzione obiettivo. I cluster vengono formati per ottimizzare un criterio di partizionamento definito, come una funzione di dissimilarità basata sulla distanza, in modo che gli oggetti all'interno di un cluster siano considerati "simili", mentre quelli in cluster diversi siano considerati "dissimili". I metodi di clustering partizionale sono particolarmente adatti per applicazioni in cui è richiesto un numero predefinito di cluster, alcuni esempi di algoritmi di clustering partizionale includono K-means, PAM (Partition Around Medoids) e Clara.
![Clustering Partizionale](https://www.mathisintheair.org/wp/wp-content/uploads/2016/09/download.jpg)

### Clustering gerarchico:
Gli algoritmi di clustering gerarchico dividono o uniscono un dataset in una sequenza di partizioni nidificate. La gerarchia delle partizioni nidificate può essere agglomerativa (bottom-up) o divisiva (top-down). Nel metodo agglomerativo, il clustering inizia con ogni singolo oggetto in un singolo cluster e continua a raggruppare le coppie di cluster più vicine fino a quando tutti gli oggetti sono insieme in un unico cluster. Il clustering gerarchico divisivo, d'altra parte, inizia con tutti gli oggetti in un singolo cluster e continua a dividere i cluster più grandi in cluster più piccoli fino a quando tutti gli oggetti sono separati in cluster unitari. Entrambi i metodi gerarchici mostrano il modo naturale di rappresentare i cluster, chiamato dendrogramma. Esempi di questi algoritmi sono ROCK, BIRCH (Balance Iterative Reducing and Clustering using Hierarchies), CURE (Cluster Using REpresentatives).

Una gerarchia di cluster può essere interpretata come un albero binario standard in cui la radice rappresenta tutti gli insiemi di oggetti dati da clusterizzare che formano il livello più alto della gerarchia (livello 0). Ad ogni livello, i nodi che sono il sottoinsieme dell'intero dataset corrispondono al cluster. Gli elementi in ciascuno di questi cluster possono essere determinati attraverso l'attraversamento dell'albero dal nodo del cluster corrente alla base singleton che sono le foglie dell'albero. Questa gerarchia di cluster è chiamata dendrogramma. Il vantaggio fondamentale di avere un metodo di clustering gerarchico è che consente di tagliare la gerarchia al livello desiderato e questa caratteristica lo rende significativamente diverso dagli altri algoritmi di clustering. Esistono diversi algoritmi di clustering agglomerativo che utilizzano misure di similarità diverse e quindi, basati su questo, sono presenti differenti algoritmi di clustering agglomerativo: linkage singolo, linkage completo, linkage medio di gruppo, linkage del centroide, criterio di Ward.
![Clustering Gerarchico](https://www.developersmaggioli.it/wp-content/uploads/2019/06/images.png)

### Clustering basato sulla densità:
Gli algoritmi di clustering basati sulla densità sono ideati per la creazione di cluster di forma arbitraria. In questo approccio, un cluster di forma arbitraria è considerato come una regione in cui la densità degli oggetti supera una soglia. Nel Clustering density-based, il raggruppamento avviene analizzando l'intorno di ogni punto dello spazio. In particolare, viene considerata la densità di punti in un intorno di raggio fissato. DBSCAN e SSN RDBC sono algoritmi tipici di questo tipo.
![Clustering basato su densità](https://lh3.googleusercontent.com/proxy/aUUxEVBkFfnCgXYt7PPsTX52j9cXBCExuwPBaa9Tpp9dJscXauJR0FUuZznrA7CR1-iIC6pPppgppNnvddc-7sBY-aSrb8QZEmEwDne5a-KeCqXGeKDPkZbVz9aj4bSNGmNWpYB7Wc53hyuGZyMaBHivrOxIeWfXkivzWFcbzgEc1jE)