#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <stdint.h>

#include "Utils.h"

#define MAGICAL_CONST 11400714819323198549ull // Constante magique

/*
    Hashtable.h
    Table de hachage pour quad_trees
*/

typedef struct bucket 
{
    node *key ;
    node *value ;
    struct bucket *next ;
} bucket ;

typedef struct hashtable
{
    int p ; // Capacité de la table de hashage : 2^p
    int n ;
    bucket **data ;
} hashtable ;

long hash(node *key, hashtable *t) ;
hashtable *create_hashtable(int p) ;
void free_hashtable(hashtable *t) ;
void hashtable_add(hashtable *t, node *key, node *value) ;
bool hashtable_mem(hashtable *t, node *key) ;
node *hashtable_get(hashtable *t, node *key) ;
// void hashtable_rem(hashtable *t, node *key) ; // Ne doit pas être utilisée pour cause de fuites de mémoire si un arbre est retiré de la table sans être libéré
node **all_keys(hashtable *t) ;
void analyse(hashtable *t) ;

#endif
