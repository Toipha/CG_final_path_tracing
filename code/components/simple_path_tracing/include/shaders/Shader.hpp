#pragma once
#ifndef __SHADER_HPP__
#define __SHADER_HPP__

#include "geometry/vec.hpp"
#include "common/macros.hpp"
#include "scene/Scene.hpp"

#include "Scattered.hpp"

namespace SimplePathTracer
{
    using namespace NRenderer;
    using namespace std;

    constexpr float PI = 3.1415926535898f;

    class Shader
    {
    protected:
        Material& material;
        vector<Texture>& textureBuffer;
    public:
        Shader(Material& material, vector<Texture>& textures)
            : material              (material)
            , textureBuffer         (textures)
        {}
        // Shader 类的构造函数，包含材质和纹理
        virtual Scattered shade(const Ray& ray, const Vec3& hitPoint, const Vec3& normal) const = 0;
        // 用shade初始化scattered ：包括：光，照到的点，此点的法向量
    };
    SHARE(Shader);
}

#endif