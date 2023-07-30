#include <stdio.h>
#include <stdlib.h>

#include "File_manipulations.h"
#include "Hashlife.h"
#include "Utils.h"

// GESTION DES FICHIERS

/*---------------------------------------------------*/
// FICHIERS .NODE
// Lecture
node *aux_file_to_node(int level, FILE *file, int count, int value, int *count_res, int *value_res, hashlife *hl)
{
    // Lecture d'un arbre écrit dans le fichier. Le but est de remonter la compression précédente. Comme on dispose du compteur, il s'agit d'initialiser les count feuilles suivantes, puis on prend le couple suivant
    // count est le nombre de feuilles de valeur value qu'il reste à initialiser
    // value est la valeur à donner aux count feuilles suivantes
    // count_res et value_res sont les count et value à la sortie de la fonction (pour être récupérés par les parents)
    
    node *qt = (node *)malloc(sizeof(node)) ; // nouveau noeud pour le résultat
    if (level == 0) // feuille
    {
        if (count == 0) // plus de feuille de la couleur value, on lit une nouvelle ligne
        {
            fscanf(file, "%d %d\n", &count, &value) ;
            *value_res = value ; // On actualise les pointeurs pour les parents
            *count_res = count ;
        }
        qt->level = value ; // feuille de la valeur actuelle
        qt->nw = NULL ;
        qt->ne = NULL ;
        qt->sw = NULL ;
        qt->se = NULL ;

        count -- ; // Une feuille de moins de cette valeur
    }
    else // noeud interne
    {
        // On prépare les appels récursifs pour récupérer un noeud de niveau inférieur
        qt->level = level ;
        qt->nw = aux_file_to_node(level - 1, file, count, value, &count, &value, hl) ;
        qt->ne = aux_file_to_node(level - 1, file, count, value, &count, &value, hl) ;
        qt->sw = aux_file_to_node(level - 1, file, count, value, &count, &value, hl) ;
        qt->se = aux_file_to_node(level - 1, file, count, value, &count, &value, hl) ;
    }

    *count_res = count ; // On actualise les pointeurs pour les parents
    *value_res = value ;

    return canonicalise(qt, hl) ;
}

node *file_to_node(char *filename, hashlife *hl)
{
    FILE *file = fopen(filename, "r") ; // Ouverture en mode lecture
    node *qt = NULL ;
    if (file != NULL)
    {
        int level ;
        int count, value ;
        fscanf(file, "%d %d\n", &count, &level) ; // Lecture de la première ligne : taille du noeud final (la valeur de count est ignorée)
        fscanf(file, "%d %d\n", &count, &value) ; // Lecture de la deuxième ligne : informations sur les premières feuilles pour lancer les appels récursifs
        qt = aux_file_to_node(level, file, count, value, &count, &value, hl) ; // Obtention du noeud final
    }

    fclose(file) ;

    return qt ;
}

/*---------------------------------------------------*/
// Écriture
int2 aux_node_to_file(node *qt, FILE *file, int count, int value)
{
    // Écriture du noeud dans le fichier file (de manière la plus compressée possible) on renvoie un int2 {count, value} pour être récupéré par après l'appel récursif
    // La méthode de compression consiste à écrire le nombre de lignes donnant la même valeur (un couple (count, value)) pour être récupéré plus tard et initialiser count feuilles à la valeur value

    int2 res = {count, value} ;

    if (qt->level <= 0)
    {
        if (qt->level != value) // Après les appels récursifs, on tombe sur une case de couleur différente : on écrit le couple (count, value) précédent et on démarre un nouveau compteur
        {   
            fprintf(file, "%d %d\n", count, value) ;
            count = 0 ;
            value = qt->level ;
        }
        res = (int2){count + 1, value} ; // Incrémentation du compteur à chaque feuille trouvée
    }
    else
    {
        // Appels récursifs pour écrire les enfants
        res = aux_node_to_file(qt->nw, file, count, value) ;
        res = aux_node_to_file(qt->ne, file, res.x, res.y) ;
        res = aux_node_to_file(qt->sw, file, res.x, res.y) ;
        res = aux_node_to_file(qt->se, file, res.x, res.y) ;
    }

    return res ;
}

void node_to_file(node *qt, char *filename)
{
    FILE *file = fopen(filename, "w") ; // Ouverture en mode écriture
    if (file != NULL)
    {
        int2 res = aux_node_to_file(qt, file, 1, qt->level) ; 
        fprintf(file, "%d %d\n", res.x, res.y) ; // Écriture finale correspondant à la couleur de la dernière feuille qui n'a pas été écrite
    }

    fclose(file) ;
}


/*---------------------------------------------------*/
// FICHIERS RLE
// Lecture
void rle_new_line(FILE *f, char *s)
{
    // Ligne suivante en ignorant les commentaires
    fgets(s, STRLEN, f) ;
    while (s[0] == '#')
        fgets(s, STRLEN, f) ;
}


bool rle_next_task(FILE *f, char *s, int *i, int *x, int *y, int **matrix)
{
    // Effectue la prochaine tâche de lecture du fichier RLE
    int n = 0;
    char tmp = s[*i] ;
    if (tmp == '!') // Fin de fichier
        return true ;
    
    if (tmp == 13 || tmp == 10) // Fin de ligne
    {
        *i = 0 ;
        rle_new_line(f, s) ;
        tmp = s[*i] ;
    }


    if ((int)tmp < 48 || (int)tmp > 57) // Lecture du nombre
        n = 1 ;
    else while ((int)tmp >= 48 && (int)tmp <= 57)
    {
        n = 10 * n + (int)tmp - 48 ;
        *i += 1 ;
        tmp = s[*i] ;
    }

    char c = s[*i] ; // Lecture de l'action à faire
    if (c  == '$') // Nouvelle ligne 
        for (int j = 0 ; j < n ; ++j)
        {
            *x = 0 ;
            *y += 1 ;
        }
    else
    {
        if (c == 'o') // Cellules vivantes
            for (int j = 0 ; j < n ; ++j)
                matrix[*x + j][*y] = 0 ;
        
        *x += n ;
    }
    *i += 1 ; 

    return false ;
}

