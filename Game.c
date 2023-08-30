#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <SDL2/SDL.h>

#include "Screen_for_SDL.h"

#include "Game.h"
#include "Utils.h"
#include "Hashlife.h"
#include "File_manipulations.h"
#include "Miscellaneous.h"

#include "Hashtable.h"

game *new_game(int w, int h, float square_size, int fps, int world_level, int automaton)
{
    // ATTENTION : world_level > 2 !
    game *g = (game *)malloc(sizeof(game)) ;
    g->gen = 0 ;
    g->world = create_hashlife(world_level, automaton) ;
    // g->world->root = rle_to_node("Structures/otcametapixel.rle", g->world) ;

    g->screen = create_screen(w, h, "The Game of Life") ;

    int l_sur_2 = 1 << (world_level - 1) ; // Moitié de la longueur du monde

    g->launched = true ;
    g->camera_pos = (int2){l_sur_2, l_sur_2} ; // caméra centrée
    g->square_size = square_size ; // taille des cases
    g->fps = fps ; // nombre d'appels à next_gen par seconde
    g->pause = true ; 

    g->edition_mode = false ; // Le mode édition de structures n'est pas activé
    g->precise_coords = true ;
    g->struct_index = 0 ;
    g->rotation = 0 ;

    return g ;
}

void free_game(game *g)
{
    free_hashlife(g->world) ;
    free_screen(g->screen) ;
    free(g) ;
}

void print_square(game *g, int x, int y)
{  
    if (g->square_size > 1)
    {
        // Rectangle SDL pour la case à afficher :
        SDL_Rect r = {x, y, (int)g->square_size + 1, (int)g->square_size + 1} ;
        SDL_RenderFillRect(g->screen->renderer, &r) ; // Affichage
    }
    else
    {
        SDL_RenderDrawPoint(g->screen->renderer, x, y) ;
    }
}

void print_aux(game *g, node *qt, float x, float y, int automaton)
{
    // Fonction auxiliaire pour l'affichage du monde, x et y représentent les coordonnées sur l'écran du coin supérieur gauche de l'arbre considéré
    if (qt->level <= 0 && qt->level != -1) // Cellule de niveau 0 non vide
    {
        if (automaton == AUTOMATON_WIREWORLD)
        {
            if (qt->level == LEAF_ALIVE) // wire
                SDL_SetRenderDrawColor(g->screen->renderer, 255, 200, 0, SDL_ALPHA_OPAQUE) ;
            else if (qt->level == LEAF_ELECTRON_HEAD) // electron head
                SDL_SetRenderDrawColor(g->screen->renderer, 0, 150, 190, SDL_ALPHA_OPAQUE) ;
            else if (qt->level == LEAF_ELECTRON_TAIL) // electron tail
                SDL_SetRenderDrawColor(g->screen->renderer, 130, 70, 0, SDL_ALPHA_OPAQUE) ;
        }
        print_square(g, (int)x, (int)y) ;
    }
    else if (qt->level > 0 && g->world->blank_nodes[qt->level] != qt) // Cellule de niveau supérieur non vierge : on affiche les sous-cellules à l'endroit approprié
    {
        float cell_size = (float)(1 << (qt->level - 1)) ;
        float l = cell_size * g->square_size ;

        // Vérification taille pixel :
        if (l <= 0.6)
        {
            SDL_RenderDrawPoint(g->screen->renderer, x, y) ;
        }
        else
        {
            // Vérification sortie d'écran :
            SDL_Rect a = {0, 0, g->screen->w, g->screen->h} ;
            int rect_len = ((int)l == 0) ? 1 : 2 * l ;
            SDL_Rect b = {x, y, rect_len, rect_len} ;

            if (SDL_HasIntersection(&a, &b))
            {
                print_aux(g, qt->nw, x, y, automaton) ;
                print_aux(g, qt->ne, x + l, y, automaton) ;
                print_aux(g, qt->sw, x, y + l, automaton) ;
                print_aux(g, qt->se, x + l, y + l, automaton) ;
            }
        }
    }
}

