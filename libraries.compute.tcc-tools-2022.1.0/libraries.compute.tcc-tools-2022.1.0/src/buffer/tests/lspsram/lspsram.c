/*******************************************************************************
INTEL CONFIDENTIAL
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#include "memory_properties.h"
#include "stdio.h"
#include "tcc_config.h"
#include "tcc_config_finder.h"
#include "tcc_log.h"
#include "tcc_region_manager.h"

#define print_error(message, ...) printf("  \"error\":true,\n\"error_messgae\":\"" message "\"\n", ##__VA_ARGS__)

int main()
{
    printf("{\n");
    region_manager_t* manager = region_manager_create(tcc_config_read(get_available_config_file()));
    if (manager == NULL) {
        print_error("Can't create region manager");
        goto error;
    }
    ssize_t regions = region_manager_count(manager);
    if (regions < 0) {
        print_error("Can't create region manager");
        goto error;
    }

    printf("  \"error\":false,\n  \"data\":\n  [\n");
    for (ssize_t i = 0; i < regions; i++) {
        printf("   { \"name\":\"#%zu\",", i);
        const memory_properties_t* prop = region_manager_get(manager, i);
        if (prop == NULL) {
            print_error("Can't create region manager for index %zu", i);
            continue;
        }
        printf("\"error\":false,%s", memory_properties_to_str(prop));
        printf("}%s\n", i != regions - 1 ? "," : "");
    }
    printf("  ]\n");
    printf("}\n");
    return 0;
error:
    printf("}\n");
    return 1;
}
