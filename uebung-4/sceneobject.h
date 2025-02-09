//
// Created by danielr on 08.01.21.
//

#ifndef UEBUNG_04_SCENEOBJECT_H
#define UEBUNG_04_SCENEOBJECT_H

#include <QMatrix4x4>
#include "vec3.h"
#include "trianglemesh.h"
#include "renderstate.h"

struct SceneObject {
    Vec3f ambientColor;
    Vec3f diffuseColor;
    Vec3f specularColor;
    float shininess;
    float reflectionIntensity;
    float transparency;
    float refractiveIndex;
    TriangleMesh& mesh;

    unsigned int draw(RenderState& state, const QMatrix4x4* lightMatrix = nullptr);
    void scale(const Vec3f& scale);
    void translate(const Vec3f& pos);
    const QMatrix4x4& getModelMatrix() const { return modelMatrix; };

    SceneObject(const Vec3f& ambientCol, const Vec3f& diffuseCol, const Vec3f& specularCol, float shini, float reflect, TriangleMesh& msh, const Vec3f& pos = Vec3f(0.f, 0.f, 0.f), const Vec3f& scale = Vec3f(1.f, 1.f, 1.f), float transp = 0.0f, float refrIdx = 1.0f);
private:
    QMatrix4x4 modelMatrix;
};

#endif //UEBUNG_04_SCENEOBJECT_H