void print_game_area(game *g, int x, int y)
{
    // Affiche le carré du monde (coordonnées du coin supérieur gauche : x, y) :
    int world_length = (1 << g->world->root->level) ;
    int chunk_length = 32 ; 
    int l = chunk_length * g->square_size ;
    SDL_Rect r  = {x, y, world_length * g->square_size, world_length * g->square_size} ;

    SDL_SetRenderDrawColor(g->screen->renderer, 30, 30, 30, SDL_ALPHA_OPAQUE) ; // dark background color
    SDL_RenderFillRect(g->screen->renderer, &r) ;

    // Affichage des petites parcelles si l'on a assez zoomé :
    SDL_SetRenderDrawColor(g->screen->renderer, 40, 40, 40, SDL_ALPHA_OPAQUE) ; // a liitle lighter
    
    if (g->square_size > 4.)
    {
        r.h = r.w = l ;
        int x_orig = x % l - l, y_orig = y % l - l ;
        int x_max = x + world_length * g->square_size ;
        int y_max = y + world_length * g->square_size ;
        int a = 0, b = 0 ;
        for (int i = x_orig ; i < g->screen->w ; i += l)
        {
            b = 0 ;
            for (int j = y_orig ; j < g->screen->h ; j += l)
            {
                if (a % 2 == b % 2 && i >= x && j >= y && i < x_max && j < y_max)
                {
                    r.x = i ;
                    r.y = j ;
                    SDL_RenderFillRect(g->screen->renderer, &r) ;
                }
                ++ b ;
            }
            ++a ;
        }
    }        
}

void print_grid(game *g, int x, int y)
{
    // Affiche la grille par dessus le monde (coordonnées du coin supérieur gauche : x, y) :
    SDL_SetRenderDrawColor(g->screen->renderer, 16, 16, 16, SDL_ALPHA_OPAQUE) ;
    int x_orig = (x % (int)g->square_size) - g->square_size ;
    int y_orig = (y % (int)g->square_size) - g->square_size ;
    int x_min = max(0, x) ;
    int y_min = max(0, y) ;
    int l = 1 << g->world->root->level ;
    int x_max = min(g->screen->w, x + g->square_size * l) ;
    int y_max = min(g->screen->h, y + g->square_size * l) ;
    for (int i = x_orig ; i <= g->screen->w ; i += g->square_size)
        if (i >= x_min && i < x_max)
            SDL_RenderDrawLine(g->screen->renderer, i, y_min, i, y_max) ;

    for (int j = y_orig ; j <= g->screen->h ; j += g->square_size)
        if (j >= y_min && j < y_max)
            SDL_RenderDrawLine(g->screen->renderer, x_min, j, x_max, j) ;
}

void print_game(game *g)
{
    // Effaçage :
    SDL_SetRenderDrawColor(g->screen->renderer, 127, 127, 127, SDL_ALPHA_OPAQUE) ;
    SDL_RenderClear(g->screen->renderer) ;

    // Affichage :
    int x = g->screen->w / 2 - g->camera_pos.x * g->square_size ;
    int y = g->screen->h / 2 - g->camera_pos.y * g->square_size ;
    
    print_game_area(g, x, y) ;

    // Détermination des coordonnées du monde sur l'écran :
    SDL_SetRenderDrawColor(g->screen->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE) ;
    print_aux(g, g->world->root, x, y, g->world->automaton) ; // Affichage du monde depuis la racine

    if (g->square_size >= 4.)
        print_grid(g, x, y) ;
    
    
    SDL_RenderPresent(g->screen->renderer) ;
}

void edit(game *g, int x, int y)
{
    // Calcul des coordonées :
    int x2 = (x - g->screen->w / 2) ;
    int x_world = x2 / g->square_size + g->camera_pos.x ;

    int y2 = (y - g->screen->h / 2) ;
    int y_world = y2 / g->square_size + g->camera_pos.y ;

    if (g->edition_mode)
    {
        if (g->struct_index == 0) // eraser
            place_node(g->world, x_world, y_world, g->struct_index, g->rotation) ;
        else
            place_structure(g->world, x_world, y_world, g->struct_index, g->rotation, g->precise_coords) ;
    }
        
    else
        change_square(g->world, x_world, y_world) ;
}

