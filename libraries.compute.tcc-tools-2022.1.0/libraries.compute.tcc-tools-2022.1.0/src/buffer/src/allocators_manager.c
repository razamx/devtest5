/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#define _GNU_SOURCE
#include "allocators_manager.h"
#include "allocator_interface.h"
#include "dram_allocator.h"
#include "memory_properties.h"
#include "region_allocator.h"
#include "tcc_log.h"
#include "tcc_region_manager.h"
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

#define UNUSED(var) ((void)(var))
#define DEBUG_STRING_SIZE (256)
#define FORLOGONLY(var) UNUSED(var)

// allocators iterator
struct allocator_it
{
    size_t region_index;
    allocator_interface_t* allocator;
};

// Helper structure with information about allocator
// Stores memory boundaries of the allocator
typedef struct
{
    allocator_interface_t* allocator;
    enum tcc_memory_level type;
    void* start_ptr;
    void* end_ptr;
} allocator_holder_t;

// instance
struct allocators_manager
{
    region_manager_t* region_manager;

    // dedicated allocator for DRAM
    allocator_interface_t* dram_allocator;

    // statically allocated iterator
    allocator_it_t it;

    size_t regions_count;

    // map: region_index -> allocator
    allocator_holder_t* allocators;

    // map:  region_index -> size to mmap from region_manager
    size_t* required_buffer_size;
};

static allocators_manager_t* _instance;

static int create_region_size_map(allocators_manager_t* self, const tcc_config_t* config);
static const tcc_config_item_t* find_config_for_region(const tcc_config_t* config,
    const memory_properties_t* properties);
static allocator_it_t* init_iterator(const allocators_manager_t* self, const allocator_it_t* it);
static int create_allocator(const allocators_manager_t* self, size_t region_index, enum tcc_memory_level type);
static int create_allocator_dram(const allocators_manager_t* self, allocator_holder_t* holder);
static int create_allocator_region(const allocators_manager_t* self, allocator_holder_t* holder, size_t region_index);
static int is_satisfy_requirements(const memory_properties_t* properties, const memory_properties_t* requirements);
static int is_satisfy_mask(const cpu_set_t* mask, const cpu_set_t* reqs);
allocator_interface_t* find_allocator_by_ptr(const allocators_manager_t* self, const void* ptr);
int is_ptr_in_allocator(allocator_holder_t* holder, const void* ptr);
static void destroy_allocator(allocator_holder_t* holder);
static void print_regions_table(const allocators_manager_t* self);

////////////////////////////////////////////////////////////////////////////////

allocators_manager_t* allocators_manager_create(region_manager_t* region_manager, const tcc_config_t* config)
{
    TCC_TRACE_FUNC("region_manager=%p config=%p", region_manager, config);

    allocators_manager_t* self = NULL;
    ssize_t regions_count = -1;

    if (region_manager == NULL) {
        TCC_LOG_ERROR("Parameter \"region_manager\" is NULL");
        goto err;
    }

    if (_instance != NULL) {
        TCC_LOG_ERROR("Allocators Manager has been already created");
        goto err;
    }

    self = (allocators_manager_t*)calloc(1, sizeof(allocators_manager_t));
    if (self == NULL) {
        TCC_LOG_ERROR("Unable to allocate memory for instance: %s (%d)", strerror(errno), errno);
        goto err;
    }

    regions_count = region_manager_count(region_manager);
    if (regions_count < 0) {
        TCC_LOG_ERROR("Unable to get regions count: %s (%d)", strerror(errno), errno);
        goto err;
    }

    self->region_manager = region_manager;
    self->regions_count = (size_t)regions_count;


    self->allocators = (allocator_holder_t*)calloc(self->regions_count, sizeof(allocator_holder_t));
    if (self->allocators == NULL) {
        TCC_LOG_ERROR("Unable to allocate memory for allocators list: %s (%d)", strerror(errno), errno);
        goto err;
    }

    self->required_buffer_size = (size_t*)calloc(self->regions_count, sizeof(size_t));
    if (self->required_buffer_size == NULL) {
        TCC_LOG_ERROR("Unable to allocate memory for required buffer size list: %s (%d)", strerror(errno), errno);
        goto err;
    }

    if (create_region_size_map(self, config) < 0) {
        TCC_LOG_ERROR("Unable to initialize buffers size map");
        goto err;
    }

    self->dram_allocator = dram_allocator_create();
    if (self->dram_allocator == NULL) {
        TCC_LOG_ERROR("Unable to create DRAM allocator: %s (%d)", strerror(errno), errno);
        goto err;
    }

    _instance = self;
    print_regions_table(self);
    return self;
err:
    allocators_manager_destroy(self);
    return NULL;
}