int **open_rle(char *filename, int *p)
{
    // Ouverture du fichier RLE, renvoie la matrice correspondante 
    FILE *f = fopen(filename, "r") ;

    int **matrix ;

    if (f != NULL)
    {
        int w, h ;
        char s[STRLEN] ;
        rle_new_line(f, s) ;
        sscanf(s, "x = %d, y = %d%s", &w, &h, s) ; // Lecture des dimensions


        *p = max(intlog2(w), intlog2(h)) + 1; // Carré le plus petit contenant la configuration
        int l = 1 << *p ;

        // Création de la matrice vierge correspondante
        matrix = (int **)malloc(l * sizeof(int *)) ;
        for (int i = 0 ; i < l ; ++i)
        {
            matrix[i] = (int *)malloc(l * sizeof(int)) ;
            for (int j = 0 ; j < l ; ++j)
                matrix[i][j] = -1 ;
        }

        int i = 0, x = 0, y = 0 ;
        fgets(s, STRLEN, f) ;
        while (!rle_next_task(f, s, &i, &x, &y, matrix)) // Décompression
        {
        }
    }

    return matrix ;
}


// Manipulation de la matrice intermédiaire :
node *aux_matrix_to_node(int **matrix, int x, int y, int p, hashlife *hl)
{
    node *res = (node *)malloc(sizeof(node)) ;
    if (p == 0) // Feuille
    {
        res->level = matrix[x][y] ;
        res->nw = NULL ;
        res->ne = NULL ;
        res->sw = NULL ;
        res->se = NULL ;
    }
    else // Noeud interne
    {
        int l = 1 << (p - 1) ;
        res->level = p ;
        res->nw = aux_matrix_to_node(matrix, x, y, p - 1, hl) ;
        res->ne = aux_matrix_to_node(matrix, x + l, y, p - 1, hl) ;
        res->sw = aux_matrix_to_node(matrix, x, y + l, p - 1, hl) ;
        res->se = aux_matrix_to_node(matrix, x + l, y + l, p - 1, hl) ;
    }

    return canonicalise(res, hl) ;
}

node *matrix_to_node(int **matrix, int p, hashlife *hl)
{
    // Crée un noeud à partir d'une matrice de côté 2^p
    return aux_matrix_to_node(matrix, 0, 0, p, hl) ;
}

node *rle_to_node(char *filename, hashlife *hl)
{
    int p ;
    int **matrix = open_rle(filename, &p) ;

    return canonicalise(matrix_to_node(matrix, p, hl), hl) ;
}

/*---------------------------------------------------*/
// Écriture :
void print_in_file(FILE *f, int count, int val, int *line_count)
{
    char c = (val == 0) ? 'o' : 'b' ;
    fprintf(f, "%d%c", count, c) ;
    *line_count += 2 ;
    if (*line_count > MAX_LINE_COUNT)
    {
        fprintf(f, "\n") ;
        *line_count = 0 ;
    }
        
}

void matrix_to_rle(int **matrix, int p, FILE *f)
{
    int l = 1 << p ; // Taille de la cellule
    fprintf(f, "x = %d, y = %d, rule = b3/s23\n", l, l) ;
    int line_count = 0 ;
    for (int i = 0 ; i < l ; ++i)
    {
        int count = 0, val = matrix[i][0] ;
        for (int j = 0 ; j < l ; ++j)
        {
            if (matrix[i][j] == val)
                ++count ;
            else
            {
                print_in_file(f, count, val, &line_count) ;
                val = matrix[i][j] ;
                count = 1 ;
            }
        }
        print_in_file(f, count, val, &line_count) ;
        fprintf(f, "$") ;
        ++line_count ;
    }
    fprintf(f, "!\n") ;
}

void aux_node_to_matrix(node *qt, int x, int y, int **matrix)
{
    if (qt->level <= 0)
        matrix[x][y] = qt->level ;
    else
    {
        int l = 1 << (qt->level - 1) ;
        aux_node_to_matrix(qt->nw, x, y, matrix) ;
        aux_node_to_matrix(qt->ne, x, y + l, matrix) ;
        aux_node_to_matrix(qt->sw, x + l, y, matrix) ;
        aux_node_to_matrix(qt->se, x + l, y + l, matrix) ;
    }
}

int **node_to_matrix(node *qt)
{
    int l = 1 << qt->level ;
    int **matrix = (int **)malloc(l * sizeof(int *)) ;
    for (int i = 0 ; i < l ; ++i)
        matrix[i] = (int *)malloc(l * sizeof(int)) ;

    aux_node_to_matrix(qt, 0, 0, matrix) ;

    return matrix ;
}

void node_to_rle(node *qt, char *filename)
{
    int l = 1 << qt->level ;
    int **matrix = node_to_matrix(qt) ;
    
    FILE *f = fopen(filename, "w") ;
    if (f != NULL)
    {
        matrix_to_rle(matrix, qt->level, f) ;
        fclose(f) ;
    }
    

    for (int i = 0 ; i < l ; ++i)
        free(matrix[i]) ;
    free(matrix) ;
}