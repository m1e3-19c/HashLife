#ifndef __LIFE_H__
#define __LIFE_H__

#include <stdbool.h>

#include "Utils.h"
#include "Hashtable.h"

#define NB_ROTATIONS 4

#define AUTOMATON_CONWAY_LIFE 0
#define AUTOMATON_WIREWORLD 1
#define AUTOMATON_DAY_AND_NIGHT 2
#define AUTOMATON_LIFE_3_4 3

#define LEAF_DEAD -1
#define LEAF_ALIVE 0
#define LEAF_ELECTRON_HEAD -2
#define LEAF_ELECTRON_TAIL -3

#define LEAF_SWITCH -256

typedef struct hashlife
{
    int gen ;
    node *root ;
    hashtable *can_tab ; // table de hachage des arbres canoniques pour ne pas avoir de noeuds doublons
    hashtable *res_tab ; // table de hachage des résultats de la génération "suivante" d'un noeud donné
    hashtable *superspeed_res_tab ;
    node **blank_nodes ; // tableau contenant les pointeurs vers les arbres remplis de vide : blank_node[i] donne l'arbre vide de niveau i
    int nb_structures ;
    node ***structures ; // Structures utiles du jeu de la vie : voir annexe

    int automaton ;
    bool superspeed ;
} hashlife ;

// void free_node(node *qt) ;
void free_hashlife() ;
node *canonicalise(node *qt, hashlife *hl) ;

hashlife *create_hashlife(int level, int automaton) ;
void next_gen(hashlife *hl) ;

void random_config(hashlife *hl) ;
void clear_world(hashlife *hl) ;

void save_world(hashlife *hl) ;

void place_structure(hashlife *hl, int x, int y, int struct_index, int rotation, bool precise_coords) ;
void place_node(hashlife *hl, int x, int y, int struct_index, int rotation) ;
#endif