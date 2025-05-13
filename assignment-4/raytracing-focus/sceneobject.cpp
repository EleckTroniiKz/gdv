//
// Created by danielr on 08.01.21.
//

#include "sceneobject.h"

unsigned int SceneObject::draw(RenderState &state, const QMatrix4x4* lightMatrix) {
    state.pushModelViewMatrix();
    state.getCurrentModelViewMatrix() *= modelMatrix;
    auto* f = state.getOpenGLFunctions();
    // TODO: Ex 4.1a Set uniforms with material parameters for OpenGL phong shader
    
    GLint ambientLocation = f->glGetUniformLocation(state.getCurrentProgram(), "material.ambientColor");
    float ambient[] = { ambientColor.x(), ambientColor.y(), ambientColor.z() };
    if (ambientLocation != -1) f->glUniform3fv(ambientLocation, 1, ambient);

    GLint diffuseLocation = f->glGetUniformLocation(state.getCurrentProgram(), "material.diffuseColor");
    float diffuse[] = { diffuseColor.x(), diffuseColor.y(), diffuseColor.z() };
    if (diffuseLocation != -1) f->glUniform3fv(diffuseLocation, 1, diffuse);

    GLint specularLocation = f->glGetUniformLocation(state.getCurrentProgram(), "material.specularColor");
    float specular[] = { specularColor.x(), specularColor.y(), specularColor.z() };
    if (specularLocation != -1) f->glUniform3fv(specularLocation, 1, specular);

    GLint shininessLocation = f->glGetUniformLocation(state.getCurrentProgram(), "material.shininess");
    if (shininessLocation != -1) f->glUniform1f(shininessLocation, shininess);

    // TODO: Ex 4.2 Fix light matrix so it contains model transformation.

    auto result = mesh.draw(state);
    state.popModelViewMatrix();
    return result;
}

SceneObject::SceneObject(const Vec3f &ambientCol, const Vec3f &diffuseCol, const Vec3f &specularCol, float shini, float reflect, TriangleMesh &msh, const Vec3f& pos, const Vec3f& scale, float transp, float refrIdx)
: ambientColor(ambientCol), diffuseColor(diffuseCol), specularColor(specularCol), shininess(shini), reflectionIntensity(reflect), mesh(msh), transparency(transp), refractiveIndex(refrIdx) {
    translate(pos);
    this->scale(scale);
}

void SceneObject::scale(const Vec3f &scale) {
    modelMatrix.scale(scale.x(), scale.y(), scale.z());
}

void SceneObject::translate(const Vec3f &pos) {
    modelMatrix.translate(pos.x(), pos.y(), pos.z());
}

