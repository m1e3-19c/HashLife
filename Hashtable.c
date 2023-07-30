#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "Hashtable.h"
#include "Utils.h"

long hash(node *key, hashtable *t)
{
    // Hachage équivalent à celui d'un point en 4D :
    long n = (long)key->nw + MAGICAL_CONST * ((long)key->ne + MAGICAL_CONST * ((long)key->sw + MAGICAL_CONST * (long)key->se)) ;
    return (n * MAGICAL_CONST) >> (64 - t->p) ; // Formule magique...
}

hashtable *create_hashtable(int p)
{
    hashtable *t = (hashtable *)malloc(sizeof(hashtable)) ;
    t->p = p ; // capacité
    t->n = 0 ; // vide
    t->data = (bucket **)malloc((1 << p) * sizeof(bucket *)) ;
    for (int i = 0 ; i < 1 << t->p ; ++i) // initialisation à des seaux vides
        t->data[i] = NULL ;

    return t ;
}

void flush_hashtable(hashtable *t)
{
    // Vidage de la table pour un redimensionnement
    for (int i = 0 ; i < 1 << t->p ; ++i)
    {
        bucket *current = t->data[i] ;
        bucket *next = NULL ;
        while (current != NULL)
        {
            next = current->next ;
            free(current) ;
            current = next ;
        }
    }
}

void free_hashtable(hashtable *t)
{
    // Libère la mémoire de la table SANS LIBÉRER LA MÉMOIRE DES CLÉS ET VALEURS
    flush_hashtable(t) ;
    free(t->data) ;
    free(t) ;
}

void temp_hashtable_add(hashtable *t, node *key, node *value)
{
    // Ajout temporaire sans redimensionner (On suppose que le couple (clé, valeur) n'est pas déjà dans la table)
    // Création d'un nouveau seau
    int i = hash(key, t) ;
    bucket *b = (bucket *)malloc(sizeof(bucket)) ;
    b->key = key ;
    b->value = value ;

    // Insertion
    b->next = t->data[i] ;
    t->data[i] = b ;
    t->n ++ ;
}

void hashtable_resize(hashtable *t, int p)
{
    // Redimensionnement de la table : On crée une nouvelle table deux fois plus grande et on ajout les éléments un par un
    hashtable *new = create_hashtable(p) ;
    for (int i = 0 ; i < 1 << t->p ; ++i)
    {
        bucket *current = t->data[i] ;
        while (current != NULL)
        {
            temp_hashtable_add(new, current->key, current->value) ;
            current = current->next ;
        }
    }

    flush_hashtable(t) ; // Vidage de l'ancienne table
    // Échange des tables :
    free(t->data) ;
    t->p = p ;
    t->data = new->data ;
    free(new) ;
}

void hashtable_add(hashtable *t, node *key, node *value)
{
    //(On suppose que le couple (clé, valeur) n'est pas déjà dans la table)
    if (t->n >= 1 << (t->p + 2)) // Redimensionnement si trop de valeurs déjà présentes
        hashtable_resize(t, t->p + 1) ;
    temp_hashtable_add(t, key, value) ;
}

bool hashtable_mem(hashtable *t, node *key)
{
    // Renvoie true si l'arbre caninique de key est présent dans la table, false sinon
    int i = hash(key, t) ; // hachage
    bucket *current = t->data[i] ;
    while (current != NULL) // parcours du seau
    {
        if (same_nodes(current->key, key)) // la clé a été trouvée
            return true ;
        current = current->next ;
    }
    return false ; // après parcours du seau, la clé n'a pas été trouvée
}

node *hashtable_get(hashtable *t, node *key)
{
    // Renvoie la valeur asséociée à la clé key dans la table
    int i = hash(key, t) ; // hachage
    bucket *current = t->data[i] ;
    while (current != NULL) // parcours du seau
    {
        if (same_nodes(current->key, key)) // la clé a été trouvée
            return current->value ;
        current = current->next ;
    }
    return NULL ; // après parcours du seau, la clé n'a pas été trouvée : erreur
}

node **all_keys(hashtable *t)
{
    // Liste de toutes les clés de la table :
    node **res = (node **)malloc(t->n * sizeof(node *)) ;
    int i = 0 ;
    for (int j = 0 ; j < 1 << t->p ; ++j)
    {
        bucket *current = t->data[j] ;
        while (current != NULL) // ajout des clés au résultat
        {
            res[i] = current->key ;
            ++i ;
            current = current->next ;
        }
    }
    return res ;
}

void analyse(hashtable *t)
{
    // Analyse de la table : Vérification de performances en tout genre
    printf("Nombre d'éléments dans la table : %d\n", t->n) ;

    int nb_vides = 0 ; // nombre de seaux vides 
    int nb_sup5  = 0 ; // nombre de seaux à plus de 5 éléments
    int nb_sup10 = 0 ; // nombre de seaux à plus de 10 éléments
    int nb_sup15 = 0 ; // nombre de seaux à plus de 15 éléments
    for (int i = 0 ; i < 1 << t->p ; ++i)
    {
        int s = 0 ;
        bucket *current = t->data[i] ;
        while (current != NULL)
        {
            s += 1 ;
            current = current->next ;
        }

        if (s >= 15)
            nb_sup15 ++ ;
        if (s >= 10)
            nb_sup10 ++ ;
        if (s >= 5)
            nb_sup5 ++ ;
        if (s == 0)
            nb_vides ++ ;
    }

    printf("Nombre de seaux vides : %d\nNombre de seaux à au moins 5 éléments : %d\nNombre de seaux à au moins 10 éléments : %d\nNombre de seaux à au moins 15 éléments : %d\n--------------------------\n", nb_vides, nb_sup5, nb_sup10, nb_sup15) ;
}