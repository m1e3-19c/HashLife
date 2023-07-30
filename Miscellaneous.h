#ifndef __MISCELLANEOUS_H__
#define __MISCELLANEOUS_H__

#include "Hashlife.h"
#include "Utils.h"
#include "File_manipulations.h"

node *random_node(int level, int p, hashlife *hl) ;
node *blank_node(int level) ;
node *blank_canonical(int level, hashlife *hl) ;
node *blank_border(node *qt, hashlife *hl) ;
node *assemble4(node *nw, node *ne, node *sw, node *se, hashlife *hl) ;
node *get_centre(node *qt, hashlife *hl) ;
node *aux_change_square(node *qt, hashlife *hl, int x, int y, int value) ;
void change_square(hashlife *hl, int x, int y) ;
void set_square(hashlife *hl, int x, int y, int value) ;
void random_config(hashlife *hl) ;
void clear_world(hashlife *hl) ;
void save_world(hashlife *hl) ;
void place_structure(hashlife *hl, int x, int y, int struct_index, int rotation, bool precise_coords) ;
void place_node(hashlife *hl, int x, int y, int struct_index, int rotation) ;
node *copy_node(node *qt) ;
node *rotate_once(node *qt, hashlife *hl) ;
node *rotate_node(node *qt, int rotation) ;

#endif