void events(game *g)
{
    SDL_Event e ; // Événement SDL à analyser
    while (SDL_PollEvent(&e))
    {
        switch (e.type) // Analyse du type d'événement
        {
            case SDL_QUIT : // L'utilisateur quitte
                g->launched = false ;
                break ;
            
            case SDL_WINDOWEVENT : // L'utilisateur modifie la fenêtre (cas d'un redimensionnement)
                if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    g->screen->w = e.window.data1 ;
                    g->screen->h = e.window.data2 ;
                }
                break ;

            case SDL_KEYDOWN :
                switch (e.key.keysym.sym)
                {   
                    case SDLK_UP : // Zoom in
                        g->square_size *= 2. ;
                        break ;
                    case SDLK_DOWN : // Zomm out
                        g->square_size /= 2. ;
                        break ;

                    case SDLK_LEFT : // Faster
                        if (g->fps > 1)
                            g->fps /= 2 ;
                        break ;
                    case SDLK_RIGHT : // Slower
                        if (g->fps < 256)
                            g->fps *= 2 ;
                        break ;

                    case SDLK_z : // move up
                        g->camera_pos.y -= (50 / g->square_size + 1) ;
                        break ;
                    case SDLK_s : // move down
                        g->camera_pos.y += (50 / g->square_size + 1) ;
                        break ;
                    case SDLK_q : // move left
                        g->camera_pos.x -= (50 / g->square_size + 1) ;
                        break ;
                    case SDLK_d : // move right
                        g->camera_pos.x += (50 / g->square_size + 1) ;
                        break ;

                    case SDLK_SPACE : // pause
                        g->pause = !g->pause ;
                        break ;

                    case SDLK_ESCAPE : // Clear
                        clear_world(g->world) ;
                        break ;

                    case SDLK_x : // Random configuration at the centre
                        random_config(g->world) ;
                        break ;

                    case SDLK_h : // Superspeed mode !
                        g->world->superspeed = !g->world->superspeed ;
                        break ;

                    case SDLK_p : // print current world in a file
                        save_world(g->world) ;
                        break ;

                    case SDLK_a : // Analysis of the hashtables
                        analyse(g->world->can_tab) ;
                        analyse(g->world->res_tab) ;
                        analyse(g->world->superspeed_res_tab) ;
                        break ;

                    case SDLK_m : // edition mode
                        g->edition_mode = !g->edition_mode ;
                        break ;

                    case SDLK_TAB : // Rotate structures
                        g->rotation = (g->rotation + 1) % 4 ;
                        break ;

                    case SDLK_0 : // Structure index set to 0
                        g->struct_index = 0 ;
                        break ;
                    case SDLK_1 : // Structure index set to 1
                        g->struct_index = 1 ;
                        break ;
                    case SDLK_2 : // Structure index set to 2
                        g->struct_index = 2 ;
                        break ;
                    case SDLK_3 : // Structure index set to 3
                        g->struct_index = 3 ;
                        break ;
                    case SDLK_4 : // Structure index set to 4
                        g->struct_index = 4 ;
                        break ;
                    case SDLK_5 : // Structure index set to 5
                        g->struct_index = 5 ;
                        break ;
                    case SDLK_6 : // Structure index set to 6
                        g->struct_index = 6 ;
                        break ;
                    case SDLK_7 : // Structure index set to 7
                        g->struct_index = 7 ;
                        break ;
                    case SDLK_8 : // Structure index set to 8
                        g->struct_index = 8 ;
                        break ;
                    case SDLK_9 : // Structure index set to 9
                        g->struct_index = 9 ;
                        break ;

                    case SDLK_c : // Precise coordinates or not
                        g->precise_coords = !g->precise_coords ;
                        break ;

                    default :
                        break ;
                }

                break ;

            case SDL_MOUSEBUTTONDOWN : // Click ! edit
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    edit(g, e.button.x, e.button.y) ;
                }
                

            default :
                break ;
        }
    }
}


void run(game *g)
{
    while (g->launched) // Boucle principale
    {
        int t1, t2 ; // pour gérer les fps
        t1 = SDL_GetTicks() ;
        events(g) ;
        print_game(g) ;

        if (!g->pause) // Nouvelle génération
        {
            next_gen(g->world) ;
            printf("Génération %d\n", g->world->gen) ;
        }

        t2 = SDL_GetTicks() ; 

        float dt = t2 - t1 ;
        if (dt < 1000 / g->fps) // fps
            SDL_Delay(1000 / g->fps - dt) ;
    }
}

int main(int argc, char **argv)
{
    if (argc == 3)
    {
        srand(time(NULL)) ; // Random seed
        
        int p = atoi(argv[1]); // Taille du monde
        int automaton = atoi(argv[2]) ;
        game *g = new_game(800, 600, 5, 8, p, automaton) ; // Jeu

        run(g) ;

        free_game(g) ;
    }
    else
        printf("Il faut entrer deux arguments : la taille du monde et l'automate voulu\n") ;

    return 0 ;
}
