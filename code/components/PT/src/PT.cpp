#include "server/Server.hpp"

#include "PT.hpp"

#include "VertexTransformer.hpp"
#include "intersections/intersections.hpp"

#include "glm/gtc/matrix_transform.hpp"

bool is_obj = true;

namespace PT
{
    RGB SimplePathTracerRenderer::gamma(const RGB& rgb) {
        return glm::sqrt(rgb);
    }

    void SimplePathTracerRenderer::renderTask(RGBA* pixels, int width, int height, int off, int step) {
        cout << "test5";
        for(int i=off; i<height; i+=step) {
            for (int j=0; j<width; j++) {
                Vec3 color{0, 0, 0};
                for (int k=0; k < samples; k++) {
                    auto r = defaultSamplerInstance<UniformInSquare>().sample2d();
                    float rx = r.x;
                    float ry = r.y;
                    float x = (float(j)+rx)/float(width);
                    float y = (float(i)+ry)/float(height);
                    auto ray = camera.shoot(x, y);        // 从摄像机发出光源

                    color += trace(ray, 0);

                }
                color /= samples;
                color = gamma(color);
                pixels[(height-i-1)*width+j] = {color, 1};
            }
        }
    }

    auto SimplePathTracerRenderer::render() -> RenderResult {
        // shaders
        shaderPrograms.clear();
        ShaderCreator shaderCreator{};
        for (auto& m : scene.materials) {
            shaderPrograms.push_back(shaderCreator.create(m, scene.textures));
        }

        RGBA* pixels = new RGBA[width*height]{};

        // 局部坐标转换成世界坐标
        VertexTransformer vertexTransformer{};
        vertexTransformer.exec(spScene);

        const auto taskNums = 8;
        thread t[taskNums];
        for (int i=0; i < taskNums; i++) {
            t[i] = thread(&SimplePathTracerRenderer::renderTask,
                this, pixels, width, height, i, taskNums);
        }
        for(int i=0; i < taskNums; i++) {
            t[i].join();
        }
        getServer().logger.log("Done...");
        return {pixels, width, height};
    }

    void SimplePathTracerRenderer::release(const RenderResult& r) {
        auto [p, w, h] = r;
        delete[] p;
    }

    HitRecord SimplePathTracerRenderer::closestHitObject(const Ray& r) {
        HitRecord closestHit = nullopt;
        float closest = FLOAT_INF;
        for (auto& s : scene.sphereBuffer) {
            auto hitRecord = Intersection::xSphere(r, s, 0.000001, closest);
            if (hitRecord && hitRecord->t < closest) {
                closest = hitRecord->t;
                closestHit = hitRecord;
            }
        }
        for (auto& t : scene.triangleBuffer) {
            auto hitRecord = Intersection::xTriangle(r, t, 0.000001, closest);
            if (hitRecord && hitRecord->t < closest) {
                closest = hitRecord->t;
                closestHit = hitRecord;
            }
        }
        for (auto& p : scene.planeBuffer) {
            auto hitRecord = Intersection::xPlane(r, p, 0.000001, closest);
            if (hitRecord && hitRecord->t < closest) {
                is_obj = false;
                closest = hitRecord->t;
                closestHit = hitRecord;
            }
        }
        return closestHit; 
    }
    
    tuple<float, Vec3> SimplePathTracerRenderer::closestHitLight(const Ray& r) {
        Vec3 v = {};
        HitRecord closest = getHitRecord(FLOAT_INF, {}, {}, {});
        for (auto& a : scene.areaLightBuffer) {
            auto hitRecord = Intersection::xAreaLight(r, a, 0.000001, closest->t);
            if (hitRecord && closest->t > hitRecord->t) {
                closest = hitRecord;
                v = a.radiance;
            }
        }
        return { closest->t, v };
    }

    RGB SimplePathTracerRenderer::trace(const Ray& r, int currDepth) {
        if (currDepth == depth) return scene.ambient.constant;

        auto hitObject = closestHitObject(r);         // 计算先碰到的物体，返回值是（t + 击中的点 + 法向量 + 材质）

        auto [ t, emitted ] = closestHitLight(r);     // 似乎是在判断能否回到光源处

        Vec3 refrac_dir = r.direction;

        // hit object
        if (hitObject && hitObject->t < t) {
            auto mtlHandle = hitObject->material;
            auto scattered = shaderPrograms[mtlHandle.index()]->shade(r, hitObject->hitPoint, hitObject->normal);  // 看 Lambertian.cpp，使用shade初始化scattered
            /*
                scattered包括：
                Ray ray = {};
                Vec3 attenuation = {};
                Vec3 emitted = {};
                float pdf = {0.f};
            */
            auto scatteredRay = scattered.ray;         // “新”的光，是经由Lambertian散射过的，这边是使用了Random的散射方式

            auto attenuation = scattered.attenuation;
            auto emitted = scattered.emitted;
            auto next = trace(scatteredRay, currDepth+1);
            if (is_obj)
            {

                //auto throughedRay = Ray(hitObject->hitPoint, refrac_dir);
                //auto through = trace(throughedRay, currDepth + 1);
                //next = through + next;
            }
            else                  //当碰到的是一个“Plane”时
            {
                is_obj = true;
            }
            float n_dot_in = glm::dot(hitObject->normal, scatteredRay.direction);
            float pdf = scattered.pdf;
            /**
             * emitted      - Le(p, w_0)
             * next         - Li(p, w_i)
             * n_dot_in     - cos<n, w_i>
             * atteunation  - BRDF
             * pdf          - p(w)
             **/
            return emitted + attenuation * next * n_dot_in / pdf;  // 渲染方程，返回这条光路的颜色值，在RenderTask中被累加：27行
        }
        // 
        else if (t != FLOAT_INF) {
            return emitted;
        }
        else {
            return Vec3{0};
        }
    }
}