void allocators_manager_destroy(allocators_manager_t* self)
{
    TCC_TRACE_FUNC("self=%p", self);

    if (self == NULL) {
        TCC_LOG_ERROR("Parameter \"self\" is NULL");
        return;
    }

    /* Delete self->dram_allocator separately, because DRAM items in the
     * self->allocators just points to the self->dram_allocator
     */
    if (self->dram_allocator != NULL) {
        self->dram_allocator->destroy(self->dram_allocator);
    }

    if (self->allocators != NULL) {
        for (size_t i = 0; i < self->regions_count; i++) {
            destroy_allocator(&self->allocators[i]);
        }
        free(self->allocators);
        self->allocators = NULL;
    }

    if (self->required_buffer_size != NULL) {
        free(self->required_buffer_size);
        self->required_buffer_size = NULL;
    }

    free(self);
    _instance = NULL;
}


/* Iterate over all possible allocators that satisfy requirements
 *
 * Algorithm:
 *  - get another suitable region
 *  - return allocator, attached to that region, or create new one
 */
const allocator_it_t* allocators_manager_get_next(allocators_manager_t* self,
    memory_properties_t* requirements,
    const allocator_it_t* it)
{
    TCC_TRACE_FUNC("self=%p requirements=%p it=%p", self, requirements, it);
    if (self == NULL || requirements == NULL) {
        TCC_LOG_ERROR("One or more parameters are NULL: self=%p, requirements=%p", self, requirements);
        return NULL;
    }
    TCC_LOG_INFO("Searching for memory to satisfy requirements: latency=%u ns, affinity=%s",
        requirements->latency,
        cpu_set_to_str(&requirements->mask));

    allocator_it_t* a_it = init_iterator(self, it);

    for (; a_it->region_index < self->regions_count; a_it->region_index++) {
        const memory_properties_t* reg_properties = region_manager_get(self->region_manager, a_it->region_index);
        const char* region_level = reg_properties ? tcc_memory_level_to_str(reg_properties->type) : "(null)";

        FORLOGONLY(region_level);

        if (reg_properties == NULL) {
            TCC_LOG_ERROR("Unable to get region #%lu: %s (%d)", a_it->region_index, strerror(errno), errno);
            goto exit;
        } else {
            TCC_LOG_DEBUG("Get region #%lu (%s)", a_it->region_index, region_level);
        }

        if (!is_satisfy_requirements(reg_properties, requirements)) {
            TCC_LOG_DEBUG("Region #%lu (%s) doesn't satisfy requirements", a_it->region_index, region_level);
            continue;
        }
        TCC_LOG_DEBUG("Region #%lu (%s) satisfies requirements", a_it->region_index, region_level);

        allocator_holder_t* alloc_holder = &self->allocators[a_it->region_index];
        if (alloc_holder->allocator == NULL) {
            TCC_LOG_DEBUG("Region #%lu (%s) has no allocator, create new one", a_it->region_index, region_level);
            if (create_allocator(self, a_it->region_index, reg_properties->type) != 0) {
                TCC_LOG_WARNING("Unable to create allocator for memory type %d", reg_properties->type);
                continue;
            }
            TCC_LOG_DEBUG("Successfully created allocator for region #%lu (%s)", a_it->region_index, region_level);
        }
        TCC_LOG_INFO("Found suitable allocator for request: region #%lu (%s)", a_it->region_index, region_level);
        a_it->region_index++;
        a_it->allocator = alloc_holder->allocator;
        return a_it;
    }

exit:
    TCC_LOG_WARNING("No suitable memory found to satisfy requirements");
    return NULL;
}

allocator_interface_t* allocators_manager_get_alloc_by_ptr(allocators_manager_t* self, const void* ptr)
{
    TCC_TRACE_FUNC("self=%p ptr=%p", self, ptr);

    if (self == NULL || ptr == NULL) {
        TCC_LOG_ERROR("One or more arguments are NULL: self=%p, ptr=%p", self, ptr);
        return NULL;
    }

    allocator_interface_t* allocator = find_allocator_by_ptr(self, ptr);
    if (allocator != NULL) {
        TCC_LOG_DEBUG("Found allocator for ptr %p: %p", ptr, allocator);
    } else {
        TCC_LOG_DEBUG("Can't found allocator for ptr %p, DRAM allocator used", ptr);
        allocator = self->dram_allocator;
    }

    return allocator;
}

