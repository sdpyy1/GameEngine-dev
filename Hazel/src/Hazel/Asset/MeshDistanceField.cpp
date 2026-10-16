#include "hzpch.h"
#include "MeshDistanceField.h"

#include "Mesh.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

#if defined(HZ_USE_EMBREE) && HZ_USE_EMBREE
#include <embree4/rtcore.h>
#include <embree4/rtcore_ray.h>
#endif

namespace GameEngine {

namespace {

constexpr float kEpsilon = 1.0e-6f;

void SetError(std::string* error, const char* message)
{
    if (error)
        *error = message;
}

float PointTriangleDistanceSquaredImpl(const glm::vec3& point,
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
        const glm::vec3 delta = point - (a + v * ab);
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
        const glm::vec3 delta = point - (a + w * ac);
        return glm::dot(delta, delta);
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        const glm::vec3 delta = point - (b + w * (c - b));
        return glm::dot(delta, delta);
    }

    const glm::vec3 normal = glm::cross(ab, ac);
    const float normalLengthSquared = glm::dot(normal, normal);
    if (normalLengthSquared <= kEpsilon)
        return std::numeric_limits<float>::max();

    const float distance = glm::dot(point - a, normal);
    return (distance * distance) / normalLengthSquared;
}

#if defined(HZ_USE_EMBREE) && HZ_USE_EMBREE

// UE's MeshUtilities builds one Embree scene for the mesh and reuses it for
// every voxel. The scene is immutable after Commit(), so Embree can service
// the point and ray queries used by the SDF baker safely from worker threads.
class EmbreeMeshScene
{
public:
    ~EmbreeMeshScene()
    {
        if (scene)
            rtcReleaseScene(scene);
        if (device)
            rtcReleaseDevice(device);
    }

    bool Build(const Mesh& mesh, std::string* error)
    {
        device = rtcNewDevice(nullptr);
        if (!device)
        {
            SetError(error, "Embree could not create a device.");
            return false;
        }

        scene = rtcNewScene(device);
        if (!scene)
        {
            SetError(error, "Embree could not create a scene.");
            return false;
        }

        geometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
        if (!geometry)
        {
            SetError(error, "Embree could not create triangle geometry.");
            return false;
        }

        vertexData = mesh.position.data();
        indexData = mesh.index.data();
        vertexCount = mesh.position.size();
        triangleCount = mesh.index.size() / 3;

        rtcSetSharedGeometryBuffer(geometry,
                                   RTC_BUFFER_TYPE_VERTEX,
                                   0,
                                   RTC_FORMAT_FLOAT3,
                                   vertexData,
                                   0,
                                   sizeof(glm::vec3),
                                   vertexCount);
        rtcSetSharedGeometryBuffer(geometry,
                                   RTC_BUFFER_TYPE_INDEX,
                                   0,
                                   RTC_FORMAT_UINT3,
                                   indexData,
                                   0,
                                   sizeof(uint32_t) * 3,
                                   triangleCount);
        rtcCommitGeometry(geometry);
        rtcAttachGeometry(scene, geometry);
        rtcReleaseGeometry(geometry);
        geometry = nullptr;
        rtcCommitScene(scene);

        const RTCError errorCode = rtcGetDeviceError(device);
        if (errorCode != RTC_ERROR_NONE)
        {
            SetError(error, "Embree failed while committing triangle geometry.");
            return false;
        }
        return true;
    }

