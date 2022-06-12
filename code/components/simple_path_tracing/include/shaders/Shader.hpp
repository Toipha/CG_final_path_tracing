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
        // Shader ��Ĺ��캯�����������ʺ�����
        virtual Scattered shade(const Ray& ray, const Vec3& hitPoint, const Vec3& normal) const = 0;
        // ��shade��ʼ��scattered ���������⣬�յ��ĵ㣬�˵�ķ�����
    };
    SHARE(Shader);
}

#endif