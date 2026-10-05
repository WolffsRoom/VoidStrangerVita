#ifndef MISSING_DATA_SCENE_H
#define MISSING_DATA_SCENE_H

#include <stdbool.h>
#include <stdint.h>
#include "missing_data_scene_policy.h"

#define MISSING_DATA_PATH_MAX 512
#define MISSING_DATA_REQUIRED_COUNT 4

typedef struct MissingDataInspection {
    uint32_t rootMissingMask;
    uint32_t misplacedMask;
    uint32_t duplicateMask;
    char misplacedPaths[MISSING_DATA_REQUIRED_COUNT][MISSING_DATA_PATH_MAX];
} MissingDataInspection;

uint32_t MissingDataScene_requiredMask(void);
MissingDataLayoutState MissingDataScene_inspect(MissingDataInspection* inspection);
bool MissingDataScene_fixOrganization(const MissingDataInspection* inspection);
/* 1 = continue boot, 0 = user requested exit, -1 = scene/assets failed. */
int MissingDataScene_run(uint32_t initialMask);
int MissingDataScene_runOrganization(const MissingDataInspection* inspection);

#endif
