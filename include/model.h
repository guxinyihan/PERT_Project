#ifndef MODEL_H
#define MODEL_H
#include "pert_types.h"
/* Initialize before use. Destroy is idempotent and accepts partial objects. */
void project_init(Project *project);
void project_destroy(Project *project);
/* Unfinalized project; copies input strings. On failure project is still owned. */
PertStatus project_add(Project *project, const char *id, const char *description,
                       double a, double m, double b, PertError *error);
/* Freezes indices, builds sorted lookup and empty graph arrays; detects IDs. */
PertStatus project_finalize(Project *project, PertError *error);
/* Finalized project; borrowed ID index is searched without mutation. */
PertStatus project_find(const Project *project, const char *id, size_t *index,
                        PertError *error);
bool model_valid_id(const char *id);
PertStatus model_validate(const char *id, const char *description,
                          double a, double m, double b, PertError *error);
#endif
