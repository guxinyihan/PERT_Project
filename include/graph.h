#ifndef GRAPH_H
#define GRAPH_H
#include "pert_types.h"
/* Finalized project, valid endpoints; commits both adjacency entries together. */
PertStatus graph_add_edge(Project *project, size_t from, size_t to, PertError *error);
/* Nonempty finalized project. Caller owns *order on success; NULL on failure. */
PertStatus graph_topological(const Project *project, size_t **order, PertError *error);
#endif
