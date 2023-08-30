#include <stdio.h>
#include <stdlib.h>

#include <SDL2/SDL.h>

#include "Miscellaneous.h"
#include "Hashlife.h"
#include "Utils.h"

// MANIPULATION DES NOEUDS :
// node *copy_node(node *qt)
// {
//     node *res = (node *)malloc(sizeof(node)) ;
//     res->level = qt->level ;
//     if (qt->level > 0)
//     {
//         res->nw = copy_node(qt->nw) ;
//         res->ne = copy_node(qt->ne) ;
//         res->sw = copy_node(qt->sw) ;
//         res->se = copy_node(qt->se) ;
//     }

//     return res ;
// }

node *rotate_once(node *qt, hashlife *hl)
{
    node *res = (node *)malloc(sizeof(node)) ;
    res->level = qt->level ;
    if (qt->level > 0)
    {
        node *tmp = qt->nw ;
        res->nw = rotate_once(qt->ne, hl) ;
        res->ne = rotate_once(qt->se, hl) ;
        res->se = rotate_once(qt->sw, hl) ;
        res->sw = rotate_once(tmp, hl) ;
    }
    else
    {
        res->nw = NULL ;
        res->ne = NULL ;
        res->sw = NULL ;
        res->se = NULL ;
    }
    return canonicalise(res, hl) ;
}


node *random_node(int level, int p, hashlife *hl)
{
    // Configuration aléatoire : macro-cellule de niveau level et dont les feuilles ont une chance sur p d'être vivantes
    node *qt = (node *)malloc(sizeof(node));
    qt->level = level;
    if (level == 0)
    {
        if (rand() % p != 0) // La case n'a pas la chance d'être noire
            qt->level = -1;
        qt->nw = NULL;
        qt->ne = NULL;
        qt->sw = NULL;
        qt->se = NULL;
    }
    else if (level > 0) // création des sous-cellules aléatoires
    {
        qt->nw = random_node(level - 1, p, hl);
        qt->ne = random_node(level - 1, p, hl);
        qt->sw = random_node(level - 1, p, hl);
        qt->se = random_node(level - 1, p, hl);
    }

    return canonicalise(qt, hl); // On renvoie un arbre canonique
}

node *blank_node(int level)
{
    // Renvoie un nouveau noeud vierge du niveau souhaité (en créant des nouveau pointeurs)
    // L'arbre ne sera pas canonique car il peut servir pour créer les arbres vierges lors de la phase de calcul des règles de base
    node *res = (node *)malloc(sizeof(node));
    if (level == 0) // feuille
    {
        res->level = -1; // blanche car noeud vierge
        res->nw = NULL;
        res->ne = NULL;
        res->sw = NULL;
        res->se = NULL;
    }
    else // noeud interne
    {
        res->level = level;
        res->nw = blank_node(level - 1);
        res->ne = blank_node(level - 1);
        res->sw = blank_node(level - 1);
        res->se = blank_node(level - 1);
    }
    return res;
}

node *blank_canonical(int level, hashlife *hl)
{
    // Renvoie le noeud vierge canonique de niveau level
    if (hl->blank_nodes[level] != NULL) // Il est déjà dans le tableau des noeuds vierges canoniques
        return hl->blank_nodes[level];

    // else (il faut créer le noeud canonique et l'ajouter au tableau des noeuds vierges) :
    node *qt = (node *)malloc(sizeof(node));
    if (level == 0)
    {
        qt->level = -1;
        qt->nw = NULL;
        qt->ne = NULL;
        qt->sw = NULL;
        qt->se = NULL;
    }
    else
    {
        qt->level = level;
        qt->nw = blank_canonical(level - 1, hl); // On prend le noeud vierge canonique de niveau inférieur (s'il y a besoin de le créer, seul un sel calcul sera fait car le résultat sera disponible en temps constant aux appels récursifs suivants)
        qt->ne = blank_canonical(level - 1, hl);
        qt->sw = blank_canonical(level - 1, hl);
        qt->se = blank_canonical(level - 1, hl);
    }
    qt = canonicalise(qt, hl);   // On canonise le nouveau noeud
    hl->blank_nodes[level] = qt; // On l'ajoute au tableau des noeuds vierges canoniques pour les appels futurs
    return qt;
}

