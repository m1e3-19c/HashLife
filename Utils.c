#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// #include "SDL_Screen.h"

#include "Utils.h"

void free_node(node *qt)
{
    // LIbération de la mémoire d'un noeud : on libère les enfants puis le pointeur du noeud
    if (qt->level > 0)
    {
        free_node(qt->nw);
        free_node(qt->ne);
        free_node(qt->sw);
        free_node(qt->se);
    }
    free(qt);
}

bool same_nodes(node *n1, node *n2)
{
    // Les deux arbres sont-ils égaux ?
    return n1->level == n2->level && n1->nw == n2->nw && n1->ne == n2->ne && n1->sw == n2->sw && n1->se == n2->se ; // Tous les attributs sont les mêmes (en particulier le niveau dans le cas des feuilles vivante et morte)
}

bool same_points(int2 p, int2 q)
{
    // Les deux points sont-ils égaux ? 
    return p.x == q.x && p.y == q.y ;
}

int max(int a, int b)
{
    return (a > b) ? a : b ;
}

int min(int a, int b)
{
    return (a > b) ? b : a ;
}

int intlog2(int n)
{
    if (n == 1)
        return 0 ;
    else
        return 1 + intlog2(n/2) ;
}

int modulo_pow_p(int i, int p)
{
    return i & ((1 << p) - 1) ;
}


