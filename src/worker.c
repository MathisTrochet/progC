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

#include "master_worker.h"


/************************************************************************
 * Données persistantes d'un worker
 ************************************************************************/
typedef struct
{
    // données internes (valeur de l'élément, cardinalité)
    float elt;
    int order;
    int cardi;
    // communication avec le père (2 tubes) et avec le master (1 tube en écriture)
    int fdIn;
    int fdOut;
    int fdToMaster;

    int sem3;

    // communication avec le fils gauche s'il existe (2 tubes)
    int *FG;
    // communication avec le fils droit s'il existe (2 tubes)
    int *FD;
    //TODO
} Data;


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/
static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s <elt> <fdIn> <fdOut> <fdToMaster>\n", exeName);
    fprintf(stderr, "   <elt> : élément géré par le worker\n");
    fprintf(stderr, "   <fdIn> : canal d'entrée (en provenance du père)\n");
    fprintf(stderr, "   <fdOut> : canal de sortie (vers le père)\n");
    fprintf(stderr, "   <fdToMaster> : canal de sortie directement vers le master\n");
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}

void fermetureTubes(Data *data){
   int ret = close(data->fdToMaster);
  myassert(ret != -1, "close tube failure"); 
}

static void parseArgs(int argc, char * argv[], Data *data)
{
    myassert(data != NULL, "il faut l'environnement d'exécution");

    if (argc != 5)
        usage(argv[0], "Nombre d'arguments incorrect");

    //TODO initialisation data
    
    //TODO (à enlever) comment récupérer les arguments de la ligne de commande
    float elt = strtof(argv[1], NULL);
    int fdIn = strtol(argv[2], NULL, 10);
    int fdOut = strtol(argv[3], NULL, 10);
    int fdToMaster = strtol(argv[4], NULL, 10);
    //printf("%g %d %d %d\n", elt, fdIn, fdOut, fdToMaster);
    (*data).elt = elt;
    (*data).fdIn = fdIn;
    (*data).fdOut = fdOut;
    (*data).fdToMaster = fdToMaster;
    //END TODO
}


/************************************************************************
 * Stop 
 ************************************************************************/
void stopAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre stop\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO

    //printf("it worked"); // seulement pour tester

    // - traiter les cas où les fils n'existent pas
    // - envoyer au worker gauche ordre de fin (cf. master_worker.h)
    // - envoyer au worker droit ordre de fin (cf. master_worker.h)
    // - attendre la fin des deux fils
    //END TODO
}


/************************************************************************
 * Combien d'éléments
 ************************************************************************/
static void howManyAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre how many\n", getpid(), getppid(),  data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    
    //       . envoyer ordre howmany (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir deux résultats (nb elts, nb elts distincts) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer les résultats (les cumuls des deux quantités + la valeur locale) au père
    //END TODO
}


/************************************************************************
 * Minimum
 ************************************************************************/
