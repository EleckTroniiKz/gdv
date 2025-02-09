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

extern const GLfloat BoxVertices[];
extern const size_t BoxVerticesSize;
extern const GLuint BoxLineIndices[];
extern const size_t BoxLineIndicesSize;
extern const GLuint BoxTriangleIndices[];
extern const size_t BoxTriangleIndicesSize;

GLuint loadImageIntoTexture(QOpenGLFunctions_3_3_Core* f, const char* fileName, bool wrap = false);
GLuint loadCubeMap(QOpenGLFunctions_3_3_Core* f, const char* fileName[6]);

bool rayAABBIntersect(const Ray<float> &r, const Vec3f& vmin, const Vec3f& vmax, float t0, float t1);

inline QVector3D& Vec3fToQVector3D(Vec3f& vec) { return reinterpret_cast<QVector3D&>(vec); }
inline const QVector3D& Vec3fToQVector3D(const Vec3f& vec) { return reinterpret_cast<const QVector3D&>(vec); }
inline Vec3f& QVector3DToVec3f(QVector3D& vec) { return reinterpret_cast<Vec3f&>(vec); }
inline const Vec3f& QVector3DToVec3f(const QVector3D& vec) { return reinterpret_cast<const Vec3f&>(vec); }

/* TODO: Ex 4.1a This function is not implemented correctly. In order to make ray tracing work correctly, you have to make
 * sure that it actually finds the earliest intersection. */
template <typename InputIterator>
InputIterator intersectRayObjectsEarliest(InputIterator objects_begin, InputIterator objects_end, const Ray<float> &ray, float &t, float &u, float &v, unsigned int &hitTri, unsigned int& intersectionTests) {
    // iterate over all meshes
    for (InputIterator iter = objects_begin; iter != objects_end; iter++) {

        // optional: check ray versus bounding box first (t must have been initialized!)
        QVector3D boundingBoxMin(iter->mesh.getBoundingBoxMin().x(), iter->mesh.getBoundingBoxMin().y(), iter->mesh.getBoundingBoxMin().z());
        QVector3D boundingBoxMax(iter->mesh.getBoundingBoxMax().x(), iter->mesh.getBoundingBoxMax().y(), iter->mesh.getBoundingBoxMax().z());

        const QMatrix4x4& modelMatrix = iter->getModelMatrix();

        boundingBoxMin = modelMatrix.map(boundingBoxMin);
        boundingBoxMax = modelMatrix.map(boundingBoxMax);

        if (!rayAABBIntersect(ray, {boundingBoxMin.x(), boundingBoxMin.y(), boundingBoxMin.z()}, {boundingBoxMax.x(), boundingBoxMax.y(), boundingBoxMax.z()}, 0.0f, t)) continue;
        // get triangle information
        const std::vector<Vec3f>& vertices = iter->mesh.getVertices();
        const std::vector<Vec3ui>& triangles = iter->mesh.getTriangles();
        // brute force: iterate over all triangles of the mesh
        for (unsigned int j = 0; j < triangles.size(); j++) {
            QVector3D p0(Vec3fToQVector3D(vertices[triangles[j][0]]));
            QVector3D p1(Vec3fToQVector3D(vertices[triangles[j][1]]));
            QVector3D p2(Vec3fToQVector3D(vertices[triangles[j][2]]));


            p0 = modelMatrix.map(p0);
            p1 = modelMatrix.map(p1);
            p2 = modelMatrix.map(p2);

            bool hit = ray.triangleIntersect(QVector3DToVec3f(p0), QVector3DToVec3f(p1), QVector3DToVec3f(p2), u, v, t);
#pragma omp atomic
            intersectionTests++;

            if (hit && t > 0.0f) {
                hitTri = j;
                return iter;
            }
        }
    }
    return objects_end;
}

#endif //UTILITES_H
