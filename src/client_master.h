#ifndef CLIENT_MASTER_H
#define CLIENT_MASTER_H

// ordres possibles du client pour le master
#define CM_ORDER_NONE         -1
#define CM_ORDER_STOP          0
#define CM_ORDER_HOW_MANY     10
#define CM_ORDER_MINIMUM      20
#define CM_ORDER_MAXIMUM      30
#define CM_ORDER_EXIST        40
#define CM_ORDER_SUM          50
#define CM_ORDER_INSERT       60
#define CM_ORDER_INSERT_MANY  70
#define CM_ORDER_PRINT        80
#define CM_ORDER_LOCAL        90      // ne concerne pas le master

// réponses possibles du master pour le client
#define CM_ANSWER_STOP_OK             0       // pour ORDER_STOP : arrêt effectué
#define CM_ANSWER_HOW_MANY_OK        10       // pour ORDER_HOW_MANY : la/les réponses suivent
#define CM_ANSWER_MINIMUM_OK         20       // pour ORDER_MINIMUM : la/les réponses suivent
#define CM_ANSWER_MINIMUM_EMPTY      21       // pour ORDER_MINIMUM : l'ensemble est vide
#define CM_ANSWER_MAXIMUM_OK         30       // pour ORDER_MAXIMUM : la/les réponses suivent
#define CM_ANSWER_MAXIMUM_EMPTY      31       // pour ORDER_MAXIMUM : l'ensemble est vide
#define CM_ANSWER_EXIST_YES          40       // pour ORDER_EXIST : l'élément est présent, la/les réponses suivent
#define CM_ANSWER_EXIST_NO           41       // pour ORDER_EXIST : l'élément n'est pas présent
#define CM_ANSWER_SUM_OK             50       // pour ORDER_SUM : la/les réponses suivent
#define CM_ANSWER_INSERT_OK          60       // pour ORDER_INSERT : insertion effectuée
#define CM_ANSWER_INSERT_MANY_OK     70       // pour ORDER_INSERT_MANY : insertions effectuées
#define CM_ANSWER_PRINT_OK           80       // pour ORDER_PRINT : affichage effectué

//TODO
// Vous pouvez mettre ici des informations soit communes au client et au
// master, soit liées aux deux :

#include <pthread.h> // rajouté

// . structures de données
typedef struct {
    int tube;
    pthread_mutex_t mutexMiddle;
} DataMiddle; //je pense qu'on peut en avoir besoin plus tard

typedef struct {

    int order;     // ordre de l'utilisateur (cf. CM_ORDER_* dans client_master.h)
    float elt;     // pour CM_ORDER_EXIST, CM_ORDER_INSERT, CM_ORDER_LOCAL
    int nb;        // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float min;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    float max;     // pour CM_ORDER_INSERT_MANY, CM_ORDER_LOCAL
    int nbThreads; // pour CM_ORDER_LOCAL

} Parametres;

// . création/libération/initialisation de ressources (sémaphores, tubes, ...)
#define MA_CLE1 23  
#define MA_CLE2 7  

#define MON_FICHIER "client_master.h"



// . communications
//END TODO

#endif
