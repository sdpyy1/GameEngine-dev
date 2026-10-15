#include "hzpch.h"
#include "MeshDistanceField.h"

#include "Mesh.h"

#include <cmath>
#include <limits>

namespace GameEngine {

namespace {

constexpr float kEpsilon = 1.0e-6f;

void SetError(std::string* error, const char* message)
{
    if (error)
        *error = message;
}

}

size_t MeshDistanceFieldData::SampleCount() const
{
    return static_cast<size_t>(resolution.x) *
           static_cast<size_t>(resolution.y) *
           static_cast<size_t>(resolution.z);
}

bool MeshDistanceFieldData::IsValid() const
{
    return resolution.x >= 2 && resolution.y >= 2 && resolution.z >= 2 &&
           voxelSize.x > 0.0f && voxelSize.y > 0.0f && voxelSize.z > 0.0f &&
           samples.size() == SampleCount();
}

size_t MeshDistanceFieldData::SampleIndex(uint32_t x, uint32_t y, uint32_t z) const
{
    return static_cast<size_t>(x) +
           static_cast<size_t>(resolution.x) *
               (static_cast<size_t>(y) + static_cast<size_t>(resolution.y) * z);
}

float MeshDistanceFieldData::SampleNearest(const glm::vec3& localPosition) const
{
    if (!IsValid())
        return 0.0f;

    const glm::vec3 coordinate = glm::clamp(
        (localPosition - boundsMin) / (boundsMax - boundsMin) *
            glm::vec3(resolution - glm::uvec3(1u)),
        glm::vec3(0.0f),
        glm::vec3(resolution - glm::uvec3(1u)));

    const glm::uvec3 sample = glm::uvec3(glm::round(coordinate));
    return samples[SampleIndex(sample.x, sample.y, sample.z)];
}

float MeshDistanceFieldData::SampleTrilinear(const glm::vec3& localPosition) const
{
    if (!IsValid())
        return 0.0f;

    const glm::vec3 coordinate = glm::clamp(
        (localPosition - boundsMin) / (boundsMax - boundsMin) *
            glm::vec3(resolution - glm::uvec3(1u)),
        glm::vec3(0.0f),
        glm::vec3(resolution - glm::uvec3(1u)));
    const glm::uvec3 lower = glm::uvec3(glm::floor(coordinate));
    const glm::uvec3 upper = glm::min(lower + glm::uvec3(1u), resolution - glm::uvec3(1u));
    const glm::vec3 fraction = coordinate - glm::vec3(lower);

    const float c000 = samples[SampleIndex(lower.x, lower.y, lower.z)];
    const float c100 = samples[SampleIndex(upper.x, lower.y, lower.z)];
    const float c010 = samples[SampleIndex(lower.x, upper.y, lower.z)];
    const float c110 = samples[SampleIndex(upper.x, upper.y, lower.z)];
    const float c001 = samples[SampleIndex(lower.x, lower.y, upper.z)];
    const float c101 = samples[SampleIndex(upper.x, lower.y, upper.z)];
    const float c011 = samples[SampleIndex(lower.x, upper.y, upper.z)];
    const float c111 = samples[SampleIndex(upper.x, upper.y, upper.z)];

    const float x00 = glm::mix(c000, c100, fraction.x);
    const float x10 = glm::mix(c010, c110, fraction.x);
    const float x01 = glm::mix(c001, c101, fraction.x);
    const float x11 = glm::mix(c011, c111, fraction.x);
    return glm::mix(glm::mix(x00, x10, fraction.y),
                    glm::mix(x01, x11, fraction.y),
                    fraction.z);
}

float MeshDistanceFieldBaker::PointTriangleDistanceSquared(const glm::vec3& point,
                                                            const glm::vec3& a,
                                                            const glm::vec3& b,
                                                            const glm::vec3& c)
{
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 ap = point - a;
    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
        return glm::dot(ap, ap);

    const glm::vec3 bp = point - b;
    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
        return glm::dot(bp, bp);

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        const glm::vec3 projection = a + v * ab;
        const glm::vec3 delta = point - projection;
        return glm::dot(delta, delta);
    }

    const glm::vec3 cp = point - c;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
        return glm::dot(cp, cp);

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        const glm::vec3 projection = a + w * ac;
        const glm::vec3 delta = point - projection;
        return glm::dot(delta, delta);
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        const glm::vec3 projection = b + w * (c - b);
        const glm::vec3 delta = point - projection;
        return glm::dot(delta, delta);
    }

    const glm::vec3 normal = glm::cross(ab, ac);
    const float normalLengthSquared = glm::dot(normal, normal);
    if (normalLengthSquared <= kEpsilon)
        return std::numeric_limits<float>::max();

    const float distance = glm::dot(point - a, normal);
    return (distance * distance) / normalLengthSquared;
}

