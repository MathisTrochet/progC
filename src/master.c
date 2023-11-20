#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utils.h"
#include "myassert.h"
#include "assert.h" //rajouté  

#include "client_master.h"
#include "master_worker.h"

#include <fcntl.h>//  rajouté
#include <pthread.h> // rajouté
#include <sys/ipc.h> // ajouté 
#include <sys/sem.h> // ajouté 


/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
typedef struct
{
    // communication avec le client
    int tubeCM;
    int tubeMC;
    int sem;
    int tubeMW;
    
    // données internes
    
    // communication avec le premier worker (double tubes)
    // communication en provenance de tous les workers (un seul tube en lecture)

} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/************************************************************************
 * initialisation complète
 ************************************************************************/
void init(Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");
  
    (*data).tubeCM = mkfifo("tubeCM", 0644);
    myassert(data->tubeCM == 0, "Création du tube");

    (*data).tubeMC = mkfifo("tubeMC", 0644);
    myassert(data->tubeMC == 0, "Création du tube");
    

    //TODO initialisation data
}


/************************************************************************
 * fin du master
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    int *fds[2];

    int order = CM_ANSWER_STOP_OK;

    int ret = write(fds[1], order, sizeof(int));
    assert(ret != 1);
     

    // - traiter le cas ensemble vide (pas de premier worker)
    
    
    
    
    // - envoyer au premier worker ordre de fin (cf. master_worker.h)
    // - attendre sa fin
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    int tubeMasterClient = data->tubeMC;
    
    ret = write(tubeMasterClient, &order, sizeof(int));
    assert(ret != -1);
    
    //END TODO
}


/************************************************************************
 * quel est la cardinalité de l'ensemble
 ************************************************************************/
void orderHowMany(Data *data)
{
  int order = CM_ANSWER_HOW_MANY_OK;
  int results;
    TRACE0("[master] ordre how many\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    if (true){
       results = 0;
    }   
    // - envoyer au premier worker ordre howmany (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultats (deux quantités) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    int tubeMasterClient = data->tubeMC;
    order = CM_ANSWER_HOW_MANY_OK ;
    int ret = write(tubeMasterClient, &order, sizeof(int));
    // - envoyer les résultats au client
    //END TODO
}


/************************************************************************
 * quel est la minimum de l'ensemble
 ************************************************************************/
void orderMinimum(Data *data)
{
    int order = CM_ANSWER_MINIMUM_OK;

    TRACE0("[master] ordre minimum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    if (true){
       order = CM_ANSWER_MINIMUM_EMPTY;
    }   
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    int tubeMasterClient = data->tubeMC;
    int ret = write(tubeMasterClient, &order, sizeof(int));
    //       . envoyer le résultat au client
    //END TODO
}


/************************************************************************
 * quel est la maximum de l'ensemble
 ************************************************************************/
void orderMaximum(Data *data)
{
    int order = CM_ANSWER_MAXIMUM_OK;
    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // cf. explications pour le minimum

    if (true){
       order = CM_ANSWER_MAXIMUM_EMPTY;
    }   

    int tubeMasterClient = data->tubeMC;
    
    int ret = write(tubeMasterClient, &order, sizeof(int));
    //END TODO
}


/************************************************************************
 * test d'existence
 ************************************************************************/
