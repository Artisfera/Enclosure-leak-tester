#ifndef ENCLOSURE_PROFILES_H_
#define ENCLOSURE_PROFILES_H_

#include "pid.h"

#define ENCLOSURE_PROFILE_COUNT 5
#define ENCLOSURE_PROFILE_NAME_LEN 24

typedef struct {
    char name[ENCLOSURE_PROFILE_NAME_LEN];
    pidParm pid;
    float output_start;
    int valid;
} enclosure_profile;

int enclosure_profiles_init(void);
int enclosure_profile_load(unsigned int index, enclosure_profile *profile);
int enclosure_profile_save(unsigned int index, const char *name,
                           pidParm pid, float output_start);

#endif
