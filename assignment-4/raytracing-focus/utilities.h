//
// Created by danielr on 29.11.20.
//

#ifndef UTILITES_H
#define UTILITES_H

#include <utility>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>

#include "vec3.h"
#include "ray.h"

struct Material {
    QVector3D ka;
    QVector3D kd;
    QVector3D ks;
    float shinyFactor;
};

template<typename T>
struct autoMoved {
    T val;

    autoMoved() : val() {}
    explicit autoMoved(T val) : val(val) {}

    autoMoved(autoMoved&& other) noexcept : val(other.val) {
        other.val = T();
    }

    autoMoved& operator=(autoMoved&& other) noexcept {
        using std::swap;
        swap(other.val, val);
        return *this;
    }

    autoMoved(const autoMoved& other) = delete;
    autoMoved& operator=(const autoMoved& other) = delete;
    T operator()() { return val; }
};

inline float dot(const Vec3f& a, const Vec3f& b)
{
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

extern const GLfloat BoxVertices[];
extern const size_t BoxVerticesSize;
extern const GLuint BoxLineIndices[];
extern const size_t BoxLineIndicesSize;
extern const GLuint BoxTriangleIndices[];
extern const size_t BoxTriangleIndicesSize;

GLuint loadImageIntoTexture(QOpenGLFunctions_3_3_Core* f, const char* fileName, bool wrap = false);
GLuint loadCubeMap(QOpenGLFunctions_3_3_Core* f, const char* fileName[6]);

bool rayAABBIntersect(const Ray<float>& r, const Vec3f& vmin, const Vec3f& vmax, float t0, float t1);


inline QVector3D& Vec3fToQVector3D(Vec3f& vec) { return reinterpret_cast<QVector3D&>(vec); }
inline const QVector3D& Vec3fToQVector3D(const Vec3f& vec) { return reinterpret_cast<const QVector3D&>(vec); }
inline Vec3f& QVector3DToVec3f(QVector3D& vec) { return reinterpret_cast<Vec3f&>(vec); }
inline const Vec3f& QVector3DToVec3f(const QVector3D& vec) { return reinterpret_cast<const Vec3f&>(vec); }

/* TODO: Ex 4.1a This function is not implemented correctly. In order to make ray tracing work correctly, you have to make
 * sure that it actually finds the earliest intersection. */
template <typename InputIterator>
InputIterator intersectRayObjectsEarliest(InputIterator objects_begin, InputIterator objects_end, const Ray<float>& ray, float& t, float& u, float& v, unsigned int& hitTri, unsigned int& intersectionTests) {
    
    float t_min = std::numeric_limits<float>::max(); // initialize intersection distance to maximum to make sure its bigger than intersection
    InputIterator earliest_iter = objects_end; // iterator for closest hit object
    unsigned int eraliest_hitTri = 0; // index of closest hit triangle
    float earliest_u = 0.0f; // barycentric coords
    float earliest_v = 0.0f;
   
    // iterate through all scene objects
    for (InputIterator iter = objects_begin; iter != objects_end; iter++) {
        // get bounding box of object 
        QVector3D boundingBoxMin(iter->mesh.getBoundingBoxMin().x(), iter->mesh.getBoundingBoxMin().y(), iter->mesh.getBoundingBoxMin().z());
        QVector3D boundingBoxMax(iter->mesh.getBoundingBoxMax().x(), iter->mesh.getBoundingBoxMax().y(), iter->mesh.getBoundingBoxMax().z());

        // apply objects transform matrix to bounding box
        const QMatrix4x4& modelMatrix = iter->getModelMatrix();
        boundingBoxMin = modelMatrix.map(boundingBoxMin);
        boundingBoxMax = modelMatrix.map(boundingBoxMax);


    // check if ray intersects with bounding box. If yes, continue and look for traingle
        if (!rayAABBIntersect(ray, { boundingBoxMin.x(), boundingBoxMin.y(), boundingBoxMin.z() }, { boundingBoxMax.x(), boundingBoxMax.y(), boundingBoxMax.z() }, 0.0f, t_min))
            continue;

        const std::vector<Vec3f>& vertices = iter->mesh.getVertices();
        const std::vector<Vec3ui>& triangles = iter->mesh.getTriangles();

        // iterate over all triangles of objects
        for (unsigned int j = 0; j < triangles.size(); j++) {
            // convert vertices to position in QVector3d
            QVector3D p0(Vec3fToQVector3D(vertices[triangles[j][0]]));
            QVector3D p1(Vec3fToQVector3D(vertices[triangles[j][1]]));
            QVector3D p2(Vec3fToQVector3D(vertices[triangles[j][2]]));

            // transform triangles to world space
            p0 = modelMatrix.map(p0);
            p1 = modelMatrix.map(p1);
            p2 = modelMatrix.map(p2);

            // intersect data saves
            float current_t = t_min;
            float current_u = u;
            float current_v = v;

            // get closest hit of triangle
            bool hit = ray.triangleIntersect(QVector3DToVec3f(p0), QVector3DToVec3f(p1), QVector3DToVec3f(p2), current_u, current_v, current_t);

#pragma omp atomic
            intersectionTests++;

            // check if intersection is valid and if its closest found yet
            if (hit && current_t > 0.0f && current_t < t_min) {
                t_min = current_t;
                earliest_u = current_u;
                earliest_v = current_v;
                eraliest_hitTri = j;
                earliest_iter = iter;
            }
        }
    }
    // if intersection was found, hit info will be updated
    if (earliest_iter != objects_end) {
        t = t_min;
        u = earliest_u;
        v = earliest_v;
        hitTri = eraliest_hitTri;
    }
    return earliest_iter;
}

#endif //UTILITES_H
