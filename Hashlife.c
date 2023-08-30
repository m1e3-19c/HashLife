#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "Hashlife.h"
#include "Hashtable.h"
#include "File_manipulations.h"
#include "Miscellaneous.h"

/*
    Life.c
    Implémentation de l'algorithme :
        Canonisation des noeuds
        Calcul des générations suivantes

    Manipulations en tous genres :
        Configurations aléatoires
        Modification du monde en place
        Placement de structures pré-faites
        Lecture et écritures de fichiers pour stockage de configurations
*/

/*---------------------------------------------------*/
// GESTION  DE LA MÉMOIRE :
void free_hashlife(hashlife *hl)
{
    // Libération de toute une structure Hashlife
    node **keys = all_keys(hl->can_tab); // Obtention de tous les arbres canoniques (ce sont les seuls arbres du jeu)
    for (int i = 0; i < hl->can_tab->n; ++i)
    {
        free(keys[i]); // On libère seulement le pointeur car les enfants seront dans une autre case du tableau
    }
    free(keys); // On libère le tableau précédent

    // Libération des trois tables de hachage
    free_hashtable(hl->can_tab);
    free_hashtable(hl->res_tab);
    free_hashtable(hl->superspeed_res_tab);

    free(hl->blank_nodes); // On libère le tableau des noeuds vierges
    for (int i = 0 ; i < hl->nb_structures ; ++i)
        free(hl->structures[i]) ;
    free(hl->structures);  // On libère le tableau des structures
    free(hl);              // On libère le pointeur final
}

// Canonisation des noeuds pour optimiser au mieux
node *canonicalise(node *qt, hashlife *hl)
{
    if (!hashtable_mem(hl->can_tab, qt)) // Le noeud n'est pas canonique, il faut l'ajouter
    {
        if (qt->level > 0) // Noeud interne
        {
            // Canonisation des enfants :
            qt->nw = canonicalise(qt->nw, hl) ;
            qt->ne = canonicalise(qt->ne, hl) ;
            qt->sw = canonicalise(qt->sw, hl) ;
            qt->se = canonicalise(qt->se, hl) ;

            if (!hashtable_mem(hl->can_tab, qt)) // Le noeud n'est toujours pas canonique, même après canonisation de ses enfants
                hashtable_add(hl->can_tab, qt, qt) ;
            else
            {
                // Le noeud admet un représentant canonique (la configuration obtenue à partir de ses enfants est déjà dans la table), dernière vérification pour s'assurer de ne pas avoir de doublon de pointeurs
                node *n = hashtable_get(hl->can_tab, qt) ;
                if (n != qt)
                {
                    free(qt) ;
                    qt = n ;
                }
            }
        }
        else // Feuille
            hashtable_add(hl->can_tab, qt, qt) ;
    }
    else 
    {
        // Le noeud admet un représentant canonique (la configuration obtenue à partir de ses enfants est déjà dans la table), dernière vérification pour s'assurer de ne pas avoir de doublon de pointeurs
        node *n = hashtable_get(hl->can_tab, qt) ;
        if (n != qt)
        {
            free(qt) ;
            qt = n ;
        }
    }   
    
    return qt ;
}

