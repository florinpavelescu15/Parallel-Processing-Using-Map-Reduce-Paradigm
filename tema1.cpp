#include <bits/stdc++.h>

using namespace std;

/* functie care verifica daca exista un numar x
astfel incat number = x ^ exp, folosind cautarea binara;
cum cautarea se face in intervalul 1..mumber, sunt
eliminate numerele nepozitive */
bool is_perfect_power(int number, int exp)
{
    if (number == 1)
        return true;
    else
    {
        int left = 1, right = number;
        while (left < right)
        {
            int middle = (left + right) / 2;
            if (pow(middle, exp) == number)
                return true;
            if (pow(middle, exp) > number)
                right = middle;
            else
                left = middle + 1;
        }
    }
    return false;
}

// argumentul functiei de thread
typedef struct thread_arg
{
    vector<string> *files;                    // lista de fisiere de analizat
    int thread_id;                            // id-ul thread-ului curent
    int num_mappers;                          // numarul de thread-uri mapper
    int num_reducers;                         // numarul de thread-uri reducer
    vector<vector<vector<int>>> *map_results; // liste partiale pentru rezultatul operatiei map
    pthread_mutex_t *mutex;
    pthread_barrier_t *barrier;
} thread_arg;

// functia de thread
void *func(void *arg)
{
    thread_arg *t = (thread_arg *)arg;

    // daca thread-ul curent este mapper
    if (t->thread_id < t->num_mappers)
    {
        /* cat timp exista fisiere de analizat, extrag un fisier,
        ii sterg numele din lista si il prelucrez */
        string curr_file_name;

        while (!(*t->files).empty())
        {
            /* folosesc un mutex pentru a nu lasa mai multe thread-uri sa
            extraga si sa prelucreze simultan acelasi fisier din lista */
            pthread_mutex_lock(t->mutex);
            curr_file_name = (*t->files)[(*t->files).size() - 1];
            (*t->files).pop_back();
            pthread_mutex_unlock(t->mutex);

            /* citesc elementele din fisierul curent si le distribui in
            listele partiale aferente thread-urilor reducer */
            int num_elem, elem;
            FILE *curr_file = fopen(curr_file_name.c_str(), "r");
            fscanf(curr_file, "%d", &num_elem);
            for (int i = 0; i < num_elem; i++)
            {
                fscanf(curr_file, "%d", &elem);
                for (int r = 0; r < t->num_reducers; r++)
                    if (is_perfect_power(elem, r + 2))
                        (*t->map_results)[t->thread_id][r].push_back(elem);
            }
            fclose(curr_file);
        }
    }

    /* astept ca toate thread-urile mapper sa-si termine treaba;
    am initializat bariera cu valoarea M + R, deci niciun thread
    nu va trece de bariera pana cand nu ajung toate M + R aici */
    pthread_barrier_wait(t->barrier);

    // daca thread-ul curent este reducer
    if (t->thread_id >= t->num_mappers)
    {
        /* numar elementele din lista aferenta thread-ului curent
        si scriu rezultatul in fisierul corespunzator */
        unordered_set<int> reduce_final_list;
        char curr_file_name[30];
        sprintf(curr_file_name, "out%d.txt", t->thread_id - t->num_mappers + 2);
        FILE *curr_file = fopen(curr_file_name, "w");
        int i = t->thread_id - t->num_mappers;

        for (int id = 0; id < t->num_mappers; id++)
            for (int j = 0; j < (*t->map_results)[id][i].size(); j++)
                reduce_final_list.insert((*t->map_results)[id][i][j]);

        /* am folosit un unordered_set pentru a retine elementele corespunzatoare
        fiecarui thread reducer si, cum unordered_set elimina duplicatele, numarul
        puterilor perfecte distincte va fi egal cu numarul elementelor multimii */
        fprintf(curr_file, "%ld", reduce_final_list.size());
        fclose(curr_file);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int num_files, id, r;
    void *status;

    // preiau argumentele din linia de comanda
    int M = atoi(argv[1]);
    int R = atoi(argv[2]);
    char *file_name = argv[3];

    // citesc numele fisierelor si alcatuiesc lista de fisiere
    FILE *in = fopen(file_name, "r");
    fscanf(in, "%d", &num_files);
    vector<string> files;
    for (int i = 0; i < num_files; i++)
    {
        fscanf(in, "%s", file_name);
        files.push_back(string(file_name));
    }

    vector<thread_arg> arguments(M + R);
    vector<pthread_t> threads(M + R);

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, M + R);

    vector<vector<vector<int>>> map_results(M, vector<vector<int>>(R));

    // creez TOATE cele M + R thread-uri
    for (id = 0; id < M + R; id++)
    {
        arguments[id].files = &files;
        arguments[id].thread_id = id;
        arguments[id].num_mappers = M;
        arguments[id].num_reducers = R;
        arguments[id].map_results = &map_results;
        arguments[id].mutex = &mutex;
        arguments[id].barrier = &barrier;

        r = pthread_create(&threads[id], NULL, func, (void *)&arguments[id]);

        if (r)
        {
            printf("Eroare la crearea thread-ului %d\n", id);
            exit(-1);
        }
    }

    // fac TOATE cele M + R join-uri aferente
    for (id = 0; id < M + R; id++)
    {
        r = pthread_join(threads[id], NULL);

        if (r)
        {
            printf("Eroare la asteptarea thread-ului %d\n", id);
            exit(-1);
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);

    return 0;
}
