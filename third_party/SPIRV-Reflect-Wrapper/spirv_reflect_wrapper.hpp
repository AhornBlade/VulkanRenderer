#pragma once

#include "SPIRV-Reflect/spirv_reflect.h"

#include <string>
#include <stdexcept>
#include <format>

namespace spv_reflect_wrapper
{

std::string getResultInfo(SpvReflectResult result);
void throwSPIRVResult(SpvReflectResult result);

class SpvReflectShaderModule
{
public:
    SpvReflectShaderModule(size_t size, const uint32_t *p_code);
    ~SpvReflectShaderModule() noexcept;
    template<class T>
    SpvReflectShaderModule(size_t size, const T* p_code)
        : SpvReflectShaderModule{size*sizeof(T)/sizeof(uint32_t), reinterpret_cast<const uint32_t*>(p_code)} {}
    template<class T>
    SpvReflectShaderModule(const T& data)requires requires(T obj){ obj.size(); obj.data(); }
        : SpvReflectShaderModule{data.size(), data.data()} {}

    std::vector<SpvReflectInterfaceVariable*> enumerateInputVariables() const;
    std::vector<SpvReflectInterfaceVariable*> enumerateOutputVariables() const;
    std::vector<SpvReflectDescriptorSet*> enumerateDescriptorSets() const;
    std::vector<SpvReflectBlockVariable*> enumeratePushConstantBlocks() const;
    std::vector<SpvReflectSpecializationConstant*> enumerateSpecializationConstants() const;
    inline SpvReflectShaderStageFlagBits getShaderStage() const noexcept
    {
        return module.shader_stage;
    }
    inline const char* getEntryFunc() const noexcept
    {
        return module.entry_point_name;
    }


private:
    ::SpvReflectShaderModule module{};
};

size_t getInterfaceVariableSize(const SpvReflectInterfaceVariable& variable);

}// namespace spv_reflect_wrapper