void orderExist(Data *data)
{
  int order = CM_ANSWER_EXIST_YES;
  int ret;
    TRACE0("[master] ordre existence\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du client
    
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    if (true){
       order = CM_ANSWER_EXIST_NO;
    }   
    // - sinon
    //       . envoyer au premier worker ordre existence (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à tester
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . si élément non présent
    //             . envoyer l'accusé de réception dédié au client (cf. client_master.h)

    int tubeMasterClient = data->tubeMC;
    
    if (false){
      int ret = write(tubeMasterClient, &order, sizeof(int));
    }
    
    //       . sinon
    //             . recevoir résultat (une quantité) venant du worker concerné
    //             . envoyer l'accusé de réception au client (cf. client_master.h)
    if (true){
      ret = write(tubeMasterClient, &order, sizeof(int));
    }
    //             . envoyer le résultat au client
    //END TODO
}

/************************************************************************
 * somme
 ************************************************************************/
void orderSum(Data *data)
{
    TRACE0("[master] ordre somme\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker) : la somme est alors 0
    // - envoyer au premier worker ordre sum (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultat (la somme) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    int tubeMasterClient = data->tubeMC;
    int order = CM_ANSWER_SUM_OK;
    int ret = write(tubeMasterClient, &order, sizeof(int));
    // - envoyer le résultat au client
    //END TODO
}

/************************************************************************
 * insertion d'un élément
 ************************************************************************/

//TODO voir si une fonction annexe commune à orderInsert et orderInsertMany est justifiée

void orderInsert(Data *data)
{
    TRACE0("[master] ordre insertion\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à insérer en provenance du client
    // - si ensemble vide (pas de premier worker)
    //       . créer le premier worker avec l'élément reçu du client
    // - sinon
    //       . envoyer au premier worker ordre insertion (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à insérer
    // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    //END TODO
}


/************************************************************************
 * insertion d'un tableau d'éléments
 ************************************************************************/
void orderInsertMany(Data *data)
{
    TRACE0("[master] ordre insertion tableau\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir le tableau d'éléments à insérer en provenance du client
    // - pour chaque élément du tableau
    //       . l'insérer selon l'algo vu dans orderInsert (penser à factoriser le code)
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    int tubeMasterClient = data->tubeMC;
    int order = CM_ANSWER_INSERT_MANY_OK;
    int ret = write(tubeMasterClient, &order, sizeof(int));
    //END TODO
}


/************************************************************************
 * affichage ordonné
 ************************************************************************/
void orderPrint(Data *data)
{
    TRACE0("[master] ordre affichage\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    // - envoyer au premier worker ordre print (cf. master_worker.h)
    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    //   note : ce sont les workers qui font les affichages
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    int tubeMasterClient = data->tubeMC;
    int order = CM_ANSWER_PRINT_OK;
    int ret = write(tubeMasterClient, &order, sizeof(int));
    //END TODO
}


/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(Data *data)
{
    bool end = false;

    init(data);

    while (!end)
    {

        //DataMiddle tubeMiddle;
        //TODO ouverture des tubes avec le client (cf. explications dans client.c)
        int order;
        int tubeClientMaster = open("tubeCM", O_RDONLY);
        int tubeMasterClient = open("tubeMC", O_WRONLY);
        
     
        (*data).tubeMC = tubeMasterClient;

        int ret = read(tubeClientMaster, &order, sizeof(int));
        if (ret == -1) {
        perror("Erreur lors de la lecture du tube");  
        //Autres actions à effectuer en cas d'erreur...
        }
        //printf("ordre int: ");
        printf("%d\n", order);
          

        //assert(ret != -1);
        switch(order)
        {
          case CM_ORDER_STOP:
            orderStop(data);
            end = true;
            break;
          case CM_ORDER_HOW_MANY:
            orderHowMany(data);
            break;
          case CM_ORDER_MINIMUM:
            orderMinimum(data);
            break;
          case CM_ORDER_MAXIMUM:
            orderMaximum(data);
            break;
          case CM_ORDER_EXIST:
            orderExist(data);
            break;
          case CM_ORDER_SUM:
            orderSum(data);
            break;
          case CM_ORDER_INSERT: 
            orderInsert(data);
            break;
          case CM_ORDER_INSERT_MANY:
            orderInsertMany(data);
            break;
          case CM_ORDER_PRINT:
            orderPrint(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

        //TODO fermer les tubes nommés
        ret = close(tubeClientMaster);
        assert(ret != 1);

        ret = close(tubeMasterClient);
        assert(ret != 1);

        //     il est important d'ouvrir et fermer les tubes nommés à chaque itération
        //     voyez-vous pourquoi ?
        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)


        int sem = data->sem;

        struct sembuf operation = {0, -1, 0}; 
        ret = semop(sem, &operation, 1);
        //assert(ret != -1);

        //Reponse :
        //En fermant le tube à la fin de chaque itération, 
        //on libere la file d'attente associée au tube nommé, 
        //cela permet de garantir que chaque iteration commence avec 
        //un tube vide (sans données non lues de l'itération précédente).

        TRACE0("[master] fin ordre\n");
    }
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

//TODO N'hésitez pas à faire des fonctions annexes ; si les fonctions main
//TODO et loop pouvaient être "courtes", ce serait bien 

int main(int argc, char * argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    TRACE0("[master] début\n");

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


    Data data;
    //DataMiddle dataMiddle;
    int ret;
    key_t key;
    int semId;

    key = ftok(MON_FICHIER, PROJ_ID);

    //TODO
    // - création des sémaphores
    int key = ftok("client_master.h", MA_CLE)
    int sem = semget(MA_CLE, 0, IPC_CREAT | IPC_EXCL | 0641);
    myassert(sem != -1, "erreur sem");
    ret = semctl(sem, 0, SETVAL, 0);
    myassert(sem != -1, "erreur sem");
    data.sem = sem; // facultatif si on fait le sem op en dessous
    

    // creation tube anonyme

    int fds[2];
    ret = pipe(fds);
    assert(ret == 0);

    data.tubeMW = fds;
    

    // - création des tubes nommés
    
    
    int tubeClientMaster = mkfifo("tubeCM", 0644); 
    assert(tubeClientMaster != 1);

    int tubeMasterClient = mkfifo("tubeMC", 0644); 
    assert(tubeMasterClient != 1);

    //END TODO

    loop(&data);
    
    //TODO destruction des tubes nommés, des sémaphores, ...

    ret = unlink(tubeClientMaster);
    assert(ret != 1);
    ret = unlink(tubeMasterClient);
    assert(ret != 1);

    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}
