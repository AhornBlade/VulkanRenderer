#include "spirv_reflect_wrapper.hpp"

namespace spv_reflect_wrapper{

std::string getResultInfo(SpvReflectResult result)
{
    switch (result) {
        case SPV_REFLECT_RESULT_SUCCESS: return "Success";
        case SPV_REFLECT_RESULT_NOT_READY: return "Not Ready";
        case SPV_REFLECT_RESULT_ERROR_PARSE_FAILED: return "Parse Failed";
        case SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED: return "Alloc Failed";
        case SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED: return "Range Exceeded";
        case SPV_REFLECT_RESULT_ERROR_NULL_POINTER: return "Null Pointer";
        case SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR: return "Internal Error";
        case SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH: return "Count Mismatch";
        case SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND: return "Element not found";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE: return "spirv invalid code size";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER: return "spirv invalid magic number";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF: return "spirv unexcepted EOF";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE: return "spirv invaild id reference";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW: return "spirv set number overflow";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS: return "spirv invalid storage class";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION: return "spirv recursion";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION: return "spirv invalid instruction";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA: return "spirv unexpected block data";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE: return "spirv invalid block member reference";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT: return "spirv invalid entry point";
        case SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE: return "spirv invalid execution mode";
    }
}

void throwSPIRVResult(SpvReflectResult result)
{
    if(result == SPV_REFLECT_RESULT_SUCCESS)
        return;
    else
        throw std::runtime_error(std::format("Error: {}", getResultInfo(result)));
}

SpvReflectShaderModule::SpvReflectShaderModule(size_t size, const uint32_t *p_code)
{
    auto result = spvReflectCreateShaderModule(size * 4, p_code, &module);
    throwSPIRVResult(result);
}

SpvReflectShaderModule::~SpvReflectShaderModule() noexcept
{
    spvReflectDestroyShaderModule(&module);
}

std::vector<SpvReflectInterfaceVariable*> SpvReflectShaderModule::enumerateInputVariables() const
{
    uint32_t count;
    auto result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
    throwSPIRVResult(result);
    std::vector<SpvReflectInterfaceVariable*> variables(count);
    spvReflectEnumerateInputVariables(&module, &count, variables.data());
    return variables;
}

std::vector<SpvReflectInterfaceVariable*> SpvReflectShaderModule::enumerateOutputVariables() const
{
    uint32_t count;
    auto result = spvReflectEnumerateOutputVariables(&module, &count, nullptr);
    throwSPIRVResult(result);
    std::vector<SpvReflectInterfaceVariable*> variables(count);
    spvReflectEnumerateOutputVariables(&module, &count, variables.data());
    return variables;
}

std::vector<SpvReflectDescriptorSet*> SpvReflectShaderModule::enumerateDescriptorSets() const
{
    uint32_t count;
    auto result = spvReflectEnumerateDescriptorSets(&module, &count, nullptr);
    throwSPIRVResult(result);
    std::vector<SpvReflectDescriptorSet*> descriptorSet(count);
    spvReflectEnumerateDescriptorSets(&module, &count, descriptorSet.data());
    return descriptorSet;
}

std::vector<SpvReflectBlockVariable*> SpvReflectShaderModule::enumeratePushConstantBlocks() const
{
    uint32_t count;
    auto result = spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
    throwSPIRVResult(result);
    std::vector<SpvReflectBlockVariable*> pushConstants(count);
    spvReflectEnumeratePushConstantBlocks(&module, &count, pushConstants.data());
    return pushConstants;
}

std::vector<SpvReflectSpecializationConstant*> SpvReflectShaderModule::enumerateSpecializationConstants() const
{
    uint32_t count;
    auto result = spvReflectEnumerateSpecializationConstants(&module, &count, nullptr);
    throwSPIRVResult(result);
    std::vector<SpvReflectSpecializationConstant*> specConstants(count);
    spvReflectEnumerateSpecializationConstants(&module, &count, specConstants.data());
    return specConstants;
}

size_t getInterfaceVariableSize(const SpvReflectInterfaceVariable& variable)
{
    size_t baseSize = 0;
    for(uint32_t index = 0; index < variable.member_count; index++)
    {
        baseSize += getInterfaceVariableSize(variable.members[index]);
    }
    if(variable.numeric.vector.component_count)
        baseSize = variable.numeric.scalar.width * variable.numeric.vector.component_count;
    else if(variable.numeric.matrix.column_count && variable.numeric.matrix.row_count)
        baseSize = variable.numeric.scalar.width * variable.numeric.matrix.column_count *
            variable.numeric.matrix.row_count;
    else
        baseSize = variable.numeric.scalar.width;
    
    for(uint32_t index = 0; index < variable.array.dims_count; index++)
    {
        baseSize *= variable.array.dims[index];
    }

    return baseSize / 8;
}

}//namespace spv_reflect_wrapper