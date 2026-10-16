#pragma once

#include "Hazel/Utils/Serializable.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <cereal/types/memory.hpp>

namespace GameEngine {

class Mesh;

// CPU reference baker settings. The largest axis uses `resolution` samples;
// the other axes keep the same voxel size so the field remains isotropic.
struct MeshDistanceFieldSettings
{
    uint32_t resolution = 32;
    uint32_t paddingVoxels = 1;
    bool signedDistance = true;

private:
    BeginSerailize
        SerailizeEntry(resolution)
        SerailizeEntry(paddingVoxels)
        SerailizeEntry(signedDistance)
    EndSerailize
};

// Mesh-local signed distance samples. This is intentionally a dense CPU
// representation for the first implementation; it is the source data for a
// later GPU 3D texture / sparse-brick representation.
struct MeshDistanceFieldData
{
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
    glm::uvec3 resolution{0u};
    glm::vec3 voxelSize{0.0f};
    bool signedDistance = true;
    std::vector<float> samples;

    bool IsValid() const;
    size_t SampleCount() const;
    size_t SampleDataBytes() const;
    size_t StorageBytes() const;
    float SampleNearest(const glm::vec3& localPosition) const;
    float SampleTrilinear(const glm::vec3& localPosition) const;
    size_t SampleIndex(uint32_t x, uint32_t y, uint32_t z) const;

private:
    BeginSerailize
        SerailizeEntry(boundsMin)
        SerailizeEntry(boundsMax)
        SerailizeEntry(resolution)
        SerailizeEntry(voxelSize)
        SerailizeEntry(signedDistance)
        SerailizeEntry(samples)
    EndSerailize
};

class MeshDistanceFieldBaker
{
public:
    static bool Bake(const Mesh& mesh,
                     const MeshDistanceFieldSettings& settings,
                     MeshDistanceFieldData& output,
                     std::string* error = nullptr);

};

using MeshDistanceFieldRef = std::shared_ptr<MeshDistanceFieldData>;

}