/*---------------------------------------------------*/
// CREATION HASHLIFE
void load_structures(hashlife *hl)
{
    // Importation des strucures depuis les fichiers
    hl->nb_structures = 1 ;
    if (hl->automaton == AUTOMATON_CONWAY_LIFE)
    {
        hl->nb_structures += 5 ;
        hl->structures = (node ***)malloc(hl->nb_structures * sizeof(node **)) ;
        for (int i = 0 ; i < hl->nb_structures ; ++i)
            hl->structures[i] = (node **)malloc(NB_ROTATIONS * sizeof(node *)) ;
        
        hl->structures[1][0] = file_to_node("Structures/Conway_Life/Glider_gun_30.node", hl) ;
        hl->structures[2][0] = file_to_node("Structures/Conway_Life/Glider_gun_60.node", hl) ; 
        hl->structures[3][0] = file_to_node("Structures/Conway_Life/Glider_gun_and_eater.node", hl) ;
        hl->structures[4][0] = file_to_node("Structures/Conway_Life/Reflector_snark.node", hl) ;
        hl->structures[5][0] = file_to_node("Structures/Conway_Life/Breeder1.node", hl) ;
        // hl->structures[5][0] = rle_to_node("Structures/Conway_Life/Breeder1.rle", hl) ;
        // hl->structures[6][0] = file_to_node("Structures/Conway_Life/Metapixel_on.node", hl) ;
        // hl->structures[7][0] = file_to_node("Structures/Conway_Life/Metapixel_off.node", hl) ;
        // hl->structures[8][0] = file_to_node("Structures/Conway_Life/Metapixel_blank.node", hl) ;
        // hl->structures[9] = canonicalise(file_to_node("Structures/Conway_Life/Metapixel_galaxy.node", hl), hl) ;
    }
    else if (hl->automaton == AUTOMATON_WIREWORLD)
    {
        hl->nb_structures += 3 ;
        hl->structures = (node ***)malloc(hl->nb_structures * sizeof(node **)) ;
        for (int i = 0 ; i < hl->nb_structures ; ++i)
            hl->structures[i] = (node **)malloc(NB_ROTATIONS * sizeof(node *)) ;

        hl->structures[1][0] = file_to_node("Structures/WireWorld/Binary_adder.node", hl) ;
        hl->structures[2][0] = file_to_node("Structures/WireWorld/Serial_adder.node", hl) ;
        hl->structures[3][0] = file_to_node("Structures/WireWorld/Cross_wires.node", hl) ;
    }
    

    hl->structures[0][0] = blank_canonical(4, hl);

    for (int i = 0 ; i < hl->nb_structures ; ++i)
    {
        node *qt = hl->structures[i][0] ;
        for (int j = 1 ; j < NB_ROTATIONS ; ++j)
        {
            if (qt->level <= 8)
                qt = rotate_once(qt, hl) ;
            hl->structures[i][j] = qt ;
        }
    }
}

hashlife *create_hashlife(int level, int automaton)
{
    // Création d'une structure Hashlife vierge

    level = max(3, level); // le monde doit être de niveau supérieur ou égal à 3 (déjà pour que ça soit intéressant, puis parce que l'on fait l'aléatoire sur deux niveaux en dessous (pour garder des grandes marges blanches))

    hashlife *hl = (hashlife *)malloc(sizeof(hashlife)); // pointeur principal
    hl->automaton = automaton ;

    printf("automaton %d\n", hl->automaton) ;

    // Tables de hachage
    hl->can_tab = create_hashtable(3);
    hl->res_tab = create_hashtable(3);
    hl->superspeed_res_tab = create_hashtable(3);

    // Tableau des noeuds vierges canoniques initialisé à NULL
    hl->blank_nodes = (node **)malloc((level + 1) * sizeof(node *));
    for (int i = 0; i < level + 1; ++i)
        hl->blank_nodes[i] = NULL;

    // Tableau des structures utiles :
    load_structures(hl);

    hl->gen = 0; // Compteur de l'avancement des générations

    // hl->root = canonicalise(open_rle("Structures/spacefiller2.rle"), hl) ; // Racine du monde vierge
    hl->root = blank_canonical(level, hl); // Racine du monde vierge

    hl->superspeed = false;

    return hl;
}

/*---------------------------------------------------*/
// CALCUL DE L'EVOLUTION :
int rules(node *qt, int nb_neighbours, int automaton)
{
    int res = LEAF_DEAD ;
    
    switch (automaton)
    {
    case AUTOMATON_CONWAY_LIFE : 
        if (nb_neighbours == 3 || (nb_neighbours == 2 && qt->level == LEAF_ALIVE))
            res = LEAF_ALIVE ;
        break;
    
    case AUTOMATON_WIREWORLD : 
        switch (qt->level)
        {   
        case LEAF_ALIVE : // wire
            if (nb_neighbours == 1 || nb_neighbours == 2)
                res = LEAF_ELECTRON_HEAD ;// becomes an electron head
            else
                res = LEAF_ALIVE ;
            break ;
        case LEAF_ELECTRON_HEAD : // electron head
            res = LEAF_ELECTRON_TAIL ; // becomes and electron tail
            break ;
        case LEAF_ELECTRON_TAIL : // electron tail
            res = LEAF_ALIVE ; // becomes a wire
            break ;
        
        default:
            break;
        }
        break ;

    case AUTOMATON_DAY_AND_NIGHT :
        if (nb_neighbours == 3 || nb_neighbours == 6 || nb_neighbours == 7 || nb_neighbours == 8 || (nb_neighbours == 4 && qt->level == 0))
            res = LEAF_ALIVE ;
        break ;

    case AUTOMATON_LIFE_3_4 :
        if (nb_neighbours == 3 || nb_neighbours == 4)
            res = LEAF_ALIVE ;
        break ;

    default:
        break;
    }

    return res ;
}

