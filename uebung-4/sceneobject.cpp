//
// Created by danielr on 08.01.21.
//

#include "sceneobject.h"

unsigned int SceneObject::draw(RenderState &state, const QMatrix4x4* lightMatrix) {
    state.pushModelViewMatrix();
    state.getCurrentModelViewMatrix() *= modelMatrix;
    auto* f = state.getOpenGLFunctions();
    // TODO: Ex 4.1a Set uniforms with material parameters for OpenGL phong shader
    // TODO: Ex 4.2 Fix light matrix so it contains model transformation.

    auto result = mesh.draw(state);
    state.popModelViewMatrix();
    return result;
}

SceneObject::SceneObject(const Vec3f &ambientCol, const Vec3f &diffuseCol, const Vec3f &specularCol, float shini, float reflect, TriangleMesh &msh, const Vec3f& pos, const Vec3f& scale)
: ambientColor(ambientCol), diffuseColor(diffuseCol), specularColor(specularCol), shininess(shini), reflectionIntensity(reflect), mesh(msh) {
    translate(pos);
    this->scale(scale);
}

void SceneObject::scale(const Vec3f &scale) {
    modelMatrix.scale(scale.x(), scale.y(), scale.z());
}

void SceneObject::translate(const Vec3f &pos) {
    modelMatrix.translate(pos.x(), pos.y(), pos.z());
}

