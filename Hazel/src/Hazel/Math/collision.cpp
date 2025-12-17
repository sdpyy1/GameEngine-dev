#include "hzpch.h"
#include "collision.h"
#include <glm/gtx/norm.hpp>
namespace GameEngine {
    AxisAlignedBox::AxisAlignedBox(const glm::vec3& center, const glm::vec3& halfExtent) { Update(center, halfExtent); }

    void AxisAlignedBox::Merge(const glm::vec3& newPoint)
    {
        minCorner = glm::min(newPoint, minCorner);
        maxCorner = glm::max(newPoint, maxCorner);
        center = 0.5f * (minCorner + maxCorner);
        halfExtent = center - minCorner;
    }

    void AxisAlignedBox::Merge(const AxisAlignedBox& other)
    {
        minCorner = glm::min(minCorner, other.minCorner);
        maxCorner = glm::max(maxCorner, other.maxCorner);

        center = 0.5f * (minCorner + maxCorner);
        halfExtent = center - minCorner;
    }

    void AxisAlignedBox::Update(const glm::vec3& center, const glm::vec3& halfExtent)
    {
        this->center = center;
        this->halfExtent = halfExtent;
        this->minCorner = center - halfExtent;
        this->maxCorner = center + halfExtent;
    }
    AxisAlignedBox AxisAlignedBox::Transformed(const glm::mat4& transform) const
    {
        // 模型空间中心点
        glm::vec3 worldCenter = glm::vec3(transform * glm::vec4(center, 1.0f));

        // 旋转矩阵的三个轴
        glm::vec3 right = glm::vec3(transform[0]);
        glm::vec3 up = glm::vec3(transform[1]);
        glm::vec3 forward = glm::vec3(transform[2]);

        // 计算变换后的半尺寸（使用绝对值，保证AABB轴对齐）
        glm::vec3 worldHalfExtent =
            glm::abs(halfExtent.x * right) +
            glm::abs(halfExtent.y * up) +
            glm::abs(halfExtent.z * forward);

        return AxisAlignedBox(worldCenter, worldHalfExtent);
    }
    glm::vec4 computeBoundingSphere(const std::vector<glm::vec3>& points)
    {
        if (points.empty()) {
            return glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        if (points.size() == 1)
        {
            return glm::vec4(points[0], 0.0f);
        }
        else if (points.size() == 2)
        {
            glm::vec3 center = (points[0] + points[1]) * 0.5f;
            float radius = glm::distance(points[0], points[1]) * 0.5f;
            return glm::vec4(center, radius);
        }

        auto min_x_it = std::min_element(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b) { return a.x < b.x; });
        auto max_x_it = std::max_element(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b) { return a.x < b.x; });
        auto min_y_it = std::min_element(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b) { return a.y < b.y; });
        auto max_y_it = std::max_element(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b) { return a.y < b.y; });
        auto min_z_it = std::min_element(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b) { return a.z < b.z; });
        auto max_z_it = std::max_element(points.begin(), points.end(), [](const glm::vec3& a, const glm::vec3& b) { return a.z < b.z; });

        float dist_sq_x = glm::length2(*max_x_it - *min_x_it);
        float dist_sq_y = glm::length2(*max_y_it - *min_y_it);
        float dist_sq_z = glm::length2(*max_z_it - *min_z_it);

        float max_dist_sq = std::max({ dist_sq_x, dist_sq_y, dist_sq_z });

        glm::vec3 p1, p2;
        if (max_dist_sq == dist_sq_x) {
            p1 = *min_x_it;
            p2 = *max_x_it;
        }
        else if (max_dist_sq == dist_sq_y) {
            p1 = *min_y_it;
            p2 = *max_y_it;
        }
        else {
            p1 = *min_z_it;
            p2 = *max_z_it;
        }

        glm::vec3 center = (p1 + p2) * 0.5f;
        float radius = glm::sqrt(max_dist_sq) * 0.5f;
        float radius_sq = radius * radius;

        for (const auto& p : points)
        {
            float dist_sq_to_center = glm::length2(p - center);

            if (dist_sq_to_center > radius_sq)
            {
                float dist_to_center = glm::sqrt(dist_sq_to_center);

                radius = (radius + dist_to_center) * 0.5f;
                radius_sq = radius * radius;

                center += (p - center) * ((dist_to_center - radius) / dist_to_center);
            }
        }
        return glm::vec4(center, radius);
    }
    static BoundingSphere mergeSpheres(const BoundingSphere& a, const BoundingSphere& b)
    {
        BoundingSphere result;
        glm::vec3 dir = b.center - a.center;
        float distance_sq = glm::dot(dir, dir);
        float distance = glm::sqrt(distance_sq);

        // 情况1：如果一个球完全包含另一个球
        if (distance + a.radius <= b.radius + 1e-5f) {
            return b;
        }
        if (distance + b.radius <= a.radius + 1e-5f) {
            return a;
        }

        // 情况2：两个球部分重叠或完全分离
        result.radius = (distance + a.radius + b.radius) * 0.5f;

        // 避免除以零（当两球心完全重合时）
        if (distance > 1e-6f) {
            result.center = a.center + dir * ((result.radius - a.radius) / distance);
        }
        else {
            result.center = a.center; 
        }

        return result;
    }
    BoundingSphere::BoundingSphere(const std::vector<BoundingSphere>& spheres)
    {
        if (spheres.empty()) {
            *this = BoundingSphere(); 
            return;
        }
        auto min_x_it = std::min_element(spheres.begin(), spheres.end(),
            [](const BoundingSphere& s1, const BoundingSphere& s2) {
                return (s1.center.x - s1.radius) < (s2.center.x - s2.radius);
            });
        auto max_x_it = std::max_element(spheres.begin(), spheres.end(),
            [](const BoundingSphere& s1, const BoundingSphere& s2) {
                return (s1.center.x + s1.radius) < (s2.center.x + s2.radius);
            });

        auto min_y_it = std::min_element(spheres.begin(), spheres.end(),
            [](const BoundingSphere& s1, const BoundingSphere& s2) {
                return (s1.center.y - s1.radius) < (s2.center.y - s2.radius);
            });
        auto max_y_it = std::max_element(spheres.begin(), spheres.end(),
            [](const BoundingSphere& s1, const BoundingSphere& s2) {
                return (s1.center.y + s1.radius) < (s2.center.y + s2.radius);
            });

        auto min_z_it = std::min_element(spheres.begin(), spheres.end(),
            [](const BoundingSphere& s1, const BoundingSphere& s2) {
                return (s1.center.z - s1.radius) < (s2.center.z - s2.radius);
            });
        auto max_z_it = std::max_element(spheres.begin(), spheres.end(),
            [](const BoundingSphere& s1, const BoundingSphere& s2) {
                return (s1.center.z + s1.radius) < (s2.center.z + s2.radius);
            });
        float dist_x = glm::distance(min_x_it->center, max_x_it->center);
        float dist_y = glm::distance(min_y_it->center, max_y_it->center);
        float dist_z = glm::distance(min_z_it->center, max_z_it->center);

        float max_dist = std::max({ dist_x, dist_y, dist_z });

        BoundingSphere current_sphere;
        if (max_dist == dist_x) {
            current_sphere = mergeSpheres(*min_x_it, *max_x_it);
        }
        else if (max_dist == dist_y) {
            current_sphere = mergeSpheres(*min_y_it, *max_y_it);
        }
        else {
            current_sphere = mergeSpheres(*min_z_it, *max_z_it);
        }
        for (const auto& s : spheres) {
            current_sphere = mergeSpheres(current_sphere, s);
        }
        this->center = current_sphere.center;
        this->radius = current_sphere.radius;
    }
    BoundingSphere::BoundingSphere(const BoundingBox& box)
    {
        center = (box.maxBound + box.minBound)/ glm::vec3(2.0f);
        radius = glm::length(box.maxBound - center);
    }

    BoundingSphere::BoundingSphere(const AxisAlignedBox& box)
    {
        center = box.GetCenter();
        radius = glm::length(box.GetHalfExtent());
    }

    BoundingSphere BoundingSphere::operator+(const BoundingSphere& other)
    {
        glm::vec3 t = other.center - center;

        float tlen2 = pow(glm::length(t), 2);
        if (pow(radius - other.radius, 2) >= tlen2)
        {
            return radius < other.radius ? other : *this;
        }

        BoundingSphere sphere;
        float tlen = sqrt(tlen2);
        sphere.radius = (tlen + radius + other.radius) * 0.5;
        sphere.center = center + t * ((sphere.radius - radius) / tlen);

        return sphere;
    }

    Frustum CreateFrustumFromMatrix(const glm::mat4& VP)
    {
        Frustum frustum;
        // 右平面
        frustum.planeRight = glm::vec4(
            VP[0][3] - VP[0][0],
            VP[1][3] - VP[1][0],
            VP[2][3] - VP[2][0],
            VP[3][3] - VP[3][0]
        );

        // 左平面
        frustum.planeLeft = glm::vec4(
            VP[0][3] + VP[0][0],
            VP[1][3] + VP[1][0],
            VP[2][3] + VP[2][0],
            VP[3][3] + VP[3][0]
        );

        // 上平面
        frustum.planeTop = glm::vec4(
            VP[0][3] - VP[0][1],
            VP[1][3] - VP[1][1],
            VP[2][3] - VP[2][1],
            VP[3][3] - VP[3][1]
        );

        // 下平面
        frustum.planeBottom = glm::vec4(
            VP[0][3] + VP[0][1],
            VP[1][3] + VP[1][1],
            VP[2][3] + VP[2][1],
            VP[3][3] + VP[3][1]
        );

        // 远平面
        frustum.planeFar = glm::vec4(
            VP[0][3] - VP[0][2],
            VP[1][3] - VP[1][2],
            VP[2][3] - VP[2][2],
            VP[3][3] - VP[3][2]
        );

        // 近平面
        frustum.planeNear = glm::vec4(
            VP[0][3] + VP[0][2],
            VP[1][3] + VP[1][2],
            VP[2][3] + VP[2][2],
            VP[3][3] + VP[3][2]
        );

        // 归一化
        auto normalizePlane = [](glm::vec4& p)
            {
                float len = glm::length(glm::vec3(p));
                p /= len;
            };

        normalizePlane(frustum.planeRight);
        normalizePlane(frustum.planeLeft);
        normalizePlane(frustum.planeTop);
        normalizePlane(frustum.planeBottom);
        normalizePlane(frustum.planeNear);
        normalizePlane(frustum.planeFar);

        return frustum;
    }

    bool FrustumIntersectBox(const Frustum& frustum, const BoundingBox& box)
    {
        glm::vec3 boxCenter = (box.maxBound + box.minBound) * 0.5f;
        glm::vec3 boxExtents = (box.maxBound - box.minBound) * 0.5f;

        const glm::vec4* planes[] = {
            &frustum.planeRight,
            &frustum.planeLeft,
            &frustum.planeTop,
            &frustum.planeBottom,
            &frustum.planeNear,
            &frustum.planeFar
        };

        for (const auto* plane : planes)
        {
            float signedDistance = glm::dot(*plane, glm::vec4(boxCenter, 1.0f));
            glm::vec3 planeNormal(plane->x, plane->y, plane->z);
            float radiusProject = glm::dot(glm::abs(planeNormal), boxExtents);
            if (signedDistance > radiusProject)
            {
                return false;
            }
        }
        return true;
    }

    BoundingBox BoundingBoxTransform(const BoundingBox& box, const glm::mat4& mat)
    {
        glm::vec3 boxOffset[8] = {
            glm::vec3(-1.0f, -1.0f, 1.0f),  // 后下左
            glm::vec3(1.0f, -1.0f, 1.0f),   // 后下右
            glm::vec3(1.0f, 1.0f, 1.0f),    // 后上右
            glm::vec3(-1.0f, 1.0f, 1.0f),   // 后上左
            glm::vec3(-1.0f, -1.0f, -1.0f), // 前下左
            glm::vec3(1.0f, -1.0f, -1.0f),  // 前下右
            glm::vec3(1.0f, 1.0f, -1.0f),   // 前上右
            glm::vec3(-1.0f, 1.0f, -1.0f)   // 前上左
        };

        glm::vec3 center = (box.maxBound + box.minBound) * 0.5f;
        glm::vec3 extents = (box.maxBound - box.minBound) * 0.5f;

        glm::vec3 minTransformed;
        glm::vec3 maxTransformed;

        for (size_t i = 0; i < 8; ++i)
        {
            glm::vec3 cornerWorld = center + extents * boxOffset[i];

            glm::vec4 cornerHomogeneous = mat * glm::vec4(cornerWorld, 1.0f);

            glm::vec3 cornerNDC;
            if (cornerHomogeneous.w != 0.0f)
            {
                cornerNDC = glm::vec3(
                    cornerHomogeneous.x / cornerHomogeneous.w,
                    cornerHomogeneous.y / cornerHomogeneous.w,
                    cornerHomogeneous.z / cornerHomogeneous.w
                );
            }
            else
            {
                cornerNDC = glm::vec3(0.0f);
            }

            if (i == 0)
            {
                minTransformed = cornerNDC;
                maxTransformed = cornerNDC;
            }
            else
            {
                minTransformed = glm::min(minTransformed, cornerNDC);
                maxTransformed = glm::max(maxTransformed, cornerNDC);
            }
        }
        BoundingBox out;
        out.minBound = minTransformed;
        out.maxBound = maxTransformed;

        return out;
    }

    bool BoxIntersectSphere(const BoundingBox& box, const BoundingSphere& sphere)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (sphere.center[i] < box.minBound[i])
            {
                if ((box.minBound[i] - sphere.center[i]) > sphere.radius)
                {
                    return false;
                }
            }
            else if (sphere.center[i] > box.maxBound[i])
            {
                if ((sphere.center[i] - box.maxBound[i]) > sphere.radius)
                {
                    return false;
                }
            }
        }

        return true;
    }
    bool BoxIntersectBox(const BoundingBox& box1, const BoundingBox& box2)
    {
        // TODO: 分离轴定理（SAT）：检查三个坐标轴上的投影是否都有重叠
        bool noOverlapInX = box1.maxBound.x < box2.minBound.x || box2.maxBound.x < box1.minBound.x;
        bool noOverlapInY = box1.maxBound.y < box2.minBound.y || box2.maxBound.y < box1.minBound.y;
        bool noOverlapInZ = box1.maxBound.z < box2.minBound.z || box2.maxBound.z < box1.minBound.z;
        return !(noOverlapInX || noOverlapInY || noOverlapInZ);
    }


















}