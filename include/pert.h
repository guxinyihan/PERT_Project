#ifndef PERT_H
#define PERT_H
#include "pert_types.h"
void pert_result_init(PERTResult *result);
void pert_result_destroy(PERTResult *result);
/* Initialized empty output, finalized project; no input/output side effects. */
PertStatus pert_calculate(const Project *project, PERTResult *result, PertError *error);
/* Existing edge is required as well as critical endpoints and time continuity. */
bool pert_critical_edge(const Project *project, const PERTResult *result,
                        size_t from, size_t to);
/* Matching successful result/project; at most 100 callbacks, stops at path 101.
   Engine owns all traversal memory. Callback errors propagate after cleanup. */
PertStatus pert_enumerate_paths(const Project *project, const PERTResult *result,
                                PathCallback callback, void *context,
                                PathSummary *summary, PertError *error);
#endif
