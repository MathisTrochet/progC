#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"
#include "myassert.h"

#include "client_master.h"

#include <fcntl.h>//  rajouté
#include <pthread.h> // rajouté
#include <sys/ipc.h> // ajouté 
#include <sys/sem.h> // ajouté 



/************************************************************************
 * chaines possibles pour le premier paramètre de la ligne de commande
 ************************************************************************/
#define TK_STOP        "stop"             // arrêter le master
#define TK_HOW_MANY    "howmany"          // combien d'éléments dans l'ensemble
#define TK_MINIMUM     "min"              // valeur minimale de l'ensemble
#define TK_MAXIMUM     "max"              // valeur maximale de l'ensemble
#define TK_EXIST       "exist"            // test d'existence d'un élément, et nombre d'exemplaires
#define TK_SUM         "sum"              // somme de tous les éléments
#define TK_INSERT      "insert"           // insertion d'un élément
#define TK_INSERT_MANY "insertmany"       // insertions de plusieurs éléments aléatoires
#define TK_PRINT       "print"            // debug : demande aux master/workers d'afficher les éléments
#define TK_LOCAL       "local"            // lancer un calcul local (sans master) en multi-thread


/************************************************************************
 * structure stockant les paramètres du client
 * - les infos pour communiquer avec le master
 * - les infos pour effectuer le travail (cf. ligne de commande)
 *   (note : une union permettrait d'optimiser la place mémoire) 
 ************************************************************************/
typedef struct {
    // communication avec le master
    int tubeCM;
    int tubeMC;
    int orderReturn;
    int sem2;
    //TODO
    // infos pour le travail à faire (récupérées sur la ligne de commande)
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL
} Data;


/************************************************************************
 * Usage
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usages : %s <ordre> [[[<param1>] [<param2>] ...]]\n", exeName);
    fprintf(stderr, "   $ %s " TK_STOP "\n", exeName);
    fprintf(stderr, "          arrêt master\n");
    fprintf(stderr, "   $ %s " TK_HOW_MANY "\n", exeName);
    fprintf(stderr, "          combien d'éléments dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MINIMUM "\n", exeName);
    fprintf(stderr, "          plus petite valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_MAXIMUM "\n", exeName);
    fprintf(stderr, "          plus grande valeur de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_EXIST " <elt>\n", exeName);
    fprintf(stderr, "          l'élement <elt> est-il présent dans l'ensemble ?\n");
    fprintf(stderr, "   $ %s " TK_SUM "\n", exeName);
    fprintf(stderr, "           somme des éléments de l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT " <elt>\n", exeName);
    fprintf(stderr, "          ajout de l'élement <elt> dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_INSERT_MANY " <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          ajout de <nb> élements (dans [<min>,<max>[) aléatoires dans l'ensemble\n");
    fprintf(stderr, "   $ %s " TK_PRINT "\n", exeName);
    fprintf(stderr, "          affichage trié (dans la console du master)\n");
    fprintf(stderr, "   $ %s " TK_LOCAL " <nbThreads> <elt> <nb> <min> <max>\n", exeName);
    fprintf(stderr, "          combien d'exemplaires de <elt> dans <nb> éléments (dans [<min>,<max>[)\n"
                    "          aléatoires avec <nbThreads> threads\n");

    if (message != NULL)
        fprintf(stderr, "message :\n    %s\n", message);

    exit(EXIT_FAILURE);
}


/************************************************************************
 * Analyse des arguments passés en ligne de commande
 ************************************************************************/
