#include "input.h"
#include "model.h"
#include "pert.h"
#include "probability.h"
#include "output.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const Project *project;
    const PERTResult *result;
    double deadline;
    size_t ordinal;
} ReportContext;
static PertStatus report_path(const size_t *path, size_t length, void *opaque, PertError *e) {
    ReportContext *context = opaque;
    ProbabilityResult q;
    PertStatus status = probability_calculate(context->result, path, length, context->deadline, &q, e);
    if (status == PERT_OK)
        output_path(stdout, context->project, path, length, ++context->ordinal, &q);
    return status;
}
static char *trim_menu(char *s) {
    while (isspace((unsigned char)*s)) ++s;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    return s;
}
int main(void) {
    for (;;) {
        fputs("\nPERT Scheduling and Probability Analysis\n"
              "1. Load CSV\n2. Enter project by keyboard\n0. Exit\nChoice: ", stdout);
        fflush(stdout);
        char *choice = NULL, *filename = NULL;
        bool eof = false; PertError error = {0};
        PertStatus status = input_read_line(stdin, &choice, &eof, &error);
        if (eof) break;
        if (status != PERT_OK) {
            output_error(stderr, "menu", &error); free(choice);
            if (status == PERT_ERR_MEMORY || status == PERT_ERR_FILE) return EXIT_FAILURE;
            continue;
        }
        char *selected = trim_menu(choice);
        if (!strcmp(selected, "0")) { free(choice); break; }
        bool csv = !strcmp(selected, "1"), keyboard = !strcmp(selected, "2");
        free(choice);
        if (!csv && !keyboard) { fputs("Choose 0, 1 or 2.\n", stdout); continue; }
        Project project; project_init(&project);
        PERTResult result; pert_result_init(&result);
        const char *source = "keyboard";
        if (csv) {
            fputs("CSV filename (no surrounding quotes): ", stdout); fflush(stdout);
            status = input_read_line(stdin, &filename, &eof, &error);
            if (status == PERT_OK && eof)
                status = pert_error(&error, PERT_ERR_INPUT, 0, NULL, "Input ended; current analysis cancelled.");
            if (status == PERT_OK) { source = trim_menu(filename); status = input_csv(source, &project, &error); }
        } else status = input_keyboard(stdin, stdout, &project, &error);
        if (status == PERT_OK) status = pert_calculate(&project, &result, &error);
        double deadline = 0;
        if (status == PERT_OK) status = input_deadline(stdin, stdout, &deadline, &error);
        if (status == PERT_OK) {
            output_schedule(stdout, &project, &result);
            ReportContext context = {&project, &result, deadline, 0};
            PathSummary summary;
            status = pert_enumerate_paths(&project, &result, report_path, &context, &summary, &error);
            if (status == PERT_OK) output_summary(stdout, &summary, deadline);
            else fputs("Analysis incomplete: path/probability report aborted.\n", stdout);
        }
        if (status != PERT_OK) output_error(stderr, source, &error);
        pert_result_destroy(&result); project_destroy(&project); free(filename);
        if (feof(stdin)) break;
    }
    fputs("Goodbye.\n", stdout); return EXIT_SUCCESS;
}