node *compute_rules(node *qt, hashlife *hl)
{
    // Évolution du noeud qt avec les règles de base (nécessite que le noeud soit de niveau 2) (le résultat est toujours la partie centrale du noeud)
    // On le remplit en regardant les voisins des feuilles centrales
    // Le noeud qt est constitué des 16 feuilles comme suit :
    /*

        1    2        3    4

        5    6        7    8




        9    10       11   12

        13   14       15   16
    */
    // Le but est de calculer le nouvel état des cellules 6, 7, 10 et 11
    node *res = blank_node(1); // Noeud vierge de niveau 1 (qui contiendra les cellules finales 6, 7, 10, 11)

    // Obtention des noeuds numérotés
    node *n1 = qt->nw->nw;
    node *n2 = qt->nw->ne;
    node *n3 = qt->ne->nw;
    node *n4 = qt->ne->ne;

    node *n5 = qt->nw->sw;
    node *n6 = qt->nw->se;
    node *n7 = qt->ne->sw;
    node *n8 = qt->ne->se;

    node *n9 = qt->sw->nw;
    node *n10 = qt->sw->ne;
    node *n11 = qt->se->nw;
    node *n12 = qt->se->ne;

    node *n13 = qt->sw->sw;
    node *n14 = qt->sw->se;
    node *n15 = qt->se->sw;
    node *n16 = qt->se->se;

    // Étant donné qu'un noeud blanc est de niveau -1 et un noeud noir de niveau 0, on remarque que
    // l'on a la somme du nombre de voisins d'une feuille en sommant les niveaux de ses voisins et en ajoutant 8
    node *neighbouring_cells_6[8]  = {n1, n2, n3, n5, n7, n9, n10, n11} ;
    node *neighbouring_cells_7[8]  = {n2, n3, n4, n6, n8, n10, n11, n12} ;
    node *neighbouring_cells_10[8] = {n5, n6, n7, n9, n11, n13, n14, n15} ;
    node *neighbouring_cells_11[8] = {n6, n7, n8, n10, n12, n14, n15, n16} ;

    int nb_alive6 = 0, nb_alive7 = 0, nb_alive10 = 0, nb_alive11 = 0 ;
    for (int i = 0 ; i < 8 ; ++i)
    {
        if ((hl->automaton == AUTOMATON_WIREWORLD && neighbouring_cells_6[i]->level == LEAF_ELECTRON_HEAD) || (hl->automaton != AUTOMATON_WIREWORLD && neighbouring_cells_6[i]->level == LEAF_ALIVE))
            nb_alive6++ ;
        if ((hl->automaton == AUTOMATON_WIREWORLD && neighbouring_cells_7[i]->level == LEAF_ELECTRON_HEAD) || (hl->automaton != AUTOMATON_WIREWORLD && neighbouring_cells_7[i]->level == LEAF_ALIVE))
            nb_alive7++ ;
        if ((hl->automaton == AUTOMATON_WIREWORLD && neighbouring_cells_10[i]->level == LEAF_ELECTRON_HEAD) || (hl->automaton != AUTOMATON_WIREWORLD && neighbouring_cells_10[i]->level == LEAF_ALIVE))
            nb_alive10++ ;
        if ((hl->automaton == AUTOMATON_WIREWORLD && neighbouring_cells_11[i]->level == LEAF_ELECTRON_HEAD) || (hl->automaton != AUTOMATON_WIREWORLD && neighbouring_cells_11[i]->level == LEAF_ALIVE))
            nb_alive11++ ;
    }
        

    // Création des nouveaux noeuds
    res->nw->level = rules(n6, nb_alive6, hl->automaton);
    res->ne->level = rules(n7, nb_alive7, hl->automaton);
    res->sw->level = rules(n10, nb_alive10, hl->automaton);
    res->se->level = rules(n11, nb_alive11, hl->automaton);

    return canonicalise(res, hl); // Ne surtout pas créer de doublon
}

