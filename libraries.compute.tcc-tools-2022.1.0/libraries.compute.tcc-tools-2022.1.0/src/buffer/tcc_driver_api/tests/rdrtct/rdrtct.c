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

#include "tcc_driver_api.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    void* rtct = NULL;
    size_t size = 0;
    if (tcc_driver_read_rtct(&rtct, &size) < 0) {
        fprintf(stderr, "Can't read RTCT from tcc_driver, error:%s(%i)\n", strerror(errno), errno);
        goto error;
    }
    size_t written_bytes = fwrite(rtct, 1, size, stdout);
    if (written_bytes != size) {
        fprintf(stderr,
            "Can't write all RTCT table to stdout(%zu/%zu), error:%s(%i)\n",
            written_bytes,
            size,
            strerror(errno),
            errno);
        goto error;
    }
    return 0;
error:
    free(rtct);
    return -1;
}