node *blank_border(node *qt, hashlife *hl)
{
    // Crée une bodure vide autour du noeud qt (on place qt au centre d'un noeud vierge plus grand)
    node *blank = blank_canonical(qt->level - 1, hl); // Monde vide au de taille inférieure (c'est le noeud avec lequel on va remplir autour)

    // On ajoute le noeud qt au centre en remplissant les contours avec le noeud vierge

    // Schéma du résultat : (on note b les noeuds blank de nievau qt->level - 1)

    //          b    b    b    b
    //
    //          b    nw   ne   b
    //
    //          b    sw   se   b
    //
    //          b    b    b    b

    node *res = (node *)malloc(sizeof(node));
    res->level = qt->level + 1;
    res->nw = (node *)malloc(sizeof(node));
    res->nw->level = qt->level;
    res->nw->nw = blank;
    res->nw->ne = blank;
    res->nw->sw = blank;
    res->nw->se = qt->nw;

    res->ne = (node *)malloc(sizeof(node));
    res->ne->level = qt->level;
    res->ne->nw = blank;
    res->ne->ne = blank;
    res->ne->sw = qt->ne;
    res->ne->se = blank;

    res->sw = (node *)malloc(sizeof(node));
    res->sw->level = qt->level;
    res->sw->nw = blank;
    res->sw->ne = qt->sw;
    res->sw->sw = blank;
    res->sw->se = blank;

    res->se = (node *)malloc(sizeof(node));
    res->se->level = qt->level;
    res->se->nw = qt->se;
    res->se->ne = blank;
    res->se->sw = blank;
    res->se->se = blank;

    return canonicalise(res, hl); // On ne crée pas de doublon évidemment
}

node *assemble4(node *nw, node *ne, node *sw, node *se, hashlife *hl)
{
    // Assembe les quatre noeuds pour en former un plus grand
    node *qt = (node *)malloc(sizeof(node));
    qt->level = max(1, nw->level + 1); // Cas quand bien même imopssible où nw est une feuille blanche
    qt->nw = nw;
    qt->ne = ne;
    qt->sw = sw;
    qt->se = se;

    return canonicalise(qt, hl);
}

node *get_centre(node *qt, hashlife *hl)
{
    // Donne les quatre noeuds centraux du noeud en argument
    node *res = (node *)malloc(sizeof(node));
    res->level = qt->level - 1;
    res->nw = qt->nw->se;
    res->ne = qt->ne->sw;
    res->sw = qt->sw->ne;
    res->se = qt->se->nw;

    return canonicalise(res, hl);
}

