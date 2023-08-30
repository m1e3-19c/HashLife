#ifndef __GAME_H__
#define __GAME_H__

#include "Screen_for_SDL.h"

#include "Utils.h"
#include "Hashlife.h"
// #include "Hashtable.h"

typedef struct game
{
    int gen ;
    hashlife *world ;
    screen *screen ;

    int2 camera_pos ;
    float square_size ;
    int fps ;

    bool launched ;
    bool pause ;

    bool edition_mode ;
    bool precise_coords ;
    int struct_index ;
    int rotation ;
} game ;

game *new_game(int w, int h, float square_size, int fps, int world_level, int automaton) ;
void free_game(game *g) ;

/// Affichage d'une cellule noire de niveau 0
void print_square(game *g, int x, int y) ;
/// Fonction auxiliaire pour l'affichage du monde. Affiche le noeud qt à la position (x, y) (coin supérieur gauche)
void print_aux(game *g, node *qt, float x, float y, int automaton) ;

/// Affichage du monde
void print_game(game *g) ;

/// Événements utilisateur
void events(game *g) ;

/// Lancement du jeu
void run(game *g) ;

#endif