allocator_interface_t* get_allocator_from_iterator(const allocator_it_t* it)
{
    TCC_TRACE_FUNC("it=%p", it);

    if (it == NULL) {
        TCC_LOG_ERROR("Iterator is NULL");
        return NULL;
    }

    return it->allocator;
}

////////////////////////////////////////////////////////////////////////////////

// Create match between every region and requested size from config
static int create_region_size_map(allocators_manager_t* self, const tcc_config_t* config)
{
    TCC_TRACE_FUNC("self=%p config=%p", self, config);

    // Fill sizes from config
    for (size_t i = 0; i < self->regions_count; i++) {
        const memory_properties_t* region = region_manager_get(self->region_manager, i);
        if (region == NULL) {
            TCC_LOG_ERROR("Unable to get region #%lu: %s (%d)", i, strerror(errno), errno);
            return -1;
        }

        const tcc_config_item_t* cfg_item = config != NULL ? find_config_for_region(config, region) : NULL;
        if (cfg_item != NULL) {
            if (cfg_item->size > 0 && cfg_item->size <= region->size) {
                self->required_buffer_size[i] = cfg_item->size;
            } else {
                TCC_LOG_WARNING(
                    "Buffer size for region #%lu (%s) in config is abnormal, using entire region size from driver",
                    i,
                    tcc_memory_level_to_str(region->type));
                self->required_buffer_size[i] = region->size;
            }
        } else {
            TCC_LOG_WARNING("There is no configuration for region #%lu (%s), using entire region size",
                i,
                tcc_memory_level_to_str(region->type));
            self->required_buffer_size[i] = region->size;
        }
        TCC_LOG_DEBUG("Allocation size for region #%zu (%s) = %lu",
            i,
            tcc_memory_level_to_str(region->type),
            self->required_buffer_size[i]);
    }

    return 0;
}

// Find config entry for region by memory type and affinity mask
static const tcc_config_item_t* find_config_for_region(const tcc_config_t* config,
    const memory_properties_t* properties)
{
    TCC_TRACE_FUNC("config=%p properties=%p", config, properties);
    TCC_LOG_TRACE("Properties: %s", memory_properties_to_str(properties));

    size_t size = (size_t)tcc_config_count(config);
    for (size_t i = 0; i < size; i++) {
        const tcc_config_item_t* cfg_item = tcc_config_get(config, i);
        if (cfg_item != NULL) {
            int is_equal = cfg_item->level == properties->type;
            if (is_equal) {
                return cfg_item;
            }
        }
    }
    return NULL;
}


// Initialize new iterator if it isn't initialized or transparently return existing
static allocator_it_t* init_iterator(const allocators_manager_t* self, const allocator_it_t* it)
{
    TCC_TRACE_FUNC("self=%p it=%p", self, it);

    if (it == NULL) {
        TCC_LOG_TRACE("Iterator is NULL, start from the begining");
        // Use single static instance of the iterator
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
        it = &self->it;
        memset((void*)it, 0, sizeof(allocator_it_t));
    } else {
        TCC_LOG_TRACE("it != NULL, continue");
    }

    return (allocator_it_t*)it;
#pragma GCC diagnostic pop
}

static int create_allocator(const allocators_manager_t* self, size_t region_index, enum tcc_memory_level type)
{
    TCC_TRACE_FUNC("self=%p region_index=%lu type=%s", self, region_index, tcc_memory_level_to_str(type));

    if (type <= TCC_MEM_UNKNOWN || type > TCC_MEM_DRAM) {
        TCC_LOG_WARNING("Region #%lu has invalid memory type %d", region_index, type);
        TCC_LOG_ERROR("Real-time Configuration Driver return a wrong type enum");
        return -1;
    }

    allocator_holder_t* alloc_holder = &self->allocators[region_index];
    alloc_holder->type = type;
    if (type == TCC_MEM_DRAM) {
        return create_allocator_dram(self, alloc_holder);
    }
    return create_allocator_region(self, alloc_holder, region_index);
}

static int create_allocator_dram(const allocators_manager_t* self, allocator_holder_t* holder)
{
    TCC_TRACE_FUNC("self=%p, holder=%p", self, holder);

    // Using existing DRAM allocator
    holder->allocator = self->dram_allocator;
    holder->start_ptr = 0;
    holder->end_ptr = 0;
    return 0;
}

