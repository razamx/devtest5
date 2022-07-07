/*******************************************************************************
Copyright Intel Corporation.
This software and the related documents are Intel copyrighted materials, and your use of them
is governed by the express license under which they were provided to you (License).
Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
or transmit this software or the related documents without Intel's prior written permission.
This software and the related documents are provided as is, with no express or implied warranties,
other than those that are expressly stated in the License.

*******************************************************************************/

#ifndef _TCC_FAKE_TCC_BUFFER_H_
#define _TCC_FAKE_TCC_BUFFER_H_

#include <linux/ioctl.h>
#include <stdlib.h>

/*
** Present /dev/tcc_buffer in user space;
** Will administer buffer allocation.
*/
#define TCC_BUFFER_NAME "/dev/tcc/tcc_buffer"
#define UNDEFINED_DEVNODE 0x55555555
/*
** IOCTL MAGIC number
*/
#define IOCTL_TCC_MAGIC 'T'

enum tcc_buf_region_type
{
    RGN_UNKNOWN = 0,
    RGN_L1,
    RGN_L2,
    RGN_L3,
    RGN_EDRAM,
    RGN_MALLOC  // DRAM
};

/*
** IN:
** id: pseudo-SRAM region id from which user request for attribute.
** OUT:
** latency: delay in cpu clocks
** type: the type of the memory pSRAM region
** size: total size in bytes
** ways: the cache ways used to create the pSRAM region.
** cpu_mask_p: affinity bitmask of the logical cores available for access to the pSRAM region
*/
struct tcc_buf_mem_config_s
{
    unsigned int id;
    unsigned int latency;
    size_t size;
    enum tcc_buf_region_type type;
    unsigned int ways;
    void* cpu_mask_p;
};

/*
** IN:
** id: pseudo-SRAM region id, from which user rquest for buffer
** size: buffer size (bytes).
** OUT:
** devnode: driver returns device node to user
*/
struct tcc_buf_mem_req_s
{
    unsigned int id;
    size_t size;
    unsigned int devnode;
};

enum ioctl_index
{
    IOCTL_TCC_GET_REGION_COUNT = 1,
    IOCTL_TCC_GET_MEMORY_CONFIG,
    IOCTL_TCC_REQ_BUFFER,
    IOCTL_TCC_QUERY_RTCT_SIZE,
    IOCTL_TCC_GET_RTCT,
};

/*
** User to get pseudo-SRAM region counts
*/
#define TCC_GET_REGION_COUNT _IOR(IOCTL_TCC_MAGIC, IOCTL_TCC_GET_REGION_COUNT, unsigned int*)

/*
** For regions with psram mem_type, user library asks for memory config
*/
#define TCC_GET_MEMORY_CONFIG _IOWR(IOCTL_TCC_MAGIC, IOCTL_TCC_GET_MEMORY_CONFIG, struct tcc_buf_mem_config_s*)

/*
** User to get pseudo-SRAM region counts
*/
#define TCC_QUERY_RTCT_SIZE _IOR(IOCTL_TCC_MAGIC, IOCTL_TCC_QUERY_RTCT_SIZE, unsigned int*)

/*
** User to get pseudo-SRAM region counts
*/
#define TCC_GET_RTCT _IOR(IOCTL_TCC_MAGIC, IOCTL_TCC_GET_RTCT, unsigned int*)

/*
** User request tcc buffer; obtain device node
*/
#define TCC_REQ_BUFFER _IOWR(IOCTL_TCC_MAGIC, IOCTL_TCC_REQ_BUFFER, struct tcc_buf_mem_req_s*)

#endif  // _TCC_FAKE_TCC_BUFFER_H_