    RTCScene GetScene() const { return scene; }
    const glm::vec3* GetVertices() const { return vertexData; }
    const uint32_t* GetIndices() const { return indexData; }
    size_t GetTriangleCount() const { return triangleCount; }

private:
    RTCDevice device = nullptr;
    RTCScene scene = nullptr;
    RTCGeometry geometry = nullptr;
    const glm::vec3* vertexData = nullptr;
    const uint32_t* indexData = nullptr;
    size_t vertexCount = 0;
    size_t triangleCount = 0;
};

struct EmbreePointQueryContext : RTCPointQueryContext
{
    const EmbreeMeshScene* mesh = nullptr;
};

bool EmbreePointQueryFunction(RTCPointQueryFunctionArguments* arguments)
{
    const auto* context = reinterpret_cast<const EmbreePointQueryContext*>(arguments->context);
    if (!context || !context->mesh || arguments->geomID == RTC_INVALID_GEOMETRY_ID ||
        arguments->primID >= context->mesh->GetTriangleCount())
        return false;

    const uint32_t* indices = context->mesh->GetIndices();
    const glm::vec3* vertices = context->mesh->GetVertices();
    const uint32_t triangle = arguments->primID;
    const glm::vec3& a = vertices[indices[triangle * 3 + 0]];
    const glm::vec3& b = vertices[indices[triangle * 3 + 1]];
    const glm::vec3& c = vertices[indices[triangle * 3 + 2]];
    const glm::vec3 queryPosition(arguments->query->x,
                                  arguments->query->y,
                                  arguments->query->z);
    const float distanceSquared = PointTriangleDistanceSquaredImpl(queryPosition, a, b, c);
    float& closestDistanceSquared = *static_cast<float*>(arguments->userPtr);
    if (distanceSquared < closestDistanceSquared)
    {
        closestDistanceSquared = distanceSquared;
        arguments->query->radius = std::sqrt(distanceSquared);
        return true;
    }
    return false;
}

float NextUEFraction(uint32_t& seed)
{
    seed = seed * 196314165u + 907633515u;
    uint32_t bits = 0x3F800000u | (seed >> 9);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value - 1.0f;
}

glm::vec3 UEUniformSampleHemisphere(float u, float v)
{
    u = u * 2.0f - 1.0f;
    v = v * 2.0f - 1.0f;
    if (u == 0.0f && v == 0.0f)
        return glm::vec3(0.0f);

    float radius = 0.0f;
    float theta = 0.0f;
    if (std::abs(u) > std::abs(v))
    {
        radius = u;
        theta = 3.14159265358979323846f * 0.25f * (v / u);
    }
    else
    {
        radius = v;
        theta = 3.14159265358979323846f * 0.5f -
            3.14159265358979323846f * 0.25f * (u / v);
    }

    const float x = radius * std::cos(theta);
    const float y = radius * std::sin(theta);
    const float radiusSquared = radius * radius;
    return glm::vec3(x * std::sqrt(std::max(0.0f, 2.0f - radiusSquared)),
                     y * std::sqrt(std::max(0.0f, 2.0f - radiusSquared)),
                     1.0f - radiusSquared);
}

std::vector<glm::vec3> BuildUESampleDirections()
{
    // This mirrors MeshUtilities::GenerateStratifiedUniformHemisphereSamples:
    // 7x7 samples per hemisphere, seed 0, then mirror the second hemisphere.
    constexpr uint32_t sampleDimension = 7;
    uint32_t seed = 0;
    std::vector<glm::vec3> directions;
    directions.reserve(sampleDimension * sampleDimension * 2);
    for (uint32_t x = 0; x < sampleDimension; ++x)
    {
        for (uint32_t y = 0; y < sampleDimension; ++y)
        {
            const float fractionX = (static_cast<float>(x) + NextUEFraction(seed)) /
                static_cast<float>(sampleDimension);
            const float fractionY = (static_cast<float>(y) + NextUEFraction(seed)) /
                static_cast<float>(sampleDimension);
            directions.push_back(UEUniformSampleHemisphere(fractionX, fractionY));
        }
    }
    for (uint32_t x = 0; x < sampleDimension; ++x)
    {
        for (uint32_t y = 0; y < sampleDimension; ++y)
        {
            const float fractionX = (static_cast<float>(x) + NextUEFraction(seed)) /
                static_cast<float>(sampleDimension);
            const float fractionY = (static_cast<float>(y) + NextUEFraction(seed)) /
                static_cast<float>(sampleDimension);
            glm::vec3 sample = UEUniformSampleHemisphere(fractionX, fractionY);
            sample.z *= -1.0f;
            directions.push_back(sample);
        }
    }
    return directions;
}

float EmbreeNearestDistance(const EmbreeMeshScene& scene,
                            const glm::vec3& position,
                            float searchRadius)
{
    RTCPointQuery query{};
    query.x = position.x;
    query.y = position.y;
    query.z = position.z;
    query.time = 0.0f;
    query.radius = searchRadius;

    EmbreePointQueryContext context{};
    rtcInitPointQueryContext(&context);
    context.mesh = &scene;
    float distanceSquared = searchRadius * searchRadius;
    rtcPointQuery(scene.GetScene(), &query, &context, EmbreePointQueryFunction, &distanceSquared);
    return std::sqrt(distanceSquared);
}

bool EmbreeRayHit(const EmbreeMeshScene& scene,
                  const glm::vec3& origin,
                  const glm::vec3& direction,
                  float maxDistance,
                  bool& backFace)
{
    RTCRayHit rayHit{};
    rayHit.ray.org_x = origin.x;
    rayHit.ray.org_y = origin.y;
    rayHit.ray.org_z = origin.z;
    rayHit.ray.dir_x = direction.x;
    rayHit.ray.dir_y = direction.y;
    rayHit.ray.dir_z = direction.z;
    rayHit.ray.tnear = 0.0f;
    rayHit.ray.tfar = 1.0f;
    rayHit.ray.mask = 0xFFFFFFFFu;
    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    rayHit.hit.primID = RTC_INVALID_GEOMETRY_ID;

    RTCIntersectArguments arguments{};
    rtcInitIntersectArguments(&arguments);
    rtcIntersect1(scene.GetScene(), &rayHit, &arguments);
    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID ||
        rayHit.ray.tfar < 0.0f || rayHit.ray.tfar > 1.0f)
        return false;