node *aux_change_square(node *qt, hashlife *hl, int x, int y, int value)
{
    // Fonction auxiliaire pour inverser la couleur de la case de coordonnées (x, y) dans l'arbre (en partant du coin en haut à gauche)
    if (qt->level <= 0)
    {
        if (value == LEAF_DEAD)
            return hl->blank_nodes[0] ;

        node *res = (node *)malloc(sizeof(node)) ;
        res->nw = NULL ;
        res->ne = NULL ;
        res->sw = NULL ;
        res->ne = NULL ;
        if (value != LEAF_SWITCH)
            res->level = value ;
        else
        {
            if (hl->automaton == AUTOMATON_WIREWORLD)
            {
                switch (qt->level)
                {
                    case LEAF_ALIVE :
                        res->level = LEAF_ELECTRON_HEAD ;
                        break ;
                    case LEAF_ELECTRON_HEAD :
                        res->level = LEAF_ELECTRON_TAIL ;
                        break ;
                    case LEAF_ELECTRON_TAIL :
                        res->level = LEAF_DEAD ;
                        break ;
                    case LEAF_DEAD :
                        res->level = LEAF_ALIVE ;
                        break ;

                    default :
                        break ;
                }
            }
            else
            {
                if (qt->level == LEAF_ALIVE)
                    res->level = LEAF_DEAD ;
                else
                    res->level = LEAF_ALIVE ;
            }
        }
        
        return canonicalise(res, hl) ;
    }
    else // Il faut descendre
    {
        int l = 1 << (qt->level - 1);           // Longueur du carré des enfants
        node *n = (node *)malloc(sizeof(node)); // Nouveau noeud pour le résultat final
        n->level = qt->level;
        n->nw = qt->nw;
        n->ne = qt->ne;
        n->sw = qt->sw;
        n->se = qt->se;

        // On cherche dans quel sous-arbre il faut inverser la feuille
        if (x >= l)
        {
            if (y >= l)
                n->se = aux_change_square(qt->se, hl, x - l, y - l, value);
            else
                n->ne = aux_change_square(qt->ne, hl, x - l, y, value);
        }
        else
        {
            if (y >= l)
                n->sw = aux_change_square(qt->sw, hl, x, y - l, value);
            else
                n->nw = aux_change_square(qt->nw, hl, x, y, value);
        }

        return canonicalise(n, hl); // On ne crée évidemment pas de doublon
    }
}

void change_square(hashlife *hl, int x, int y)
{
    // Inverse la couleur de la case de coordonnées (x, y) dans l'arbre (en partant du coin en haut à gauche)
    int l = 1 << hl->root->level;
    if (x >= 0 && y >= 0 && x < l && y < l)                   // On s'assure que la case est bien dans l'arbre
        hl->root = aux_change_square(hl->root, hl, x, y, LEAF_SWITCH); // Inversion
}

void set_square(hashlife *hl, int x, int y, int value)
{
    // Inverse la couleur de la case de coordonnées (x, y) dans l'arbre (en partant du coin en haut à gauche)
    int l = 1 << hl->root->level;
    if (x >= 0 && y >= 0 && x < l && y < l)                      // On s'assure que la case est bien dans l'arbre
        hl->root = aux_change_square(hl->root, hl, x, y, value); // Inversion
}

node *get_leaf(node *qt, int x, int y)
{
    if (qt->level <= 0)
        return qt ;
    else
    {
        int l = 1 << (qt->level - 1) ;
        if (x < l)
        {
            if (y < l)
                return get_leaf(qt->nw, x, y) ;
            else   
                return get_leaf(qt->sw, x, y - l) ;
        }
        else
        {
            if (y < l)
                return get_leaf(qt->ne, x - l, y) ;
            else   
                return get_leaf(qt->se, x - l, y - l) ;
        }
    }
}

void random_config(hashlife *hl)
{
    // Remplace le monde par une configuration aléatoire
    node *qt = random_node(hl->root->level - 2, 1, hl);
    hl->root = blank_border(blank_border(qt, hl), hl);
    hl->gen = 0;
}

void clear_world(hashlife *hl)
{
    // Efface le monde
    hl->root = blank_canonical(hl->root->level, hl);
    hl->gen = 0;
}

void save_world(hashlife *hl)
{
    // Sauve le monde actuel dans le premier fichier de auvegarde libre
    bool file_ok = false;
    int i = 1;
    char filename[64] ;
    while (!file_ok) // Recherche du premier fichier libre
    {
        sprintf(filename, "Saves/Hashlife_save%d.node", i) ;
        FILE *test = fopen(filename, "r");
        if (test == NULL)
            file_ok = true;
        else
            fclose(test);
        ++i;
    }

    node_to_file(hl->root, filename); // Sauvegarde du monde
}



// node *aux_place_structure(node *target, int x, int y, node *structure)
// {
   
//     // On descend dans les feuilles du monde pour les placer dans target
//     // x et y sont les coordonnées du coin supérieur gauche de target dans la structure
//     if (target == NULL)
//         return NULL ;

