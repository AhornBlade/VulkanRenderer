#include <spirv_reflect_wrapper.hpp>

#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <format>
#include <filesystem>

#include "spirv_reflect_wrapper.hpp"

std::string getField(const SpvReflectTypeDescription& type, std::string name);

std::string getStructType(const SpvReflectTypeDescription& type)
{
    std::string memberField;
    for(uint32_t index = 0; index < type.member_count; index++)
    {
        const auto& member = type.members[index];
        memberField = memberField + getField(member, member.struct_member_name);
    }

    return std::format("struct {}\n{{\n{}}}", type.type_name, memberField);
}

std::string getDataType(const SpvReflectTypeDescription& type)
{
    if(type.type_flags & SPV_REFLECT_TYPE_FLAG_BOOL)
        return "bool";
    else if(type.type_flags & SPV_REFLECT_TYPE_FLAG_VOID)
        return "void";
    else if(type.type_flags & SPV_REFLECT_TYPE_FLAG_INT)
    {
        std::string lenth = std::to_string(type.traits.numeric.scalar.width);
        std::string preType = type.traits.numeric.scalar.signedness ? "int" : "uint";
        return std::format("{}{}_t", preType, lenth);
    }
    else if(type.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
        if(type.traits.numeric.scalar.width == 32)
            return "float";
        else if(type.traits.numeric.scalar.width == 64)
            return "double";
        else if(type.traits.numeric.scalar.width == 128)
            return "long double";
    }
    return "";
}

std::string getMathType(const SpvReflectTypeDescription& type)
{
    std::string dataType = getDataType(type);
    if(type.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)
        return std::format("glm::mat<{}, {}, {}>", type.traits.numeric.matrix.column_count,
            type.traits.numeric.matrix.row_count, dataType);
    if(type.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR)
        return std::format("glm::vec<{}, {}>", type.traits.numeric.vector.component_count, dataType);
    return "";
}

std::string getImageType(const SpvReflectTypeDescription& type)
{
    std::string dataType = getDataType(type);
    return std::format("std::vector<{}>", dataType);
}

std::string getType(const SpvReflectTypeDescription& type)
{
    if(type.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
        return getStructType(type);
    else if(type.type_flags & (SPV_REFLECT_TYPE_FLAG_MATRIX | SPV_REFLECT_TYPE_FLAG_VECTOR))
        return getMathType(type);
    else if(type.type_flags & (SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE | 
        SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE))
        return getImageType(type);
    else
        return getDataType(type);
}

std::string getField(const SpvReflectTypeDescription& type, std::string name)
{
    std::string arrays;
    for(uint32_t index = 0; index < type.traits.array.dims_count; index++)
    {
        arrays = arrays + std::format("[{}]", type.traits.array.dims[index]);
    }
    return std::format("{} {}{};\n", getType(type), name, arrays);
}

// std::string getSpecConstantField(const SpvReflectSpecializationConstant& constant)
// {
//     switch (constant.constant_type) 
//     {
//         case SPV_REFLECT_SPECIALIZATION_CONSTANT_BOOL:
//             return std::format("bool {} = {};\n", constant.name, constant.default_value.int_bool_value);
//         case SPV_REFLECT_SPECIALIZATION_CONSTANT_INT:
//             return std::format("int {} = {};\n", constant.name, constant.default_value.int_bool_value);
//         case SPV_REFLECT_SPECIALIZATION_CONSTANT_FLOAT:
//             return std::format("float {} = {};\n", constant.name, constant.default_value.float_value);
//     }
// }

void printDescriptorBinding(std::ostream& out, const SpvReflectDescriptorBinding& binding)
{
    out << getField(*(binding.type_description), binding.name);
}

void printDescriptorSet(std::ostream& out, const SpvReflectDescriptorSet& set)
{
    for(uint32_t index = 0; index < set.binding_count; index++)
    {
        printDescriptorBinding(out, *set.bindings[index]);
    }
}

void printInterfaceVariable(std::ostream& out, const SpvReflectInterfaceVariable& variable)
{
    out << getField(*(variable.type_description), variable.name);
}

void printPushConstant(std::ostream& out, const SpvReflectBlockVariable& constant)
{
    out << getField(*(constant.type_description), constant.name);
}

// void printSpecConstant(std::ostream& out, const SpvReflectSpecializationConstant& specConstant)
// {
//     out << getSpecConstantField(specConstant);
// }

void printShader(std::ostream& out, const spv_reflect_wrapper::SpvReflectShaderModule& module)
{
    out << "struct Descriptor\n{\n";
    auto descriptorSets = module.enumerateDescriptorSets();
    for(const auto& set : descriptorSets)
    {
        printDescriptorSet(out, *set);
    }
    out << "};\n\n";

    out << std::format("struct {}\n{{\n", "StageInput");
    auto inputs = module.enumerateInputVariables();
    for(const auto& input : inputs)
    {
        printInterfaceVariable(out, *input);
    }
    out << "};\n\n";

    out << std::format("struct {}\n{{\n", "StageOutput");
    auto outputs = module.enumerateOutputVariables();
    for(const auto& output : outputs)
    {
        printInterfaceVariable(out, *output);
    }
    out << "};\n\n";

    out << std::format("struct {}\n{{\n", "PushConstant");
    auto pushConstants = module.enumeratePushConstantBlocks();
    for(const auto& constant: pushConstants)
    {
        printPushConstant(out, *constant);
    }
    out << "};\n\n";

    // out << std::format("struct {}\n{{\n", "SpecializationConstant");
    // auto specConstants = module.enumerateSpecializationConstants();
    // for(const auto& constant: specConstants)
    // {
    //     printSpecConstant(out, *constant);
    // }
    // out << "};\n\n";
}

std::vector<uint32_t> readSourceFile(const std::string& filename)
{
    std::ifstream file{filename, std::ios::binary | std::ios::ate};

    if(!file.is_open())
    {
        throw std::runtime_error(std::format("Failed to open {}", filename));
    }

    auto filesize = file.tellg();
    std::vector<uint32_t> buffer(filesize/4);
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(buffer.data()), filesize);

    return buffer;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
       std::cout << "Usage: generate_shader [source_spirv_file] [target_header_file]";
       return -1;
    }

    std::string sourceFilename = argv[1];
    std::string targetFilename = argv[2];

    try {
        auto shader = readSourceFile(sourceFilename);
        spv_reflect_wrapper::SpvReflectShaderModule module{shader};
        std::ofstream targetFile{targetFilename, std::ios::out | std::ios::trunc};
        std::string shaderName = std::filesystem::path{targetFilename}.stem().string();
        targetFile << "#pragma once\n#include <array>\n#include <vector>\n#include <glm/glm.hpp>\n";
        targetFile << std::format("namespace ShaderData::{}{{\n", shaderName);
        targetFile << std::format("static const std::array<uint32_t, {}> code = \n{{\n    ", shader.size());
        for(uint32_t data : shader)
        {
            targetFile << std::format("{:#0x}, ", data);
        }
        targetFile << "\n};\n\n";
        printShader(targetFile, module);
        targetFile << std::format("}}//namespace ShaderData::{}\n", shaderName);
    } catch (const std::exception& e) {
        std::cout << e.what();
        return -1;
    }
}