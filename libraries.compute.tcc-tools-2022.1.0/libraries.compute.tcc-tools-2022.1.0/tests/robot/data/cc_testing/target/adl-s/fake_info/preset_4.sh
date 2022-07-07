#!/bin/sh
# /*******************************************************************************
# INTEL CONFIDENTIAL
# Copyright Intel Corporation.
# This software and the related documents are Intel copyrighted materials, and your use of them
# is governed by the express license under which they were provided to you (License).
# Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
# or transmit this software or the related documents without Intel's prior written permission.
# This software and the related documents are provided as is, with no express or implied warranties,
# other than those that are expressly stated in the License.
#
# *******************************************************************************/

cat <<- text
{
    "nominal_tsc_freq": 2112000000.0,
    "is_vmm": false,
    "rtct": [
        {
            "RTCT_Version_Major": 2,
            "RTCT_Version_Minor": 0,
            "RTCD_Version_Major": 2,
            "RTCD_Version_Minor": 0,
            "Type": "common.rtct.rtct_common.Compatibility"
        },
        {
            "TOTAL_IA_L2_CLOS": 8,
            "TOTAL_IA_L3_CLOS": 16,
            "TOTAL_L2_INSTANCES": 10,
            "TOTAL_L3_INSTANCES": 1,
            "TOTAL_GT_CLOS": 4,
            "TOTAL_WRC_CLOS": 1,
            "MAX_TCC_STREAMS": 8,
            "MAX_TCC_REGISTERS": 550,
            "Type": "common.rtct.v2.rtct.RTCD_Limits"
        },
        {
            "Address": 579342336,
            "Size": 20512,
            "Type": "common.rtct.v2.rtct.CRL_Binary"
        },
        {
            "Level": 2,
            "Cache_ID": 0,
            "WayMask": [
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 1,
            "WayMask": [
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 2,
            "WayMask": [
                511,
                511,
                511,
                511,
                511,
                511,
                511,
                511
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 3,
            "WayMask": [
                511,
                511,
                511,
                511,
                511,
                511,
                511,
                511
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 4,
            "WayMask": [
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 5,
            "WayMask": [
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 6,
            "WayMask": [
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 7,
            "WayMask": [
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023,
                1023
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 2,
            "Cache_ID": 9,
            "WayMask": [
                65535,
                65535,
                65535,
                65535,
                65535,
                65535,
                65535,
                65535
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 3,
            "Cache_ID": 0,
            "WayMask": [
                496,
                3,
                12,
                496,
                496,
                496,
                496,
                496,
                496,
                496,
                496,
                496,
                496,
                496,
                496,
                496
            ],
            "Type": "common.rtct.v2.rtct.IA_WayMasks"
        },
        {
            "Level": 3,
            "Cache_ID": 0,
            "WayMask": 2,
            "Type": "common.rtct.v2.rtct.WRC_WayMasks"
        },
        {
            "Level": 3,
            "Cache_ID": 0,
            "WayMask": [
                508,
                508,
                508,
                508
            ],
            "Type": "common.rtct.v2.rtct.GT_WayMasks"
        },
         {
            "Level": 2,
            "Cache_ID": 2,
            "WayMask": 512,
            "Type": "common.rtct.v2.rtct.SSRAM_WayMask"
        },
        {
            "Level": 2,
            "Cache_ID": 3,
            "WayMask": 512,
            "Type": "common.rtct.v2.rtct.SSRAM_WayMask"
        },
        {
            "Level": 3,
            "Cache_ID": 0,
            "WayMask": 512,
            "Type": "common.rtct.v2.rtct.SSRAM_WayMask"
        },
        {
            "Level": 2,
            "Cache_ID": 2,
            "Base": 578289536,
            "Size": 131072,
            "Shared": 0,
            "Type": "common.rtct.v2.rtct.SoftwareSRAM"
        },
        {
            "Level": 2,
            "Cache_ID": 3,
            "Base": 578289536,
            "Size": 131072,
            "Shared": 0,
            "Type": "common.rtct.v2.rtct.SoftwareSRAM"
        },
        {
            "Level": 3,
            "Cache_ID": 0,
            "Base": 578289536,
            "Size": 0,
            "Shared": 0,
            "Type": "common.rtct.v2.rtct.SoftwareSRAM"
        },
        {
            "Hierarchy": 2,
            "Clock_Cycles": 17,
            "Cache_ID": [
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                9
            ],
            "Type": "common.rtct.v2.rtct.MemoryHierarchyLatency"
        },
        {
            "Hierarchy": 3,
            "Clock_Cycles": 139,
            "Cache_ID": [
                0
            ],
            "Type": "common.rtct.v2.rtct.MemoryHierarchyLatency"
        },
        {
            "Hierarchy": 256,
            "Clock_Cycles": 321,
            "Cache_ID": [
                0
            ],
            "Type": "common.rtct.v2.rtct.MemoryHierarchyLatency"
        },
        {
            "Address": 579375104,
            "Size": 488,
            "Type": "common.rtct.v2.rtct.ErrorLogAddress"
        }
    ],
    "cache_info": {
        "levels": {
            "2": {
                "0": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        0
                    ]
                },
                "1": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        1
                    ]
                },
                "2": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        2
                    ]
                },
                "3": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        3
                    ]
                },
                "4": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        4
                    ]
                },
                "5": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        5
                    ]
                },
                "6": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        6
                    ]
                },
                "7": {
                    "size": 1310720,
                    "ways_count": 10,
                    "cpuid": [
                        7
                    ]
                },
                "9": {
                    "size": 2097152,
                    "ways_count": 16,
                    "cpuid": [
                        8,
                        9,
                        10,
                        11
                    ]
                }
            },
            "3": {
                "0": {
                    "size": 26214400,
                    "ways_count": 10,
                    "cpuid": [
                        0,
                        1,
                        2,
                        3,
                        4,
                        5,
                        6,
                        7,
                        8,
                        9,
                        10,
                        11
                    ]
                }
            }
        },
        "isinclusive": false
    },
    "cpu_info": {
        "model_name": "12th Gen Intel(R) Core(TM) i7-12700E\u0000\u0000\u0000",
        "cpuid_apicid": {
            "0": 0,
            "1": 8,
            "2": 16,
            "3": 24,
            "4": 32,
            "5": 40,
            "6": 48,
            "7": 56,
            "8": 72,
            "9": 74,
            "10": 76,
            "11": 78
        }
    }
}
text
