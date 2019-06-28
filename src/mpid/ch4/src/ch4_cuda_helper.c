#include "ch4_cuda_helper.h"

int is_mem_type_device(const void *address)
{
    CUmemorytype memory_type;
    uint32_t is_managed = 0;

    void *attribute_data[] = { (void *) &memory_type, (void *) &is_managed };

    CUpointer_attribute required_attributes[2] = { CU_POINTER_ATTRIBUTE_MEMORY_TYPE,
        CU_POINTER_ATTRIBUTE_IS_MANAGED
    };
    CUresult result;

    if (address == NULL) {
        return 0;
    }

    result = cuPointerGetAttributes(2, required_attributes, attribute_data, (CUdeviceptr) address);
    return ((result == CUDA_SUCCESS) && (!is_managed && (memory_type == CU_MEMORYTYPE_DEVICE)));
}