static void parseArgs(int argc, char * argv[], Data *data)
{
    data->order = CM_ORDER_NONE;

    if (argc == 1)
        usage(argv[0], "Il faut préciser une commande");

    // première vérification : la commande est-elle correcte ?
    if (strcmp(argv[1], TK_STOP) == 0)
        data->order = CM_ORDER_STOP;
    else if (strcmp(argv[1], TK_HOW_MANY) == 0)
        data->order = CM_ORDER_HOW_MANY;
    else if (strcmp(argv[1], TK_MINIMUM) == 0)
        data->order = CM_ORDER_MINIMUM;
    else if (strcmp(argv[1], TK_MAXIMUM) == 0)
        data->order = CM_ORDER_MAXIMUM;
    else if (strcmp(argv[1], TK_EXIST) == 0)
        data->order = CM_ORDER_EXIST;
    else if (strcmp(argv[1], TK_SUM) == 0)
        data->order = CM_ORDER_SUM;
    else if (strcmp(argv[1], TK_INSERT) == 0)
        data->order = CM_ORDER_INSERT;
    else if (strcmp(argv[1], TK_INSERT_MANY) == 0)
        data->order = CM_ORDER_INSERT_MANY;
    else if (strcmp(argv[1], TK_PRINT) == 0)
        data->order = CM_ORDER_PRINT;
    else if (strcmp(argv[1], TK_LOCAL) == 0)
        data->order = CM_ORDER_LOCAL;
    else
        usage(argv[0], "commande inconnue");

    // deuxième vérification : nombre de paramètres correct ?
    if ((data->order == CM_ORDER_STOP) && (argc != 2))
        usage(argv[0], TK_STOP " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_HOW_MANY) && (argc != 2))
        usage(argv[0], TK_HOW_MANY " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MINIMUM) && (argc != 2))
        usage(argv[0], TK_MINIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_MAXIMUM) && (argc != 2))
        usage(argv[0], TK_MAXIMUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_EXIST) && (argc != 3))
        usage(argv[0], TK_EXIST " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_SUM) && (argc != 2))
        usage(argv[0], TK_SUM " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_INSERT) && (argc != 3))
        usage(argv[0], TK_INSERT " : il faut un et un seul argument après la commande");
    if ((data->order == CM_ORDER_INSERT_MANY) && (argc != 5))
        usage(argv[0], TK_INSERT_MANY " : il faut 3 arguments après la commande");
    if ((data->order == CM_ORDER_PRINT) && (argc != 2))
        usage(argv[0], TK_PRINT " : il ne faut pas d'argument après la commande");
    if ((data->order == CM_ORDER_LOCAL) && (argc != 7))
        usage(argv[0], TK_LOCAL " : il faut 5 arguments après la commande");

    // extraction des arguments
    if (data->order == CM_ORDER_EXIST)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT)
    {
        data->elt = strtof(argv[2], NULL);
    }
    else if (data->order == CM_ORDER_INSERT_MANY)
    {
        data->nb = strtol(argv[2], NULL, 10);
        data->min = strtof(argv[3], NULL);
        data->max = strtof(argv[4], NULL);
        if (data->nb < 1)
            usage(argv[0], TK_INSERT_MANY " : nb doit être strictement positif");
        if (data->max < data->min)
            usage(argv[0], TK_INSERT_MANY " : max ne doit pas être inférieur à min");
    }
    else if (data->order == CM_ORDER_LOCAL)
    {
        data->nbThreads = strtol(argv[2], NULL, 10);
        data->elt = strtof(argv[3], NULL);
        data->nb = strtol(argv[4], NULL, 10);
        data->min = strtof(argv[5], NULL);
        data->max = strtof(argv[6], NULL);
        if (data->nbThreads < 1)
            usage(argv[0], TK_LOCAL " : nbThreads doit être strictement positif");
        if (data->nb < 1)
            usage(argv[0], TK_LOCAL " : nb doit être strictement positif");
        if (data->max <= data->min)
            usage(argv[0], TK_LOCAL " : max ne doit être strictement supérieur à min");
    }
}


/************************************************************************
 * Partie multi-thread
 ************************************************************************/
//TODO Une structure pour les arguments à passer à un thread (aucune variable globale autorisée)
typedef struct {
    const float* tab;
    pthread_mutex_t* thread;
    int min;
    int max;
    float elt;
    int *result;
}ThreadData;

//TODO
void * countElt(void * arg){
    ThreadData *data = (ThreadData *) arg;
    int count = 0;
    pthread_mutex_lock(data->thread);
    
    for(int i=data->min; i <= data->max; i++){
        if(data->tab[i] == data->elt){
            count++;
        }
    }
    *(data->result) += count;
    pthread_mutex_unlock(data->thread);
    pthread_exit(NULL);
    return NULL;
}
// Code commun à tous les threads
// Un thread s'occupe d'une portion du tableau et compte en interne le nombre de fois
// où l'élément recherché est présent dans cette portion. On ajoute alors,
// en section critique, ce nombre au compteur partagé par tous les threads.
// Le compteur partagé est la variable "result" de "lauchThreads".
// A vous de voir les paramètres nécessaires  (aucune variable globale autorisée)
//END TODO



void lauchThreads(const Data *data)
{
    //TODO déclarations nécessaires : mutex, ...
    int result = 0;
    float * tab = ut_generateTab(data->nb, data->min, data->max, 0);
    int nb_thread = data->nbThreads;

    pthread_t threadID[nb_thread];
    ThreadData Datas[nb_thread];
    pthread_mutex_t thread = PTHREAD_MUTEX_INITIALIZER;
    int sizetab = data->nb;

    int min = 0;
    int max = (sizetab / nb_thread) -1;

    for (int i = 0; i < nb_thread; i++)
    {
        Datas[i].result = &result;
        Datas[i].elt = data->elt;
        Datas[i].tab = tab;
        Datas[i].thread = &thread;
        if(i == (nb_thread-1)){
            Datas[i].min = Datas[i-1].max + 1;
            Datas[i].max = sizetab;
        }
        else{
            Datas[i].min = min;
            Datas[i].max = max;

            min = max +1;
            max = min + (sizetab / nb_thread) -1;
        }
    }


    //TODO lancement des threads
    for(int i=0; i < nb_thread; i++){
        int ret = pthread_create(&threadID[i], NULL, countElt, &(Datas[i])); //!!!!!!!!!!!
        myassert(ret == 0, "Erreur : lancement des threads");
    }
    
    //TODO attente de la fin des threads
    for(int i=0; i < nb_thread; i++){
        int ret = pthread_join(threadID[i], NULL); 
        myassert(ret == 0, "Erreur : attente de la fin des threads");
    }

    // résultat (result a été rempli par les threads)
    // affichage du tableau si pas trop gros
    if (data->nb <= 20)
    {
        printf("[");
        for (int i = 0; i < data->nb; i++)
        {
            if (i != 0)
                printf(" ");
            printf("%g", tab[i]);
        }
        printf("]\n");
    }
    // recherche linéaire pour vérifier
    int nbVerif = 0;
    for (int i = 0; i < data->nb; i++)
    {
        if (tab[i] == data->elt)
            nbVerif ++;
    }
    printf("Elément %g présent %d fois (%d attendu)\n", data->elt, result, nbVerif);
    if (result == nbVerif)
        printf("=> ok ! le résultat calculé par les threads est correct\n");
    else
        printf("=> PB ! le résultat calculé par les threads est incorrect\n");

    //TODO libération des ressources    
    //pthread_mutex_destroy(&threadID);
    free(tab);
}


/************************************************************************
 * Partie communication avec le master
 ************************************************************************/
// envoi des données au master
void sendData(const Data *data)
{    

    int tubeClientMaster = data->tubeCM;
    // - envoi de l'ordre au master (cf. CM_ORDER_* dans client_master.h)
    int order = data->order;
    int ret = write(tubeClientMaster, &order, sizeof(int));
    myassert(ret!=1, "write erreur");


    if (order == CM_ORDER_INSERT || order == CM_ORDER_EXIST){
        Parametres par;
        par.elt = data->elt;
        write(tubeClientMaster, &par, sizeof(par));
    }
    if (order == CM_ORDER_INSERT_MANY){
        Parametres par;
        par.nb = data->nb;
        par.min = data->min;
        par.max = data->max;
        write(tubeClientMaster, &par, sizeof(par));
        
    }
    /*
    else if (order == CM_ORDER_LOCAL){
        Parametres par;
        par.elt = data->elt;
        par.nb = data->nb;
        par.min = data->min;
        par.max = data->max;
        par.nbThreads = data->nbThreads;
        write(tubeClientMaster, &par, sizeof(par));
    }
    */


    
    // - envoi des paramètres supplémentaires au master (pour CM_ORDER_EXIST,
    //   CM_ORDER_INSERT et CM_ORDER_INSERT_MANY)
    //END TODO
}

// attente de la réponse du master
void receiveAnswer(const Data *data)
{

    int tubeMasterClient = data->tubeMC;

    //TODO
    // - récupération de l'accusé de réception du master (cf. CM_ANSWER_* dans client_master.h)
    int order;
    float param;
    int how;
    int ret = read(tubeMasterClient, &order, sizeof(int));
    myassert(ret!=1, "read erreur");

    if (order == CM_ANSWER_MINIMUM_OK || order == CM_ANSWER_MAXIMUM_OK  || order == CM_ANSWER_SUM_OK){
        int ret = read(tubeMasterClient, &param, sizeof(float));
        myassert(ret!=1, "read erreur");
        printf("[CLIENT] résultat : %g\n", param);
    } 
    if (order == CM_ANSWER_HOW_MANY_OK ){
        int ret = read(tubeMasterClient, &how, sizeof(int));
        myassert(ret!=1, "read erreur");
        printf("[CLIENT] résultat : %d\n", how);
    }

    printf("[CLIENT] accusé de reception : %d \n", order);

    // - selon l'ordre et l'accusé de réception :
    //      . récupération de données supplémentaires du master si nécessaire
    // - affichage du résultat  
    
    //END TODO
}


/************************************************************************
 * Fonction principale
 ************************************************************************/
int main(int argc, char * argv[])
{
    Data data;
    //DataMiddle dataMiddle;
    parseArgs(argc, argv, &data);

    if (data.order == CM_ORDER_LOCAL)
        lauchThreads(&data);
    else
    {

        int ret, sem1, sem2, key1, key2;

        key1 = ftok(MON_FICHIER, MA_CLE1);
        sem1 = semget(key1, 1, 0); // on recupere le semaphore pour la section critique (sem1)

        key2 = ftok(MON_FICHIER, MA_CLE2);
        sem2 = semget(key2, 1, 0); // on recupere le semaphore pour bloquer la loop

        data.sem2 = sem2;

        //TODO
        // - entrer en section critique 

        struct sembuf operation = {0, -1, 0};
        ret = semop(sem1, &operation, 1);
        myassert(ret!=1, "read erreur");

        
        //       . pour empêcher que 2 clients communiquent simultanément
        //       . le mutex est déjà créé par le master
        // - ouvrir les tubes nommés (ils sont déjà créés par le master)


        int tubeClientMaster = open("tubeCM", O_WRONLY);
        myassert(ret!=1, "open erreur");


        int tubeMasterClient = open("tubeMC", O_RDONLY);
        myassert(ret!=1, "open erreur");

        data.tubeCM = tubeClientMaster;
        data.tubeMC = tubeMasterClient;

        //       . les ouvertures sont bloquantes, il faut s'assurer que
        //         le master ouvre les tubes dans le même ordre
        //END TODO

        sendData(&data);
        receiveAnswer(&data);

        //TODO        

        // - libérer les ressources (fermeture des tubes, ...)
        ret = close(tubeClientMaster);
        myassert(ret!=1, "close erreur");
        ret = close(tubeMasterClient);      //
        myassert(ret!=1, "close erreur");
        
        // - débloquer le master grâce à un second sémaphore (cf. ci-dessous)

        struct sembuf operation2 = {0, +1, 0};
        ret = semop(sem2, &operation2, 1);
        myassert(ret!=1, "operation2 erreur");

        // - sortir de la section critique

        struct sembuf operation1 = {0, +1, 0};
        ret = semop(sem1, &operation1, 1);
        myassert(ret!=1, "operation1 erreur");

        
        // Une fois que le master a envoyé la réponse au client, il se bloque
        // sur un sémaphore ; le dernier point permet donc au master de continuer
        //
        // N'hésitez pas à faire des fonctions annexes ; si la fonction main
        // ne dépassait pas une trentaine de lignes, ce serait bien.
    }
    
    return EXIT_SUCCESS;
}
