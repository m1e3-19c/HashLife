#ifndef __FILE_MANIPULATION_H__
#define __FILE_MANIPULATION_H__

#define STRLEN 512
#define MAX_LINE_COUNT 50

#include <stdlib.h>

#include "Hashlife.h"
#include "Utils.h"

node *file_to_node(char *filename, hashlife *hl) ;
void node_to_file(node *qt, char *filename) ;

node *rle_to_node(char *filename, hashlife *hl) ;
void node_to_rle(node *qt, char *filename) ;

#endif