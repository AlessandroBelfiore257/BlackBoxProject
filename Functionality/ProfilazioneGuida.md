# Profilazione guida
parte relativa alla profilazione sarà aggiunta in seguito..
## Tecniche di clustering:
Una volta processati i dati relativi all'analisi di profilazione del guidatore, essi possono essere inviati ad un server remoto. Dopodichè posso essere usate varie tecniche di clustering per raggruppare i clienti dell'autonoleggio in cluster e classificarli come prudenti o negligenti.
Il clustering è una tecnica di apprendimento non supervisionato il cui scopo è quello di raggruppare un insieme di oggetti in cluster; gli oggetti dovranno avere un'alta similarità intra-cluster e una bassa inter-cluster. E' una tecnica molto complessa dato che i vari algoritmi imparano osservando i dati e non tramite degli esempi.

[![](https://mermaid.ink/img/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcpXG4gIGMoRGF0YSBtYXRyaXgpXG4gIGQoQ2x1c3RlcmluZyBhbGdvcml0aG1zKVxuICBlKENsdXN0ZXJzKVxuICBcbiAgYS0tPmJcbiAgYi0tPmNcbiAgYy0tSU5QVVQtLT5kXG4gIGQtLU9VVFBVVC0tPmUiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)](https://workflow.jace.pro/#/edit/eyJjb2RlIjoiZ3JhcGggTFJcbiAgYShEYXRhIHJlYWwgZGlydHkpXG4gIGIoUHJlLXByb2Nlc3NpbmcpXG4gIGMoRGF0YSBtYXRyaXgpXG4gIGQoQ2x1c3RlcmluZyBhbGdvcml0aG1zKVxuICBlKENsdXN0ZXJzKVxuICBcbiAgYS0tPmJcbiAgYi0tPmNcbiAgYy0tSU5QVVQtLT5kXG4gIGQtLU9VVFBVVC0tPmUiLCJtZXJtYWlkIjp7InRoZW1lIjoiZGVmYXVsdCJ9LCJ1cGRhdGVFZGl0b3IiOmZhbHNlfQ)

Seguendo il processo sopra raffigurato, la fase di clustering è lo step che precede la fase finale del nostro studio, ovvero la clasterizzazione.
Le fasi principali sono 5: 
1. Acquisizione dei dati
2. Pre-processing composto a sua volta da più fasi, data cleaning, data integration, data reduction e data transformation
3. Ottenimento dei dati sotto forma vettoriale o matriciale
4. Applicazione dei diversi tipi di algoritmi di clustering
5. Suddivisione in cluster degli oggetti appartenenti al campione iniziale

Sono presenti diverse tecniche di clusterizzazione:
- Clustering partizionale
- Clustering gerarchico
- Cluster basato sulla densità

### Clustering partizionale:
Il clustering partizionale è considerata la tecnica più popolare tra gli algoritmi di clusterizzazione. Questi algoritmi minimizzano un criterio di classificazione dato attraverso un'iterativa ricollocazione degli oggetti tra cluster fino a quando non viene raggiunta una partizione ottima. Questo tipo di clusterizzazione divide gli oggetti in K partizioni, dove ciascuna rappresenta un cluster. Esempi di algoritmi di questa categoria sono K-means, PAM (Partition around mediods) e clara.

K-means:
K = n. di cluster da sapere a priori. Ogni cluster viene rappresentato dal centroide ovvero dal valore medio dei punti di un cluster. Nel k-medoids (PAM) ogni cluster viene rappresentato da un punto nel cluster detto medoide. 
Funzionamento dell'algoritmo:
- Scegliere casualmente una partizione -> calcolare i centroidi
- Ripeti finchè i centroidi non cambiano più:
    1. Riassegnare ogni oggetto al cluster a cui è più simile (al centroide più vicino)
    2. Aggiornare i centroidi

### Clustering gerarchico:
Gli algoritmi di clustering gerarchico dividono o uniscono un set di dati in una sequenza di partizioni nidificate. La gerarchia delle partizioni nidificate può essere agglomerativa (dal basso verso l'alto) o divisiva (dall'alto verso il basso). Nel metodo agglomerativo, il clustering inizia con un singolo oggetto in un singolo cluster e continua a raggruppare le coppie di cluster più vicine finchè tutti gli oggetti non sono insieme in un unico cluster. Il clustering gerarchico divisivo, d'altra parte, inizia con tutti gli oggetti in un singolo cluster e continua a dividere i cluster più grandi in cluster più piccoli finchè tutti gli oggetti non vengono separati in cluster unitari.

Struttura dati sfruttata: dendogramma, in questo approccio non serve sapere k a priori. Posso tagliare la struttura dove voglio per ottenere quanti k voglio (a posteriori)

Agnes & Diana (aggomerativo e a partizione):
Agnes:
L'algoritmo continua finchè tutti i punti sono uniti in un unico grande cluster.
Diana:
Considero inizialmente tutti i punti in un unico cluster.
1. Trova l'oggetto che ha la più alta dissimilarità media rispetto a tutti gli altri (Questo oggetto inizializza il nuovo cluster)
2. Per ogni oggetto fuori dallo splinter group calcolare una distanza di Di
3. Trova l'oggetto h per cui Dh è la più grande. Se Dh > 0 allora h è più vicino allo splinter group (sposto l'elemento nel nuovo cluster e aggiorno tutte le distanze)
4. Continuo fin quando tutte le distanze Dh sono negative (Dataset diviso in 2)
5. Seleziono il cluster con diametro più grande (+ grande distanza tra qualsiasi coppia di oggetti al suo interno)
6. Riapplico il criterio precedente per dividere questo nuovo cluster
7. ripeto finchè ogni cluster contiene solo 1 oggetto

### Clustering basato sulla densità:
Gli algoritmi di clustering basati sulla densità sono ideati per la creazione di cluster di forma arbitraria. In questo approccio, un cluster di forma arbitraria è considerato come una regione in cui la densità degli oggetti supera una soglia. DBSCAN e SSN RDBC sono algoritmi tipici di questo tipo.

DBSCAN:


Ambito applicativo del clustering: Marketing -> gruppi di clienti