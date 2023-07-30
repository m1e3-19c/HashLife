#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdbool.h>

// #include "SDL_Screen.h"

typedef struct int2
{
    int x, y ;
} int2 ;

typedef struct node
{
    int level ; // Niveau de la macro-cellule (si cellule unique, alors -1 pour mort et 0 pour vivant)
    struct node *nw, *ne, *sw, *se ;
} node ;

// Utilitaire structures :
void free_node(node *qt) ;
bool same_nodes(node *n1, node *n2) ;
bool egal(int2 p, int2 q) ;

// Utilitaire Maths :
int max(int a, int b) ;
int min(int a, int b) ;
int intlog2(int n) ;
int modulo_pow_p(int k, int p) ;

// node *copy_node(node *qt) ;
node *rotate_node(node *qt, int rotation) ;

// void set_leaf(node *qt, int x, int y, int value) ;

int **open_rle(char *filename, int *p) ;

#endif