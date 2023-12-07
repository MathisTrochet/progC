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

#include "client_master.h"
#include "master_worker.h"

#include <fcntl.h>//  rajouté
#include <pthread.h> // rajouté
#include <sys/ipc.h> // ajouté 
#include <sys/sem.h> // ajouté 
#include <sys/stat.h> // ajouté 


/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/
typedef struct
{
    // communication avec le client
    int tubeCM;
    int tubeMC;
    int sem1;
    int sem2; // le semaphore pour bloquer la loop
    
    // données internes
    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL

    bool isGrandWorkerEmpty;
    int pid;
    int sem3;
    
    // communication avec le premier worker (double tubes)
    int *tubeWW;
    // communication en provenance de tous les workers (un seul tube en lecture)
    int *tubeMW;

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
    /*
        (*data).tubeCM = open
    myassert(data->tubeCM == 0, "Création du tube");

    (*data).tubeMC = mkfifo("tubeMC", 0644);
    myassert(data->tubeMC == 0, "Création du tube");
  */
 

    (*data).isGrandWorkerEmpty = true; // au debut le worker est vide

    //TODO initialisation data
}



/************************************************************************
 * initialisation des éléments du master *
 ************************************************************************/

void creation_Semaphore(Data *data){

  int ret, sem1, sem2, sem3;
    key_t key1, key2, key3;

    key1 = ftok(MON_FICHIER, MA_CLE1); // sem1 -> semaphore pour empecher 2 clients de faire un demande au master en meme temps 
    myassert(key1 != -1, "erreur key");

    sem1 = semget(key1, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(sem1 != -1, "erreur semget");

    ret = semctl(sem1, 0, SETVAL, 1);
    myassert(ret != -1, "erreur semctl");

    (*data).sem1 = sem1;



    key2 = ftok(MON_FICHIER, MA_CLE2);   // sem2 -> semaphore pour bloquer la loop du master le temps que le client read le resultat et le print 
    myassert(key2 != -1, "erreur key2");

    sem2 = semget(key2, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(sem2 != -1, "erreur semget2");

    ret = semctl(sem2, 0, SETVAL, 0);
    myassert(ret != -1, "erreur semctl2");

    (*data).sem2 = sem2;

/*
    key3 = ftok(MON_FICHIER2, MA_CLE3);   // sem3 -> semaphore pour bloquer la loop du worker le temps que le master read le resultat et le renvoie au client
    myassert(key3 != -1, "erreur key2");

    sem3 = semget(key3, 1, IPC_CREAT | IPC_EXCL | 0641);
    myassert(sem3 != -1, "erreur semget3");

    ret = semctl(sem3, 0, SETVAL, 0);
    myassert(ret != -1, "erreur semctl3");

    (*data).sem3 = sem3; // facultatif si on fait le sem op en dessous
  */

}

void creation_TubeAnonyme(Data *data){
    int ret;
    int fdsWW[2]; 
    ret = pipe(fdsWW);
    myassert(ret == 0, "tubeAnonyme WW Erreur");

    int fdsMW[2];
    ret = pipe(fdsMW);
    myassert(ret == 0, "tubeAnonyme MW Erreur");

    (*data).tubeWW = fdsWW;
    (*data).tubeMW = fdsMW;
}

void creation_TubeNomme(Data *data){
  int tubeClientMaster = mkfifo("tubeCM", 0644); 
    myassert(tubeClientMaster != 1, "tube CM erreur creation");

  int tubeMasterClient = mkfifo("tubeMC", 0644); 
    myassert(tubeMasterClient != 1, "tube MC erreur creation");

    (*data).tubeCM = tubeClientMaster;
    (*data).tubeMC = tubeMasterClient;
}

void destruction_TubeNomme(){
  int ret;
    ret = unlink("tubeCM");
    myassert(ret != 1, "destruction tubeCM nomme erreur");
    ret = unlink("tubeMC");
    myassert(ret != 1, "destruction tubeMC nomme erreur");
}

void destruction_Semaphore(Data *data){
  int ret;
  int sem1 = data->sem1, sem2 = data->sem2, sem3 = data->sem3;
    ret = semctl(sem1, -1, IPC_RMID);
    myassert(ret != -1, "erreur semctl pour suppression du sémaphore");
    ret = semctl(sem2, -1, IPC_RMID);
    myassert(ret != -1, "erreur semctl pour suppression du sémaphore");
    /*
    ret = semctl(sem3, -1, IPC_RMID);
    myassert(ret != -1, "erreur semctl pour suppression du sémaphore");
    */
  }



/************************************************************************
 * fin du master    
 ************************************************************************/
void orderStop(Data *data)
{
    TRACE0("[master] ordre stop\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    int ret;

    int order = CM_ANSWER_STOP_OK;

    //int ret = write(fds[1], &order, sizeof(int));

    
    if (!data->isGrandWorkerEmpty){
      write(data->tubeWW[1], &order, sizeof(int));
    }

    
    
    // - traiter le cas ensemble vide (pas de premier worker) check
    // - envoyer au premier worker ordre de fin (cf. master_worker.h) check
    // - attendre sa fin -> (semaphore a mettre)
    // - envoyer l'accusé de réception au client (cf. client_master.h) check
    int tubeMasterClient = data->tubeMC;
    
    ret = write(tubeMasterClient, &order, sizeof(int));
    myassert(ret != -1, "tube ecriture erreur");
    
    //END TODO
}


/************************************************************************
 * quel est la cardinalité de l'ensemble
 ************************************************************************/
void orderHowMany(Data *data)
{
  int order = CM_ANSWER_HOW_MANY_OK;
  //int results;
    int result, ret;
    TRACE0("[master] ordre how many\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter le cas ensemble vide (pas de premier worker)
    if (data->isGrandWorkerEmpty){
      result = 0;
    }
    // - envoyer au premier worker ordre howmany (cf. master_worker.h)

    // - recevoir accusé de réception venant du premier worker (cf. master_worker.h)
    // - recevoir résultats (deux quantités) venant du premier worker
    // - envoyer l'accusé de réception au client (cf. client_master.h)
    
    order = CM_ANSWER_HOW_MANY_OK ;
    ret = write(data->tubeMC, &order, sizeof(int));
    myassert(ret != -1, "tube masterClient ecriture erreur");
    ret = write(data->tubeMC, &result, sizeof(int));
    myassert(ret != -1, "tube masterClient ecriture erreur");
    // - envoyer les résultats au client
    //END TODO
}


/************************************************************************
 * quel est la minimum de l'ensemble
 ************************************************************************/
void orderMinimum(Data *data)
{
    int order = CM_ANSWER_MINIMUM_OK, result, ret, answer;

    TRACE0("[master] ordre minimum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    if (data->isGrandWorkerEmpty){
       order = CM_ANSWER_MINIMUM_EMPTY;
       ret = write(data->tubeMC, &order, sizeof(int));
       myassert(ret != -1, "tube masterClient ecriture erreur");
    } 
    else {
      ret = write(data->tubeWW[1], &order, sizeof(int));
      myassert(ret != -1, "tube masterClient ecriture erreur");

      ret = read(data->tubeMW[0], &answer, sizeof(int));        // recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
      myassert(ret != -1, "tube masterClient lecture erreur");
      ret = read(data->tubeMW[0], &result, sizeof(float));        // recevoir résultat (la valeur) venant du worker concerné
      myassert(ret != -1, "tube masterClient lecture erreur");

      ret = write(data->tubeMC, &order, sizeof(int));           // envoyer l'accusé de réception au client (cf. client_master.h)
      myassert(ret != -1, "tube masterClient ecriture erreur");
      ret = write(data->tubeMC, &result, sizeof(float));
      myassert(ret != -1, "tube masterClient ecriture erreur");
    }
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    //       . envoyer le résultat au client
    //END TODO
}


/************************************************************************
 * quel est la maximum de l'ensemble
 ************************************************************************/
void orderMaximum(Data *data)
{
    int order = CM_ANSWER_MAXIMUM_OK, ret, result, answer;
    TRACE0("[master] ordre maximum\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    if (data->isGrandWorkerEmpty){
       order = CM_ANSWER_MAXIMUM_EMPTY;
       ret = write(data->tubeMC, &order, sizeof(int));
       myassert(ret != -1, "tube masterClient ecriture erreur");
    } 
    else {
      ret = write(data->tubeWW[1], &order, sizeof(int));
      myassert(ret != -1, "tube masterClient ecriture erreur");

      ret = read(data->tubeMW[0], &answer, sizeof(int));        // recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
      myassert(ret != -1, "tube masterClient lecture erreur");
      ret = read(data->tubeMW[0], &result, sizeof(float));        // recevoir résultat (la valeur) venant du worker concerné
      myassert(ret != -1, "tube masterClient lecture erreur");

      ret = write(data->tubeMC, &order, sizeof(int));           // envoyer l'accusé de réception au client (cf. client_master.h)
      myassert(ret != -1, "tube masterClient ecriture erreur");
      ret = write(data->tubeMC, &result, sizeof(float));
      myassert(ret != -1, "tube masterClient ecriture erreur");
    }
    // - sinon
    //       . envoyer au premier worker ordre minimum (cf. master_worker.h)
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . recevoir résultat (la valeur) venant du worker concerné
    //       . envoyer l'accusé de réception au client (cf. client_master.h)
    //       . envoyer le résultat au client
    //END TODO
}


/************************************************************************
 * test d'existence
 ************************************************************************/
void orderExist(Data *data)
{
  int order = CM_ANSWER_EXIST_YES;
  int ret, result, answer;
    TRACE0("[master] ordre existence\n");
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du client
    
    // - si ensemble vide (pas de premier worker)
    //       . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    if (data->isGrandWorkerEmpty){
       order = CM_ANSWER_EXIST_NO;
       ret = write(data->tubeMC, &order, sizeof(int));
       myassert(ret != -1, "tube masterClient ecriture erreur");
    } 
    else {
      ret = write(data->tubeWW[1], &order, sizeof(int));
      myassert(ret != -1, "tube masterClient ecriture erreur");
      ret = write(data->tubeWW[1], &data->elt, sizeof(int));
      myassert(ret != -1, "tube masterClient ecriture erreur");

      ret = read(data->tubeMW[0], &answer, sizeof(int));        // recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
      myassert(ret != -1, "tube masterClient lecture erreur");
      
      if (answer == MW_ANSWER_EXIST_NO){
        ret = write(data->tubeMC, &order, sizeof(int));           // envoyer l'accusé de réception au client (cf. client_master.h)
        myassert(ret != -1, "tube masterClient ecriture erreur");
      }
      else{
        ret = write(data->tubeMC, &order, sizeof(int));           // envoyer l'accusé de réception au client (cf. client_master.h)
        myassert(ret != -1, "tube masterClient ecriture erreur");
        ret = write(data->tubeMC, &result, sizeof(float));
        myassert(ret != -1, "tube masterClient ecriture erreur");
      }

      ret = read(data->tubeMW[0], &result, sizeof(float));        // recevoir résultat (la valeur) venant du worker concerné
      myassert(ret != -1, "tube masterClient lecture erreur");

      
    }
    // - sinon
    //       . envoyer au premier worker ordre existence (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à tester
    //       . recevoir accusé de réception venant du worker concerné (cf. master_worker.h)
    //       . si élément non présent
    //             . envoyer l'accusé de réception dédié au client (cf. client_master.h)
    
    //       . sinon
    //             . recevoir résultat (une quantité) venant du worker concerné
    //             . envoyer l'accusé de réception au client (cf. client_master.h)
    //             . envoyer le résultat au client
    ret = write(data->tubeMC, &order, sizeof(int));
    myassert(ret != -1, "tube masterClient ecriture erreur");
    ret = write(data->tubeMC, &result, sizeof(int));
    myassert(ret != -1, "tube masterClient ecriture erreur");
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
    myassert(ret != -1, "tube ecriture erreur");
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

    // - recevoir l'élément à insérer en provenance du client 
    int param = data->elt, order=data->order, ret, answer=0;
    char strParam[20], strT1[20], strT2[20], strT3[20];
    snprintf(strParam, sizeof(strParam), "%d", param);

    //printf("[MASTER] -> ||%d||",order);

    if (data->isGrandWorkerEmpty == true ){ //ensemble vide
      pid_t pid = fork();                                            /* --------creation processus pour creer le worker --------*/
      myassert(pid != 1, "erreur pid");
      if (pid == 0){

        if (data->tubeWW[0] == -1 || data->tubeWW[1] == -1 || data->tubeMW == -1) {
          fprintf(stderr, "Erreur : Les tubes ne sont pas correctement initialisés.\n");
          exit(EXIT_FAILURE);
        }

        ret = snprintf(strT1, sizeof(strT1), "%d", data->tubeWW[1]);
        myassert(ret != -1, "snprintf erreur");
        ret = snprintf(strT2, sizeof(strT2), "%d",data->tubeWW[0]);
        myassert(ret != -1, "snprintf erreur");
        ret = snprintf(strT3, sizeof(strT3), "%d", data->tubeMW[1]);
        myassert(ret != -1, "snprintf erreur");
        //printf("[MASTER] -> ||%d||",order);
        

        ret = execl("./worker", "worker", strParam,  strT1, strT2, strT3, (char *)NULL);
        myassert(ret != -1, "erreur execl");
            
        exit(EXIT_FAILURE);
        
      }
      //mettre ce qu'il y a au dessus 
    }

    else {
      printf("le bigworker n'est plus vide ");
      write (data->tubeWW[1], &order, sizeof(int));
      myassert(ret != -1, "tube ecriture erreur");
      write (data->tubeWW[1], &param, sizeof(int));
    }
    // Sinon juste envoyer les données avec le tube MASTER-WORKER 
    // (car le processus fils devrait tourner en boucle)

    // - si ensemble vide (pas de premier worker)
    //       . créer le premier worker avec l'élément reçu du client
    // - sinon
    //       . envoyer au premier worker ordre insertion (cf. master_worker.h)
    //       . envoyer au premier worker l'élément à insérer
    // - recevoir accusé de réception venant du worker concerné (cf. master_worker.h)

    ret = read(data->tubeMW[0], &answer, sizeof(int));
    myassert(ret != -1, "read erreur");

    if (answer == MW_ANSWER_INSERT){
      (*data).isGrandWorkerEmpty = false; // si le worker renvoi une valeur ca veut dire
    }                                // que le premier worker a bien été créé (donc var = nonVide)

    printf("[MASTER] Accusé de reception : %d\n", answer);
     
    // - envoyer l'accusé de réception au client (cf. client_master.h)

    int tubeMasterClient = data->tubeMC;
    order = CM_ANSWER_INSERT_OK;
    ret = write(tubeMasterClient, &order, sizeof(int));
    myassert(ret != -1, "write erreur");
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
    myassert(ret != -1, "write erreur");
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
    myassert(ret != -1, "write erreur");
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

        int order, ret;
        
        int tubeClientMaster = open("tubeCM", O_RDONLY);
        myassert(tubeClientMaster != -1, "tube pas ouvert");
        int tubeMasterClient = open("tubeMC", O_WRONLY);
        myassert(tubeMasterClient != -1, "tube pas ouvert");

        (*data).tubeMC = tubeMasterClient; // pour que les fonctions au dessus puissent envoyer les données dans le tube qu'on vient d'ouvrir

        //              RECUPERATION DONNEE CLIENT
        ret = read(tubeClientMaster, &order, sizeof(int)); 
        myassert (ret != -1, "Erreur lors de la lecture du tube");
        printf("|%d|\n", order);

        //on stock l'ordre dans data

        (*data).order = order;

        // on recoit et on stock les parametres dans data 

        if (order == CM_ORDER_INSERT || order == CM_ORDER_EXIST || order == CM_ORDER_INSERT_MANY || order == CM_ORDER_LOCAL){
          Parametres par;
          ret = read(tubeClientMaster, &par, sizeof(par));                  // on decide ici de recuperer la structure de donnée et la placer dans la structure Data
          myassert (ret != -1, "Erreur lors de la lecture du tube");
          printf("(%d)\n", (int) par.elt);

          (*data).elt = par.elt;
          (*data).max = par.max;
          (*data).min = par.min;
          (*data).nb = par.nb;
          (*data).nbThreads = par.nbThreads;
        }                                           //je sais pas si cette façon de faire peut provoquer des fuites mémoire ou pas (à cause des valeurs non existantes copiés)
          
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

        //TODO attendre ordre du client avant de continuer (sémaphore pour une précédence)

        ret = close(tubeClientMaster);
        myassert(ret != -1, "tube CM erreur ");

        ret = close(tubeMasterClient);
        myassert(ret != -1, "tube MC erreur" ); 

        int sem2 = data->sem2;

        struct sembuf operation2 = {0, -1, 0}; 
        ret = semop(sem2, &operation2, 1);
        myassert(ret != -1, "erreur sem2");   

        //TODO fermer les tubes nommés
      
        

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

    Data data;
    
    creation_Semaphore(&data);
 
    //creation_TubeAnonyme(&data);
    int ret, fdsWW[2], fdsMW[2];
    ret = pipe(fdsWW);
    myassert(ret == 0, "tubeAnonyme WW Erreur");
    ret = pipe(fdsMW);
    myassert(ret == 0, "tubeAnonyme MW Erreur");
    data.tubeWW = fdsWW;
    data.tubeMW = fdsMW;
    

    creation_TubeNomme(&data);    

    loop(&data);

    destruction_TubeNomme();

    destruction_Semaphore(&data);

    TRACE0("[master] terminaison\n");
    return EXIT_SUCCESS;
}