static void minimumAction(Data *data)
{
  int ret ;
    TRACE3("    [worker (%d, %d) {%g}] : ordre minimum\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche n'existe pas (on est sur le minimum)
    //       . envoyer l'accusé de réception au master (cf. master_worker.h)
    //       . envoyer l'élément du worker courant au master
    if (true){
      int answer = MW_ANSWER_MINIMUM;
      ret = write(data->fdToMaster, &answer, sizeof(int)); 
      myassert(ret != -1, "tube masterClient ecriture erreur");
      ret = write(data->fdToMaster, &(data->elt), sizeof(float));
      myassert(ret != -1, "tube masterClient ecriture erreur");
    }
    else {
      int answer = MW_ORDER_MINIMUM;
      ret = write(data->fdIn, &answer, sizeof(int)); 
      myassert(ret != -1, "tube masterClient ecriture erreur");
    }
    
    // - sinon
    //       . envoyer au worker gauche ordre minimum (cf. master_worker.h)
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Maximum
 ************************************************************************/
static void maximumAction(Data *data)
{
  int ret ;
    TRACE3("    [worker (%d, %d) {%g}] : ordre maximum\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils droit n'existe pas (on est sur le maximum)
    //       . envoyer l'accusé de réception au master (cf. master_worker.h)
    //       . envoyer l'élément du worker courant au master
    if (true){
      int answer = MW_ANSWER_MAXIMUM;
      ret = write(data->fdToMaster, &answer, sizeof(int)); 
      myassert(ret != -1, "tube masterClient ecriture erreur");
      ret = write(data->fdToMaster, &(data->elt), sizeof(float));
      myassert(ret != -1, "tube masterClient ecriture erreur");
    }
    else {  
      int answer = MW_ORDER_MAXIMUM;
      ret = write(data->fdIn, &answer, sizeof(int)); 
      myassert(ret != -1, "tube masterClient ecriture erreur");
    }
    
    // - sinon
    //       . envoyer au worker droit ordre minimum (cf. master_worker.h)
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Existence
 ************************************************************************/
static void existAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre exist\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - recevoir l'élément à tester en provenance du père
    int answer;
    int order = data->order;
    int param;
    read(data->fdOut, &param, sizeof(float));

    if (param == data->elt){
      answer = MW_ANSWER_EXIST_YES;
      write(data->fdToMaster, &answer, sizeof(int));
      write(data->fdToMaster, &data->cardi, sizeof(int));
    }
    else {
      if (param < data->elt && data->FG == NULL){
        //envoyer au master echec
      }
      else {
        if (param > data->elt && data->FG == NULL){ 
          //envoyer au master echec
        }
        else {
          if (param < data->elt ){
            //envoyer au master et autre 
          }
          else {
            if (param > data->elt ){
              //envoyer au master et autre
            }
        }
        }
      }
    }
    // - si élément courant == élément à tester
    //       . envoyer au master l'accusé de réception de réussite (cf. master_worker.h)
    //       . envoyer cardinalité de l'élément courant au master
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . envoyer au master l'accusé de réception d'échec (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant)
    //       . envoyer au worker gauche ordre exist (cf. master_worker.h)
    //       . envoyer au worker gauche élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    // - sinon (donc elt à tester > elt courant)
    //       . envoyer au worker droit ordre exist (cf. master_worker.h)
    //       . envoyer au worker droit élément à tester
    //       . note : c'est un des descendants qui enverra le résultat au master
    //END TODO
}


/************************************************************************
 * Somme
 ************************************************************************/
static void sumAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre sum\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - traiter les cas où les fils n'existent pas
    // - pour chaque fils
    //       . envoyer ordre sum (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    //       . recevoir résultat (somme du fils) venant du fils
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    // - envoyer le résultat (le cumul des deux quantités + la valeur locale) au père
    //END TODO
}


/************************************************************************
 * Insertion d'un nouvel élément
 ************************************************************************/
static void insertAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre insert\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    printf("\nje suis là\n");

    int order = data->order;
    int param;
    read(data->fdOut, &param, sizeof(float));
    write(data->fdToMaster, &order, sizeof(int)); // envoyer accusé de reception 
    //TODO
    // - recevoir l'élément à insérer en provenance du père
    // - si élément courant == élément à tester
    //       . incrémenter la cardinalité courante
    //       . envoyer au master l'accusé de réception (cf. master_worker.h)
    // - sinon si (elt à tester < elt courant) et (pas de fils gauche)
    //       . créer un worker à gauche avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à tester > elt courant) et (pas de fils droit)
    //       . créer un worker à droite avec l'élément reçu du client
    //       . note : c'est ce worker qui enverra l'accusé de réception au master
    // - sinon si (elt à insérer < elt courant)
    //       . envoyer au worker gauche ordre insert (cf. master_worker.h)
    //       . envoyer au worker gauche élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    // - sinon (donc elt à insérer > elt courant)
    //       . envoyer au worker droit ordre insert (cf. master_worker.h)
    //       . envoyer au worker droit élément à insérer
    //       . note : c'est un des descendants qui enverra l'accusé de réception au master
    //END TODO
}


/************************************************************************
 * Affichage
 ************************************************************************/
static void printAction(Data *data)
{
    TRACE3("    [worker (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), data->elt);
    myassert(data != NULL, "il faut l'environnement d'exécution");

    //TODO
    // - si le fils gauche existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - afficher l'élément courant avec sa cardinalité
    // - si le fils droit existe
    //       . envoyer ordre print (cf. master_worker.h)
    //       . recevoir accusé de réception (cf. master_worker.h)
    // - envoyer l'accusé de réception au père (cf. master_worker.h)
    //END TODO
}


/************************************************************************
 * Boucle principale de traitement
 ************************************************************************/
void loop(Data *data)
{
    int ret;
    bool end = false;


    while (!end)
    {
        int ret = read(data->fdOut, &data->order, sizeof(int));
        myassert(ret!=-1, "read marche pas");
        printf("\n[WORKER] order-> ||%d|| - ", data->order);


        //TRACE3("    [LOOP (%d, %d) {%g}] : ordre print\n", getpid(), getppid(), data->elt);

        //int order = MW_ORDER_STOP;   //TODO pour que ça ne boucle pas, mais recevoir l'ordre du père 
        switch(data->order)
        {
          case MW_ORDER_STOP:
            stopAction(data);
            end = true;
            break;
          case MW_ORDER_HOW_MANY:
            howManyAction(data);
            break;
          case MW_ORDER_MINIMUM:
            minimumAction(data);
            break;
          case MW_ORDER_MAXIMUM:
            maximumAction(data);
            break;
          case MW_ORDER_EXIST:
            existAction(data);
            break;
          case MW_ORDER_SUM:
            sumAction(data);
            break;
          case MW_ORDER_INSERT:
            insertAction(data);
            break;
          case MW_ORDER_PRINT:
            printAction(data);
            break;
          default:
            myassert(false, "ordre inconnu");
            exit(EXIT_FAILURE);
            break;
        }

/*
          struct sembuf operation3 = {0, -1, 0}; 
          ret = semop(sem3, &operation3, 1);
          myassert(ret != -1, "erreur operation3"); 
*/
        

        

        TRACE3("    [worker (%d, %d) {%g}] : fin ordre\n", getpid(), getppid(), data->elt);
    }
}





/************************************************************************
 * Programme principal
 ************************************************************************/

int main(int argc, char * argv[])
{
    Data data;
    int ret;
    parseArgs(argc, argv, &data);
    TRACE3("    [worker (%d, %d) {%g}] : début worker\n", getpid(), getppid(), data.elt);

    //TRACE3("||(%d, %d) {%d}||\n", answer, answer, answer);
/*
    key3 = ftok(MON_FICHIER2, MA_CLE3);
    sem3 = semget(key3, 1, 0); // on recupere le semaphore pour bloquer la loop
    data.sem3 = sem3;
    */



    //TODO envoyer au master l'accusé de réception d'insertion (cf. master_worker.h)
    int answer = MW_ANSWER_INSERT;
    ret = write(data.fdToMaster, &answer, sizeof(int));
    myassert(ret != -1, "write pas bon");

    //TODO note : en effet si je suis créé c'est qu'on vient d'insérer un élément : moi


    loop(&data);

    //TODO fermer les tubes
    fermetureTubes(&data);

    TRACE3("    [worker (%d, %d) {%g}] : fin worker\n", getpid(), getppid(), data.elt);
    return EXIT_SUCCESS;
}
