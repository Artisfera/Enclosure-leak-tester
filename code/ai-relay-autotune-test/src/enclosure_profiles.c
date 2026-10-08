#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kvss/nvs.h>
#include <zephyr/storage/flash_map.h>

#include "enclosure_profiles.h"

#define PROFILE_MAGIC 0x454c5431u
#define PROFILE_PARTITION storage_partition

typedef struct {
    uint32_t magic;
    char name[ENCLOSURE_PROFILE_NAME_LEN];
    pidParm pid;
    float output_start;
} saved_profile;

static struct nvs_fs profiles_fs;

int enclosure_profiles_init(void)
{
    struct flash_pages_info page;
    int err;

    profiles_fs.flash_device = PARTITION_DEVICE(PROFILE_PARTITION);
    if (!device_is_ready(profiles_fs.flash_device)) {
        return -ENODEV;
    }

    profiles_fs.offset = PARTITION_OFFSET(PROFILE_PARTITION);
    err = flash_get_page_info_by_offs(profiles_fs.flash_device,
                                      profiles_fs.offset, &page);
    if (err != 0) {
        return err;
    }

    profiles_fs.sector_size = page.size;
    profiles_fs.sector_count = 3U;
    return nvs_mount(&profiles_fs);
}

int enclosure_profile_load(unsigned int index, enclosure_profile *profile)
{
    saved_profile saved;
    ssize_t size;

    if (index >= ENCLOSURE_PROFILE_COUNT) {
        return -EINVAL;
    }

    memset(profile, 0, sizeof(*profile));
    size = nvs_read(&profiles_fs, index + 1U, &saved, sizeof(saved));
    if (size != sizeof(saved) || saved.magic != PROFILE_MAGIC) {
        return -ENOENT;
    }

    memcpy(profile->name, saved.name, sizeof(profile->name));
    profile->name[sizeof(profile->name) - 1] = '\0';
    profile->pid = saved.pid;
    profile->output_start = saved.output_start;
    profile->valid = 1;
    return 0;
}

int enclosure_profile_save(unsigned int index, const char *name,
                           pidParm pid, float output_start)
{
    saved_profile saved = {
        .magic = PROFILE_MAGIC,
        .pid = pid,
        .output_start = output_start,
    };
    ssize_t size;

    if (index >= ENCLOSURE_PROFILE_COUNT || name[0] == '\0') {
        return -EINVAL;
    }

    strncpy(saved.name, name, sizeof(saved.name) - 1);
    size = nvs_write(&profiles_fs, index + 1U, &saved, sizeof(saved));
    return size < 0 ? (int)size : 0;
}
