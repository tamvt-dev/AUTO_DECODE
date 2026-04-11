#ifndef PIPELINE_H
#define PIPELINE_H
#include <glib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "buffer.h"

typedef struct {
    char *name;
    Buffer buf;
} StepInfo;

typedef struct Candidate {
    Buffer buf;
    double score;
    GList *history;    // list of StepInfo*
    GList *steps;      // LEGACY: list of strings (for compatibility if needed, but we'll prefer history)
    char *meta;        // extra metadata
} Candidate;

// Beam search pipeline
GList* pipeline_beam_search(Buffer input, int max_depth, int beam_width);

// Structured multi-stage pipeline:
// Input -> Heuristic -> Planner -> Executor -> Scoring -> Retry/Mutation
GList* pipeline_smart_search(Buffer input, int max_depth, int beam_width);

// Free a list of candidates
void candidate_list_free(GList *list);
void candidate_free(Candidate *c);

#ifdef __cplusplus
}
#endif
#endif // PIPELINE_H
