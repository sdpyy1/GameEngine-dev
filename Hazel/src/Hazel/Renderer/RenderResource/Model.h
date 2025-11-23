#pragma once
namespace GameEngine { 

    typedef struct ModelProcessSetting
    {
        bool smoothNormal = false;                  // 生成平滑法线
        bool flipUV = false;                        // 翻转UV
        bool loadMaterials = false;                 // 读取文件中的材质并生成材质资源
        bool tangentSpace = false;                  // 生成切线
        bool generateBVH = false;                   // 生成BVH
        bool generateCluster = false;               // 生成Cluster
        bool generateVirtualMesh = false;           // 生成虚拟几何体
        bool cacheCluster = false;                  // 对于虚拟几何体和Cluster做缓存，只需要生成一次
    }ModelProcessSetting;








	class Model {

		Model(std::string path, ModelProcessSetting processSetting);











	};
}