static int create_allocator_region(const allocators_manager_t* self, allocator_holder_t* holder, size_t region_index)
{
    TCC_TRACE_FUNC("self=%p holder=%p region_index=%lu", self, holder, region_index);

    size_t size = self->required_buffer_size[region_index];
    void* memory = region_manager_mmap(self->region_manager, region_index, size);
    if (memory == NULL) {
        TCC_LOG_WARNING("Unable to get memory for request region_index=%lu, size=%lu: %s (%d)",
            region_index,
            size,
            strerror(errno),
            errno);
        return -1;
    }

    holder->allocator = region_allocator_create(memory, size);
    if (holder->allocator == NULL) {
        TCC_LOG_WARNING("Unable to create allocator for region #%lu: %s (%d)", region_index, strerror(errno), errno);
        return -1;
    }

    holder->start_ptr = memory;
    holder->end_ptr = (uint8_t*)memory + size;

    return 0;
}

// Check that memory properties requirements satisfy given requirements
static int is_satisfy_requirements(const memory_properties_t* properties, const memory_properties_t* requirements)
{
    TCC_TRACE_FUNC("properties=%p requirements=%p", properties, requirements);

    TCC_LOG_DEBUG("Required latency: %u", requirements->latency);
    TCC_LOG_DEBUG("Region's latency: %u", properties->latency);
    TCC_LOG_DEBUG("Required mask %s", cpu_set_to_str(&requirements->mask));
    TCC_LOG_DEBUG("Region's mask %s", cpu_set_to_str(&properties->mask));

    return properties->latency <= requirements->latency && is_satisfy_mask(&properties->mask, &requirements->mask);
}

// Check that "mask" bitmask contains or equal "reqs" bitmask
static int is_satisfy_mask(const cpu_set_t* mask, const cpu_set_t* reqs)
{
    TCC_TRACE_FUNC("mask=%p reqs=%p", mask, reqs);

    cpu_set_t combined;
    CPU_AND(&combined, reqs, mask);
    int reqs_cnt = CPU_COUNT(reqs);
    int combined_cnt = CPU_COUNT(&combined);
    return combined_cnt == reqs_cnt;
}


allocator_interface_t* find_allocator_by_ptr(const allocators_manager_t* self, const void* ptr)
{
    TCC_TRACE_FUNC("self=%p ptr=%p", self, ptr);

    for (size_t i = 0; i < self->regions_count; i++) {
        if (is_ptr_in_allocator(&self->allocators[i], ptr)) {
            return self->allocators[i].allocator;
        }
    }
    return NULL;
}

int is_ptr_in_allocator(allocator_holder_t* holder, const void* ptr)
{
    TCC_TRACE_FUNC("allocator={start=%p, end=%p}, ptr=%p", holder->start_ptr, holder->end_ptr, ptr);

    return ptr >= holder->start_ptr && ptr < holder->end_ptr;
}

static void destroy_allocator(allocator_holder_t* holder)
{
    TCC_TRACE_FUNC("holder=%p allocator=%p", holder, holder->allocator);

    if (holder->allocator != NULL && holder->type != TCC_MEM_DRAM) {
        holder->allocator->destroy(holder->allocator);
        holder->allocator = NULL;
    }
}

static void print_regions_table(const allocators_manager_t* self)
{
    char region_size_str[DEBUG_STRING_SIZE] = "Regular RAM";
    char available_size_str[DEBUG_STRING_SIZE] = "Regular RAM";

    TCC_LOG_INFO("Available regions:");
    TCC_LOG_INFO("Id          Cache level      Latency, ns  Affinity mask     "
                 "Entire region size, b  Allocation size from tcc.config, b");
    for (size_t index = 0; index < self->regions_count; index++) {
        const memory_properties_t* region = region_manager_get(self->region_manager, index);

        if (region == NULL) {
            TCC_LOG_ERROR("Unable to get region #%lu: %s (%d)", index, strerror(errno), errno);
            return;
        }

        size_t allocation_size = self->required_buffer_size[index];
        const char* region_level = tcc_memory_level_to_str(region->type);

        if (region->type != TCC_MEM_DRAM) {
            snprintf(region_size_str, DEBUG_STRING_SIZE, "%lu", region->size);
            snprintf(available_size_str, DEBUG_STRING_SIZE, "%lu", allocation_size);
        }

        TCC_LOG_INFO("%-10u  %-15s  %-11u  %-16s  %-21s  %s",
            region->id,
            region_level,
            region->latency,
            cpu_set_to_str(&region->mask),
            region_size_str,
            available_size_str);
    }
}