//     int l_struc = 1 << structure->level ;
//     SDL_Rect r = {0, 0, l_struc, l_struc} ;
//     if (target->level <= 0) // feuille
//     {
//         if (x >= 0 && x < l_struc && y >= 0 && y < l_struc)
//         {
//             node *n = get_leaf(structure, x, y) ;
//             if (n->level == 0)
//                 return n ;
//         }
//     }
//     else // Pas feuille
//     {
//         int l = 1 << (target->level - 1) ;
//         SDL_Rect r2 = {x, y, 2 * l, 2 * l} ;
//         if (SDL_HasIntersection(&r, &r2))
//         {
//             node *nw = aux_place_structure(target->nw, x, y, structure) ;
//             node *ne = aux_place_structure(target->ne, x + l, y, structure) ;
//             node *sw = aux_place_structure(target->sw, x, y + l, structure) ;
//             node *se = aux_place_structure(target->se, x + l, y + l, structure) ;
//             if (nw != target->nw || ne != target->ne || sw != target->sw || se != target->se)
//             {
//                 node *res = (node *)malloc(sizeof(node)) ;
//                 res->level = target->level ;
//                 res->nw = nw ;
//                 res->ne = ne ;
//                 res->sw = sw ;
//                 res->se = se ;
//                 return res ;
//             }
//         }
//     }

//     return target ;
// }

void aux_place_structure(hashlife *hl, int x, int y, node *structure)
{
    if (structure->level <= 0)
    {
        if (structure->level != -1)
            set_square(hl, x, y, structure->level) ;
    }
    else
    {
        int l = 1 << (structure->level - 1) ;
        aux_place_structure(hl, x, y, structure->nw) ;
        aux_place_structure(hl, x + l, y, structure->ne) ;
        aux_place_structure(hl, x, y + l, structure->sw) ;
        aux_place_structure(hl, x + l, y + l, structure->se) ;
    }
}

node *aux_place_node(node *qt, int x, int y, node *n, hashlife *hl)
{
    // Place le noeud n dans le noeud qt
    if (n->level == qt->level)
        return n ;
    else if (n->level < qt->level) 
    {
        int l = 1 << (qt->level - 1);
        node *res = (node *)malloc(sizeof(node)) ;
        res->level = qt->level ;
        res->nw = qt->nw ;
        res->ne = qt->ne ;
        res->sw = qt->sw ;
        res->se = qt->se ;
        if (x < l)
        {
            if (y < l)
                res->nw = aux_place_node(qt->nw, x, y, n, hl) ;
            else
                res->sw = aux_place_node(qt->sw, x, y - l, n, hl) ;
        }
        else
        {
            if (y < l)
                res->ne = aux_place_node(qt->ne, x - l, y, n, hl) ;
            else
                res->se = aux_place_node(qt->se, x - l, y - l, n, hl) ;
        }
        return canonicalise(res, hl) ;
    }
    else
        return qt ;
}

void place_structure(hashlife *hl, int x, int y, int struct_index, int rotation, bool precise_coords)
{
    if (0 <= struct_index && struct_index < hl->nb_structures)
    {
        node *structure = hl->structures[struct_index][rotation];
        structure = canonicalise(structure, hl);

        if (! precise_coords)
        {
            int l = 1 << (structure->level - 1) ;
            x -= (x % l) ;
            y -= (y % l) ;
        }
    

        if (structure->level <= hl->root->level)
        {
            aux_place_structure(hl, x, y, structure) ;
        }
    }
}

void place_node(hashlife *hl, int x, int y, int struct_index, int rotation)
{
    if (0 <= struct_index && struct_index < hl->nb_structures)
    {
        node *structure = hl->structures[struct_index][rotation] ;
        structure = canonicalise(structure, hl);

        if (structure->level <= hl->root->level)
        {
            hl->root = aux_place_node(hl->root, x, y, structure, hl);
        }
    }
}


