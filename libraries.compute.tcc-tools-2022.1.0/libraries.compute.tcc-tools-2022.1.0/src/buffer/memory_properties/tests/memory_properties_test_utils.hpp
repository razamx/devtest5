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

#ifndef BUFFER_TEST_UTILS_HPP
#define BUFFER_TEST_UTILS_HPP

#include <stdlib.h>
#include <sched.h>
#include <stdint.h>
#include <vector>
#include "memory_properties.h"
//#include "tcc_config.h"

// helper structure for make_prop()
struct prop_args
{
	unsigned int latency;
	uint8_t mask;
	size_t size;
	tcc_memory_level type;
};

// Create cpu_set_t from integer bitmask
cpu_set_t cpu_set_from_bits(uint8_t mask)
{
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	for(size_t i = 0; i < sizeof(mask) * 8; i++) {
		if(mask & 1) CPU_SET(i, &cpu_set);
		mask >>= 1;
	}
	return cpu_set;
}

// Create cpu_set_t from cpuid
cpu_set_t cpu_set_from_cpu(uint cpu)
{
	cpu_set_t affinity;
	CPU_ZERO(&affinity);
	CPU_SET(cpu, &affinity);
	return affinity;
}

cpu_set_t cpu_set_from_list(std::vector<unsigned> set)
{
	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	for(auto cpu: set){
		CPU_SET(cpu, &cpu_set);
	}
	return cpu_set;
}

// create memory_properties easier
memory_properties_t make_prop(prop_args args)
{
	return (memory_properties_t){
		.mask = cpu_set_from_bits(args.mask),
		.latency = args.latency,
		.type = args.type,
		.size = args.size
	};
}

#define DEFAULT_SIZE ((size_t)-1)
#define DEFAULT_TYPE TCC_MEM_UNKNOWN

// Helper macro to create memory_properties_structure easier with "named arguments"
// Named arguments make test code more undertandable
#define prop(...) make_prop({__VA_ARGS__})

#endif // BUFFER_TEST_UTILS_HPP
