#ifndef __ROUTE_H__
#define __ROUTE_H__

#include "lib_io.h"
#include <iostream>
using namespace std;
void deploy_server(char * graph[MAX_EDGE_NUM], int edge_num, char * filename);
void deploy_server_ex(const char *const inputFileName, const char * const outputFileName);

#endif