bool MeshDistanceFieldBaker::RayIntersectsTriangle(const glm::vec3& origin,
                                                    const glm::vec3& direction,
                                                    const glm::vec3& a,
                                                    const glm::vec3& b,
                                                    const glm::vec3& c,
                                                    float& distance)
{
    const glm::vec3 edge1 = b - a;
    const glm::vec3 edge2 = c - a;
    const glm::vec3 h = glm::cross(direction, edge2);
    const float determinant = glm::dot(edge1, h);
    if (std::abs(determinant) <= kEpsilon)
        return false;

    const float inverseDeterminant = 1.0f / determinant;
    const glm::vec3 s = origin - a;
    const float u = inverseDeterminant * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f)
        return false;

    const glm::vec3 q = glm::cross(s, edge1);
    const float v = inverseDeterminant * glm::dot(direction, q);
    if (v < 0.0f || u + v > 1.0f)
        return false;

    distance = inverseDeterminant * glm::dot(edge2, q);
    return distance > kEpsilon;
}

bool MeshDistanceFieldBaker::Bake(const Mesh& mesh,
                                  const MeshDistanceFieldSettings& settings,
                                  MeshDistanceFieldData& output,
                                  std::string* error)
{
    output = MeshDistanceFieldData{};

    if (mesh.position.empty())
    {
        SetError(error, "Mesh has no position data.");
        return false;
    }
    if (mesh.index.size() < 3 || mesh.index.size() % 3 != 0)
    {
        SetError(error, "Mesh index data must contain complete triangles.");
        return false;
    }
    if (settings.resolution < 2)
    {
        SetError(error, "Distance field resolution must be at least 2.");
        return false;
    }

    glm::vec3 sourceMin = mesh.position.front();
    glm::vec3 sourceMax = sourceMin;
    for (const glm::vec3& position : mesh.position)
    {
        sourceMin = glm::min(sourceMin, position);
        sourceMax = glm::max(sourceMax, position);
    }

    const glm::vec3 sourceExtent = sourceMax - sourceMin;
    const float largestExtent = std::max(sourceExtent.x, std::max(sourceExtent.y, sourceExtent.z));
    if (largestExtent <= kEpsilon)
    {
        SetError(error, "Mesh bounds are degenerate.");
        return false;
    }

    const float voxelLength = largestExtent / static_cast<float>(settings.resolution - 1);
    const float padding = voxelLength * static_cast<float>(settings.paddingVoxels);
    const glm::vec3 boundsMin = sourceMin - glm::vec3(padding);
    const glm::vec3 paddedExtent = sourceExtent + glm::vec3(padding * 2.0f);
    const glm::uvec3 resolution = glm::uvec3(
        glm::max(glm::uvec3(2u), glm::uvec3(glm::ceil(paddedExtent / voxelLength)) + glm::uvec3(1u)));
    const glm::vec3 boundsMax = boundsMin + glm::vec3(resolution - glm::uvec3(1u)) * voxelLength;

    output.boundsMin = boundsMin;
    output.boundsMax = boundsMax;
    output.resolution = resolution;
    output.voxelSize = glm::vec3(voxelLength);
    output.signedDistance = settings.signedDistance;
    output.samples.resize(output.SampleCount());

    const glm::vec3 rayDirection(1.0f, 0.0f, 0.0f);
    const float rayEpsilon = voxelLength * 1.0e-4f;
    for (uint32_t z = 0; z < resolution.z; ++z)
    {
        for (uint32_t y = 0; y < resolution.y; ++y)
        {
            for (uint32_t x = 0; x < resolution.x; ++x)
            {
                const glm::vec3 samplePosition = boundsMin +
                    glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * voxelLength;

                float nearestDistanceSquared = std::numeric_limits<float>::max();
                uint32_t intersectionCount = 0;
                const glm::vec3 rayOrigin = samplePosition + glm::vec3(0.0f, rayEpsilon, rayEpsilon * 2.0f);

                for (size_t triangle = 0; triangle < mesh.index.size(); triangle += 3)
                {
                    const uint32_t i0 = mesh.index[triangle + 0];
                    const uint32_t i1 = mesh.index[triangle + 1];
                    const uint32_t i2 = mesh.index[triangle + 2];
                    if (i0 >= mesh.position.size() || i1 >= mesh.position.size() || i2 >= mesh.position.size())
                    {
                        SetError(error, "Mesh index references a missing vertex.");
                        output = MeshDistanceFieldData{};
                        return false;
                    }

                    nearestDistanceSquared = std::min(nearestDistanceSquared,
                        PointTriangleDistanceSquared(samplePosition,
                                                     mesh.position[i0],
                                                     mesh.position[i1],
                                                     mesh.position[i2]));

                    if (settings.signedDistance)
                    {
                        float intersectionDistance = 0.0f;
                        if (RayIntersectsTriangle(rayOrigin,
                                                  rayDirection,
                                                  mesh.position[i0],
                                                  mesh.position[i1],
                                                  mesh.position[i2],
                                                  intersectionDistance))
                        {
                            ++intersectionCount;
                        }
                    }
                }

                float distance = std::sqrt(nearestDistanceSquared);
                if (settings.signedDistance && (intersectionCount & 1u) != 0u)
                    distance = -distance;
                output.samples[output.SampleIndex(x, y, z)] = distance;
            }
        }
    }

    return true;
}

}