node *evolve(node *qt, hashlife *hl)
{
    // Renvoie le résultat de l'évolution du milieu du noeud qt
    // Le noeud qt se prensente sous cette forme :

    //        1    2    3    4

    //        5    6    7    8

    //        9    10   11   12

    //        13   14   15   16

    // Cette fonction renvoie l'évulotion du centre du noeud, donc le noeud formé par les enfants 6, 7, 10 et 11

    if (hl->superspeed)
    {
        if (hashtable_mem(hl->superspeed_res_tab, qt))
            return hashtable_get(hl->superspeed_res_tab, qt); // Pas besoin de calculer
    }
    else
    {
        if (hashtable_mem(hl->res_tab, qt))
            return hashtable_get(hl->res_tab, qt); // Pas besoin de calculer
    }

    // Il faut alors calculer le résultat
    node *res; // pointeur du résultat que l'on va créer
    if (qt->level == 2)
    {
        res = compute_rules(qt, hl); // Cas de base
    }
    else
    {
        // On crée les 9 noeuds auxiliaires qui permettront de réconstruire le noeud central

        // On décompose qt comme sur le schéma

        // node *n1  = qt->nw->nw ;
        node *n2 = qt->nw->ne;
        node *n3 = qt->ne->nw;
        // node *n4  = qt->ne->ne ;

        node *n5 = qt->nw->sw;
        node *n6 = qt->nw->se;
        node *n7 = qt->ne->sw;
        node *n8 = qt->ne->se;

        node *n9 = qt->sw->nw;
        node *n10 = qt->sw->ne;
        node *n11 = qt->se->nw;
        node *n12 = qt->se->ne;

        // node *n13 = qt->sw->sw ;
        node *n14 = qt->sw->se;
        node *n15 = qt->se->sw;
        // node *n16 = qt->se->se ;

        // On crée les noeuds auxiliaires

        node *aux1 = qt->nw;
        node *aux2 = assemble4(n2, n3, n6, n7, hl); // Assemblage des quatre bon noeuds correspondants (l'avantage est qu'il sera déjà canonisé)
        node *aux3 = qt->ne;
        node *aux4 = assemble4(n5, n6, n9, n10, hl);
        node *aux5 = assemble4(n6, n7, n10, n11, hl);
        node *aux6 = assemble4(n7, n8, n11, n12, hl);
        node *aux7 = qt->sw;
        node *aux8 = assemble4(n10, n11, n14, n15, hl);
        node *aux9 = qt->se;

        // On fait évoluer une première fois les noeuds auxiliaires :
        aux1 = evolve(aux1, hl);
        aux2 = evolve(aux2, hl);
        aux3 = evolve(aux3, hl);
        aux4 = evolve(aux4, hl);
        aux5 = evolve(aux5, hl);
        aux6 = evolve(aux6, hl);
        aux7 = evolve(aux7, hl);
        aux8 = evolve(aux8, hl);
        aux9 = evolve(aux9, hl);

        // Création des quatre noeuds intermédiaires qui vont nous permettre de reconstruire le centre final
        node *inter1 = assemble4(aux1, aux2, aux4, aux5, hl);
        node *inter2 = assemble4(aux2, aux3, aux5, aux6, hl);
        node *inter3 = assemble4(aux4, aux5, aux7, aux8, hl);
        node *inter4 = assemble4(aux5, aux6, aux8, aux9, hl);

        // On revoie l'assemblage de l'évolution des noeuds auxiliaires :
        if (hl->superspeed) // Mode superspeed enclenché, on fait évoluer une deuxième fois les noeuds intermédaires et les centres obtenus donnet le résultat
            res = assemble4(evolve(inter1, hl), evolve(inter2, hl), evolve(inter3, hl), evolve(inter4, hl), hl);
        else // pas de mode superspeed, on récupère juste les centres et on les assemble
            res = assemble4(get_centre(inter1, hl), get_centre(inter2, hl), get_centre(inter3, hl), get_centre(inter4, hl), hl);
    }

    // Le noeud res est déjà canonisé, il reste donc juste à l'ajouter à la table de hachage
    if (hl->superspeed)
        hashtable_add(hl->superspeed_res_tab, qt, res);
    else
        hashtable_add(hl->res_tab, qt, res);

    return res;
}

void next_gen(hashlife *hl)
{
    // On place la racine comme centre d'une cellule vide plus grande :
    node *qt = blank_border(hl->root, hl);

    hl->root = evolve(canonicalise(qt, hl), hl); // On ne garde que le centre
    // On fait avancer du bon nombre de générations
    if (hl->superspeed)
        hl->gen += 1 << (hl->root->level - 1);
    else
        hl->gen++;
}
