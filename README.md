# Procesare paralela folosind paradigma Map-Reduce
ALGORITMI PARALELI SI DISTRIBUITI,
Tema #1,
Pavelescu Florin, 334CC
## PREAMBUL
Rezolvarea temei mi-a luat aproximativ 5 ore. Tema mi s-a parut foarte 
interesanta si accesibila, fiind foarte asemanatoare cu cerintele din
cadrul laboratoarelor. Am urmat intocmai indicatiile din cerinta: 
toate cele `M + R` thread-uri au fost create la inceput, am folosit exact
`M + R` thread-uri, nu am folosit variabile globale, am urmat paradigma
Map-Reduce.

## DETALII DE IMPLEMENTARE
### Structura data ca argument functiei de thread
```
typedef struct thread_arg                          
{                                                  
	vector<string> *files;                         
	int thread_id;                                 
	int num_mappers;                               
	int num_reducers;                              
	vector<vector<vector<int>>> *map_results;	   
	pthread_mutex_t *mutex;                       
	pthread_barrier_t *barrier;                   
} thread_arg;
```
- `files` = vector ce contine numele fisierelor ce vor fi analizate;
- `thread_id` = indexul thread-ului curent;
- `num_mappers` = numarul de thread-uri mapper;
- `num_reducers` = numarul de thread-uri reducer;
- `map_results` = vector tridimensional de numere intregi; pentru fiecare
mapper am o vector in care tin cate un vector de numere corespunzatoare
fiecaruia dintre cei `R` reduceri;
- `mutex` = mutex folosit pentru evitarea race condition la distribuirea
fisierelor catre thread-uri;
- `barrier` = bariera folosita pentru a astepta thread-urile mapper sa-si
termine treaba, inainte ca thread-urile reducer sa inceapa procesarea.

**Observatie:** `files` si `map_results` sunt structuri de date partajate intre
thread-uri; toate threadurile pot efectua modificari asupra lor, iar
aceste modificari sunt persistente si in afara functiei de thread, 
gratie pointerilor ele comportandu-se ca niste variabile globale.

### Functia de thread
```
┌func
|   ┌if (thread-ul curent e mapper)
|   |    ┌while (mai am fisiere de prelucrat)
|   |    |     // nu las mai multe thread-uri sa preia fisiere simultan
|   |    |     mutex_lock(mutex)
|   |    |     extrag fisierul din lista
|   |    |     mutex_unlock(mutex)
|   |    |     deschid file = fisierul extras
|   |    |     ┌for (number in file)
|   |    |     |    ┌for (r = 0..R - 1)
|   |    |     |    |   ┌if (exista x a. i. number = x ^ (r + 2))
|   |    |     |    |   |   adaug number in lista corespunzatoare lui r
|   |    |     |    |   └■
|   |    |     |    └■
|   |    |     └■
|   |    └■
|   └■
|   /* nu las thread-urile sa treaca mai departe pana nu ajung toate aici
|   (thread-urile reducer ajung direct, cele mapper atunci cand nu mai
|   sunt fisiere de procesat) */
|   barrier_wait(barrier(M + R))
|
|   ┌if (thread-ul curent e reducer)
|   |    r = thread_id - M
|   |    ┌for (m = 0..M - 1)
|   |    |   adaug in lista list(r) numerele din lista
|   |    |   corespunzatoare lui r creata de thread-ul m
|   |    └■
|   |    N = numarul de numere distincte din list(r)
|   |    scriu N in fisierul de iesire corespunzator
|   └■
└■
```
**Observatie:** am o singura functie de thread atat pentru thread-urile mapper,
cat si pentru cele reducer, diferenta intre cele doua tipuri de thread-uri
se face pe baza `thread_id`-ului (`0 <= id_mapper < M <= id reducer < M + R`).

### Functia care verifica daca un numar e putere perfecta
Folosesc cautarea binara pentru a verifica daca exista un numar natural `x`,
astfel incat `number = x ^ exp`. Evident, cautarea se face in intervalul `1..number`.

### Functia `main`:
Preiau numarul de thread-uri mapper si numarul de thread-uri reduceri din
linia de comanda, precum si fisierul ce contine numele fisierelor cu date.
Citesc numele fisierelor si le adaug in vectorul de stringuri files.
Initializez mutex-ul si bariera folosite pentru sincronizarea thread-urilor.
De asemenea, am creat exact `M + R` thread-uri si am facut join-urile aferente
acestora.

Mai multe detalii despre implementare se gasesc in comentariile din cod.
