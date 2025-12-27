#pragma once
#include "Hazel/Utils/Serializable.h"
#include <glm/glm.hpp>

namespace GameEngine {

    class AxisAlignedBox
    {
    public:
        AxisAlignedBox() {};
        AxisAlignedBox(const glm::vec3& center, const glm::vec3& halfExtent);
        ~AxisAlignedBox() {};

        void Merge(const glm::vec3& newPoint);
        void Merge(const AxisAlignedBox& other);
        void Update(const glm::vec3& center, const glm::vec3& halfExtent);
        AxisAlignedBox Transformed(const glm::mat4& transform) const;

        inline glm::vec3 GetCenter() const { return center; }
        inline glm::vec3 GetHalfExtent() const { return halfExtent; }
        inline glm::vec3 GetMinCorner() const { return minCorner; }
        inline glm::vec3 GetMaxCorner() const { return maxCorner; }

    private:
        glm::vec3 center = glm::zero<glm::vec3>();
        glm::vec3 halfExtent = glm::zero<glm::vec3>();
        glm::vec3 maxCorner = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 minCorner = glm::vec3(std::numeric_limits<float>::min());

    private:
        BeginSerailize
            SerailizeEntry(center)
            SerailizeEntry(halfExtent)
            SerailizeEntry(maxCorner)
            SerailizeEntry(minCorner)
            EndSerailize
    };

    struct Frustum
    {
        union
        {
            struct
            {
                glm::vec4 planeRight;
                glm::vec4 planeLeft;
                glm::vec4 planeTop;
                glm::vec4 planeBottom;
                glm::vec4 planeNear;
                glm::vec4 planeFar;
            };
            glm::vec4 planes[6];
        };
    };

    struct BoundingBox
    {
        glm::vec3 maxBound = glm::vec3(std::numeric_limits<float>::min());
        float _padding0 = 0.0f;

        glm::vec3 minBound = glm::vec3(std::numeric_limits<float>::max());
        float _padding1 = 0.0f;

        BoundingBox() {}

        BoundingBox(const glm::vec3& point)
        {
            minBound = point;
            maxBound = point;
        }

        BoundingBox(const glm::vec3& minv, const glm::vec3 maxv)
        {
            minBound = minv;
            maxBound = maxv;
        }

        BoundingBox(const AxisAlignedBox& aabb)
        {
            minBound = aabb.GetMinCorner();
            maxBound = aabb.GetMaxCorner();
        }

        void Merge(const BoundingBox& newBox)
        {
            minBound = glm::min(minBound, newBox.minBound);
            maxBound = glm::max(maxBound, newBox.maxBound);
        }
        /*void Merge(const glm::vec3& point)
        {
            minBound = point.cwiseMin(minBound).array().floor();
            maxBound = point.cwiseMax(maxBound).array().ceil();
        }*/
        void Merge(const glm::vec3& point)
        {
            // 取当前minBound和新点point各分量的最小值
            minBound = glm::min(minBound, point);
            // 取当前maxBound和新点point各分量的最大值
            maxBound = glm::max(maxBound, point);
        }

    private:
        BeginSerailize
            SerailizeEntry(maxBound)
            SerailizeEntry(minBound)
            EndSerailize
    };

    struct BoundingSphere
    {

        glm::vec3 center = glm::zero<glm::vec3>();
        float radius = 0.0f;



        BoundingSphere() {};
        BoundingSphere(const glm::vec3& center, const float& radius)
        {
            this->center = center;
            this->radius = radius;
        };
        BoundingSphere(const std::vector<glm::vec3>& points);
        BoundingSphere(const std::vector<BoundingSphere>& spheres);
        BoundingSphere(const BoundingBox& box);
        BoundingSphere(const AxisAlignedBox& box);
        BoundingSphere operator+(const BoundingSphere& other);

    private:
        BeginSerailize
            SerailizeEntry(center)
            SerailizeEntry(radius)
            EndSerailize
    };

    Frustum CreateFrustumFromMatrix(const glm::mat4& VP);

    bool FrustumIntersectBox(const Frustum& frustum, const BoundingBox& box);

    BoundingBox BoundingBoxTransform(const BoundingBox& box, const glm::mat4& mat);

    bool BoxIntersectSphere(const BoundingBox& box, const BoundingSphere& sphere);

    bool BoxIntersectBox(const BoundingBox& box1, const BoundingBox& box2);

}
