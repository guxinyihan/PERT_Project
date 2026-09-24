#include "input.h"
#include "model.h"
#include "graph.h"
#include "pert.h"
#include "probability.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#ifdef TRACK_MEMORY
#include "memory_probe.h"
#endif

static size_t checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
#define NEAR(a,b) CHECK(fabs((a)-(b)) <= 1e-8)
static const char *header = "ID,Description,a,m,b,Predecessors\n";
static PertError error;
static void write_case(const char *body) {
    FILE *f = fopen("tests/generated_case.csv", "wb"); CHECK(f != NULL);
    CHECK(fputs(header, f) >= 0); CHECK(fputs(body, f) >= 0); CHECK(fclose(f) == 0);
}
static void load_case(const char *body, Project *p, PERTResult *r) {
    write_case(body); project_init(p); pert_result_init(r);
    CHECK(input_csv("tests/generated_case.csv", p, &error) == PERT_OK);
    CHECK(pert_calculate(p, r, &error) == PERT_OK);
}
static void cleanup(Project *p, PERTResult *r) { pert_result_destroy(r); project_destroy(p); }
typedef struct {
    const Project *p; const PERTResult *r;
    size_t count, expected_length;
    bool lecture;
} Capture;
static PertStatus capture(const size_t *path, size_t length, void *context, PertError *e) {
    Capture *c = context; ++c->count;
    CHECK(length > 0 && length <= c->p->count);
    if (c->expected_length) CHECK(length == c->expected_length);
    CHECK(c->p->predecessors[path[0]].count == 0);
    CHECK(c->p->successors[path[length - 1]].count == 0);
    for (size_t i = 1; i < length; ++i) CHECK(pert_critical_edge(c->p, c->r, path[i - 1], path[i]));
    ProbabilityResult q;
    CHECK(probability_calculate(c->r, path, length, c->r->duration + 3, &q, e) == PERT_OK);
    NEAR(q.duration, c->r->duration);
    if (c->lecture) {
        const char *ids[] = {"A","B","C","E","F","J","L","N"};
        CHECK(length == 8);
        for (size_t i = 0; i < length; ++i) CHECK(!strcmp(c->p->activities[path[i]].id, ids[i]));
        NEAR(q.variance, 9); NEAR(q.standard_deviation, 3); NEAR(q.z, 1);
        NEAR(q.probability, 0.8413447460685429);
    }
    return PERT_OK;
}
static void paths(Project *p, PERTResult *r, size_t expected, bool truncated, size_t length, bool lecture) {
    Capture c = {p, r, 0, length, lecture}; PathSummary summary;
    CHECK(pert_enumerate_paths(p, r, capture, &c, &summary, &error) == PERT_OK);
    CHECK(c.count == expected); CHECK(summary.emitted == expected); CHECK(summary.truncated == truncated);
}
static void lecture_test(void) {
    Project p; project_init(&p); PERTResult r; pert_result_init(&r);
    CHECK(input_csv("data/lecture_example.csv", &p, &error) == PERT_OK);
    CHECK(pert_calculate(&p, &r, &error) == PERT_OK);
    CHECK(p.count == 14 && p.edge_count == 16); NEAR(r.duration, 44);
    const double rows[14][7] = {
        {2,1.0/9,0,2,0,2,0},{4,1,2,6,2,6,0},{10,4,6,16,6,16,0},
        {6,1,16,22,20,26,4},{4,4.0/9,16,20,16,20,0},{5,1,20,25,20,25,0},
        {7,1,22,29,26,33,4},{9,4,29,38,33,42,4},{7,1,16,23,18,25,2},
        {8,1,25,33,25,33,0},{4,0,33,37,34,38,1},{5,1,33,38,33,38,0},
        {2,1.0/9,38,40,42,44,4},{6,4.0/9,38,44,38,44,0}
    };
    for (size_t i = 0; i < 14; ++i) {
        ActivityResult a = r.activities[i];
        const double actual[] = {a.expected,a.variance,a.es,a.ef,a.ls,a.lf,a.slack};
        for (size_t j = 0; j < 7; ++j) NEAR(actual[j], rows[i][j]);
        CHECK(a.critical == (rows[i][6] == 0));
    }
    CHECK(r.critical_count == 8); paths(&p, &r, 1, false, 8, true); cleanup(&p, &r);
    puts("PASS classroom: all 14 rows, path and probability");
}
static void topology_tests(void) {
    Project p; PERTResult r;
    load_case("A,Single,2,2,2,-\n", &p, &r); NEAR(r.duration,2); paths(&p,&r,1,false,1,false);
    size_t path[] = {0}; ProbabilityResult q;
    CHECK(probability_calculate(&r,path,1,2,&q,&error)==PERT_OK); CHECK(q.deterministic); NEAR(q.probability,1);
    CHECK(probability_calculate(&r,path,1,1.9999999999,&q,&error)==PERT_OK); NEAR(q.probability,0);
    CHECK(probability_calculate(&r,path,1,-1,&q,&error)==PERT_ERR_INPUT);
    CHECK(probability_calculate(&r,path,1,INFINITY,&q,&error)==PERT_ERR_INPUT); cleanup(&p,&r);
    load_case("C,Merge,1,1,1,A|B\nB,Start,3,3,3,-\nA,Start,2,2,2,-\n",&p,&r);
    NEAR(r.duration,4); NEAR(r.activities[2].slack,1); paths(&p,&r,1,false,2,false); cleanup(&p,&r);
    load_case("A,Start,1,1,1,-\nB,End,2,2,2,A\nC,End,3,3,3,A\n",&p,&r);
    NEAR(r.activities[1].slack,1); paths(&p,&r,1,false,2,false); cleanup(&p,&r);
    load_case("A,Component one,2,2,2,-\nB,Component two,5,5,5,-\n",&p,&r);
    NEAR(r.activities[0].slack,3); paths(&p,&r,1,false,1,false); cleanup(&p,&r);
    load_case("D,End,1,1,1,B|C\nA,Start,1,1,1,-\nB,Left,2,2,2,A\nC,Right,2,2,2,A\n",&p,&r);
    NEAR(r.duration,4); paths(&p,&r,2,false,3,false);
    size_t *order = NULL; CHECK(graph_topological(&p,&order,&error)==PERT_OK);
    size_t position[4]; for(size_t i=0;i<4;++i) position[order[i]]=i;
    for(size_t i=0;i<4;++i) for(size_t j=0;j<p.successors[i].count;++j)
        CHECK(position[i]<position[p.successors[i].indices[j]]);
    free(order); cleanup(&p,&r);
    load_case("A,Zero,0,0,0,-\nB,Zero,0,0,0,A\nC,Zero,0,0,0,A\nD,End,0,0,0,B|C\n",&p,&r);
    paths(&p,&r,2,false,3,false); cleanup(&p,&r);
    puts("PASS single/multiple starts/ends, disconnected, unsorted, multiple/zero-duration paths");
}
static void invalid(const char *body, PertStatus expected) {
    write_case(body); Project p; project_init(&p);
    CHECK(input_csv("tests/generated_case.csv",&p,&error)==expected);
    CHECK(p.count==0 && !p.activities); project_destroy(&p);
}
static void invalid_tests(void) {
    invalid("A,one,1,1,1,B\nB,two,1,1,1,A\n",PERT_ERR_CYCLE);
    invalid("A,one,1,1,1,-\nA,two,1,1,1,-\n",PERT_ERR_DUPLICATE); CHECK(error.line==3);
    invalid("A,one,1,1,1,-\nB,two,1,1,1,A|A\n",PERT_ERR_DUPLICATE); CHECK(error.line==3);
    invalid("A,one,1,1,1,Z\n",PERT_ERR_REFERENCE); CHECK(error.line==2); CHECK(!strcmp(error.activity_id,"A"));
    invalid("A,one,1,1,1,A\n",PERT_ERR_REFERENCE);
    invalid("A,one,-1,1,1,-\n",PERT_ERR_INPUT);
    invalid("A,one,2,1,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,NaN,1,2,-\n",PERT_ERR_INPUT);
    invalid("A,one,1,2,Inf,-\n",PERT_ERR_INPUT);
    invalid("A,one,1x,2,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,0x1p0,2,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,1,2,3\n",PERT_ERR_INPUT);
    invalid("A,one,1,2,3,-,extra\n",PERT_ERR_INPUT);
    invalid("A,,1,2,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,1,,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,1,2,3,\n",PERT_ERR_INPUT);
    invalid("1A,one,1,2,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,1,2,3,B|\nB,two,1,2,3,-\n",PERT_ERR_INPUT);
    invalid("A,one,0,1,1e309,-\n",PERT_ERR_NUMERIC);
    invalid("A,one,0,1,1e200,-\n",PERT_ERR_NUMERIC);
    invalid("A,one,0,0,1e-200,-\n",PERT_ERR_NUMERIC);
    invalid("",PERT_ERR_INPUT);
    Project p; project_init(&p); CHECK(input_csv("tests/does_not_exist.csv",&p,&error)==PERT_ERR_FILE);
    puts("PASS invalid graph, estimates, CSV fields, references and numeric range");
}
static void numeric_tests(void) {
    Project p; PERTResult r;
    load_case("A,Fraction,0.1,0.2,0.4,-\nB,Fraction,0,0.1,0.3,A\n",&p,&r);
    NEAR(r.duration,1.0/3.0); paths(&p,&r,1,false,2,false); cleanup(&p,&r);
    load_case("A,Long,1,1,1,-\nB,Short,0.99999998,0.99999998,0.99999998,-\n",&p,&r);
    CHECK(!r.activities[1].critical); CHECK(r.activities[1].slack>1e-9); cleanup(&p,&r);
    load_case("A,Long,1000000000000,1000000000000,1000000000000,-\nB,Short,999999999999,999999999999,999999999999,-\n",&p,&r);
    CHECK(!r.activities[1].critical); NEAR(r.activities[1].slack,1); cleanup(&p,&r);
    load_case("A,Tiny variance,1,1,1.00000001,-\n",&p,&r);
    size_t path[]={0}; ProbabilityResult q;
    CHECK(probability_calculate(&r,path,1,r.duration,&q,&error)==PERT_OK);
    CHECK(!q.deterministic); CHECK(q.variance>0); NEAR(q.probability,0.5); cleanup(&p,&r);
    write_case("A,Huge,1e308,1e308,1e308,-\nB,Huge,1e308,1e308,1e308,A\n");
    project_init(&p); pert_result_init(&r); CHECK(input_csv("tests/generated_case.csv",&p,&error)==PERT_OK);
    CHECK(pert_calculate(&p,&r,&error)==PERT_ERR_NUMERIC); CHECK(!r.activities); cleanup(&p,&r);
    puts("PASS fractional, tiny variance, small slack, large scale and forward overflow");
}
static void large_and_limit(void) {
    Project p; project_init(&p); PERTResult r; pert_result_init(&r);
    const size_t n=20000;
    for(size_t i=0;i<n;++i) { char id[33]; snprintf(id,sizeof id,"A%zu",i); CHECK(project_add(&p,id,"chain",1,1,1,&error)==PERT_OK); }
    CHECK(project_finalize(&p,&error)==PERT_OK);
    for(size_t i=1;i<n;++i) CHECK(graph_add_edge(&p,i-1,i,&error)==PERT_OK);
    CHECK(pert_calculate(&p,&r,&error)==PERT_OK); NEAR(r.duration,(double)n);
    paths(&p,&r,1,false,n,false); cleanup(&p,&r);
    project_init(&p); pert_result_init(&r);
    for(size_t i=0;i<16;++i) { char id[33]; snprintf(id,sizeof id,"L%zu",i); CHECK(project_add(&p,id,"layer",1,1,1,&error)==PERT_OK); }
    CHECK(project_finalize(&p,&error)==PERT_OK);
    for(size_t layer=0;layer<7;++layer) for(size_t a=0;a<2;++a) for(size_t b=0;b<2;++b)
        CHECK(graph_add_edge(&p,2*layer+a,2*(layer+1)+b,&error)==PERT_OK);
    CHECK(pert_calculate(&p,&r,&error)==PERT_OK); paths(&p,&r,100,true,8,false); cleanup(&p,&r);
    /* Boundary case: exactly 100 paths must NOT be marked truncated. */
    project_init(&p); pert_result_init(&r);
    for(size_t i=0;i<100;++i) { char id[33]; snprintf(id,sizeof id,"P%zu",i); CHECK(project_add(&p,id,"isolated",1,1,1,&error)==PERT_OK); }
    CHECK(project_finalize(&p,&error)==PERT_OK); CHECK(pert_calculate(&p,&r,&error)==PERT_OK);
    paths(&p,&r,100,false,1,false); cleanup(&p,&r);
    puts("PASS 20000-node chain, 256-path truncation and exact 100-path boundary");
}
static PertStatus cancel_callback(const size_t *p,size_t n,void *ctx,PertError *e) {
    (void)p; (void)n; (void)ctx;
    return pert_error(e,PERT_ERR_INTERNAL,0,NULL,"Intentional callback cancellation.");
}
static void callback_test(void) {
    Project p; PERTResult r; load_case("A,one,1,1,1,-\n",&p,&r); PathSummary s;
    CHECK(pert_enumerate_paths(&p,&r,cancel_callback,NULL,&s,&error)==PERT_ERR_INTERNAL);
    CHECK(s.emitted==0); cleanup(&p,&r); puts("PASS callback failure propagation");
}
#ifdef TRACK_MEMORY
static PertStatus no_output(const size_t *p,size_t n,void *ctx,PertError *e) {
    (void)p; (void)n; (void)ctx; (void)e; return PERT_OK;
}
static void allocation_failures(void) {
    size_t failure;
    for(failure=0;failure<10000;++failure) {
        probe_fail_after(failure);
        Project p; project_init(&p); PERTResult r; pert_result_init(&r); PathSummary s;
        PertStatus status=input_csv("data/lecture_example.csv",&p,&error);
        if(status==PERT_OK) status=pert_calculate(&p,&r,&error);
        if(status==PERT_OK) status=pert_enumerate_paths(&p,&r,no_output,NULL,&s,&error);
        cleanup(&p,&r); CHECK(probe_outstanding()==0);
        if(status==PERT_OK) break;
        CHECK(status==PERT_ERR_MEMORY);
    }
    CHECK(failure<10000); probe_fail_after(SIZE_MAX);
    printf("PASS allocation failure sweep: %zu failing allocation positions plus success\n",failure);
}
#endif
int main(void) {
    lecture_test(); topology_tests(); invalid_tests(); numeric_tests(); large_and_limit(); callback_test();
#ifdef TRACK_MEMORY
    CHECK(probe_outstanding()==0); allocation_failures(); CHECK(probe_outstanding()==0);
    puts("PASS allocation tracker: no live blocks, invalid frees or changed tail guards");
#endif
    CHECK(remove("tests/generated_case.csv")==0);
    printf("ALL CORE TESTS PASSED: %zu assertions\n",checks); return 0;
}