    const glm::vec3 geometricNormal(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z);
    backFace = glm::dot(direction, geometricNormal) > 0.0f;
    (void)maxDistance;
    return true;
}

#endif

}

size_t MeshDistanceFieldData::SampleCount() const
{
    return static_cast<size_t>(resolution.x) *
           static_cast<size_t>(resolution.y) *
           static_cast<size_t>(resolution.z);
}

size_t MeshDistanceFieldData::SampleDataBytes() const
{
    return samples.size() * sizeof(float);
}

size_t MeshDistanceFieldData::StorageBytes() const
{
    return sizeof(MeshDistanceFieldData) + samples.capacity() * sizeof(float);
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

#if !defined(HZ_USE_EMBREE) || !HZ_USE_EMBREE
    SetError(error, "This build was compiled without Embree support.");
    return false;
#else

    for (size_t triangle = 0; triangle < mesh.index.size(); triangle += 3)
    {
        if (mesh.index[triangle + 0] >= mesh.position.size() ||
            mesh.index[triangle + 1] >= mesh.position.size() ||
            mesh.index[triangle + 2] >= mesh.position.size())
        {
            SetError(error, "Mesh index references a missing vertex.");
            return false;
        }
    }

    glm::vec3 sourceMin = mesh.position.front();
    glm::vec3 sourceMax = sourceMin;
    for (const glm::vec3& position : mesh.position)
    {
        sourceMin = glm::min(sourceMin, position);
        sourceMax = glm::max(sourceMax, position);
    }

    // UE widens degenerate axes to one world unit so planes and thin meshes
    // still get a valid isotropic distance field.
    const glm::vec3 sourceCenter = (sourceMin + sourceMax) * 0.5f;
    const glm::vec3 sourceExtent = glm::max(sourceMax - sourceMin, glm::vec3(1.0f));
    sourceMin = sourceCenter - sourceExtent * 0.5f;
    sourceMax = sourceCenter + sourceExtent * 0.5f;
    const float largestExtent = std::max(sourceExtent.x, std::max(sourceExtent.y, sourceExtent.z));

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

    EmbreeMeshScene embreeScene;
    if (!embreeScene.Build(mesh, error))
    {
        output = MeshDistanceFieldData{};
        return false;
    }

    const std::vector<glm::vec3> sampleDirections = BuildUESampleDirections();
    // UE encodes four voxel diagonals around the surface. Keeping the same
    // trace band makes the sign test and far-field clamp behave like UE's
    // PF_G8 distance field even though Hazel still keeps float samples here.
    const float traceDistance = voxelLength * std::sqrt(3.0f) * 4.0f;
    const float rayEpsilon = 1.0e-4f;
    for (uint32_t z = 0; z < resolution.z; ++z)
    {
        for (uint32_t y = 0; y < resolution.y; ++y)
        {
            for (uint32_t x = 0; x < resolution.x; ++x)
            {
                const glm::vec3 samplePosition = boundsMin +
                    glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)) * voxelLength;

                float distance = EmbreeNearestDistance(embreeScene, samplePosition, traceDistance * 2.0f);
                bool traceRays = settings.signedDistance && distance <= traceDistance;
                int32_t hitCount = 0;
                int32_t backFaceCount = 0;
                if (traceRays)
                {
                    for (const glm::vec3& unitDirection : sampleDirections)
                    {
                        const glm::vec3 start = samplePosition - unitDirection * (rayEpsilon * traceDistance);
                        bool backFace = false;
                        if (EmbreeRayHit(embreeScene, start, unitDirection * traceDistance,
                                         traceDistance, backFace))
                        {
                            ++hitCount;
                            if (backFace)
                                ++backFaceCount;
                        }
                    }
                }

                distance = std::min(distance, traceDistance);
                if (settings.signedDistance && hitCount > 0 &&
                    backFaceCount > static_cast<int32_t>(sampleDirections.size() * 0.25f))
                    distance = -distance;
                output.samples[output.SampleIndex(x, y, z)] = distance;
            }
        }
    }

    return true;
#endif
}

}
