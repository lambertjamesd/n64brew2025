#ifndef __CUTSCENE_CUTSCENE_REFERENCE_H__
#define __CUTSCENE_CUTSCENE_REFERENCE_H__

#include "cutscene.h"

enum cutscene_ref_type {
    CUTSCENE_REF_NONE,
    CUTSCENE_REF_DIRECT,
    CUTSCENE_REF_SCENE,
};

union cutscene_ref_data {
    struct {
        cutscene_t* cutscene;
    } direct;
    struct {
        char* name;
    } scene;
};

struct cutscene_ref {
    enum cutscene_ref_type type;
    union cutscene_ref_data data;
};

typedef struct cutscene_ref cutscene_ref_t;

void cutscene_ref_init_null(cutscene_ref_t* ref);
void cutscene_ref_init(cutscene_ref_t* ref, const char* name);
void cutscene_ref_destroy(cutscene_ref_t* ref);

void cutscene_ref_run(cutscene_ref_t* ref, entity_id subject);

#endif