//
// Created by danielr on 26.11.20.
//

#ifndef UEBUNG_03_RENDERSTATE_H
#define UEBUNG_03_RENDERSTATE_H

#include <stack>
#include <QMatrix3x3>
#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>

#include "vec3.h"
#include "light.h"

class RenderState {
    Light sceneLight;
    //Vec3f lightPos;
    GLuint activeProgram{}, standardProgram{};
    std::stack<QMatrix4x4> modelViewMatrixStack;
    std::stack<QMatrix4x4> projectionMatrixStack;
    QOpenGLFunctions_3_3_Core* f;
    GLint modelViewMatrixUniformStandard{-1}, projectionMatrixUniformStandard{-1}, normalMatrixUniformStandard{-1}, lightPositionUniformStandard{-1},
            cameraPositionUniformStandard{-1}, textureUniformStandard{-1}, normalMapUniformStandard{-1}, useTextureUniformStandard{-1}, ambientColorUniformStandard{-1},
            diffuseColorUniformStandard{-1}, specularColorUniformStandard{-1}, shininessUniformStandard{-1}, depthMapUniformStandard{-1}, lightMatrixUniformStandard{-1},
            lightIntensityUniformStandard{-1}, ambientIntensityUniformStandard{-1};
    GLint modelViewMatrixUniform{-1}, projectionMatrixUniform{-1}, normalMatrixUniform{-1}, lightPositionUniform{-1},
        cameraPositionUniform{-1}, textureUniform{-1}, normalMapUniform{-1}, useTextureUniform{-1}, ambientColorUniform{-1},
        diffuseColorUniform{-1}, specularColorUniform{-1}, shininessUniform{-1}, depthMapUniform{-1}, lightMatrixUniform{-1},
        lightIntensityUniform{-1}, ambientIntensityUniform{-1};

    static void loadIdentity(std::stack<QMatrix4x4>& stack) {
        if (!stack.empty()) {
            stack.top().setToIdentity();
        }
    }

public:
    explicit RenderState(QOpenGLFunctions_3_3_Core* f = nullptr) : f(f) {
        //Put the identity matrix on all stacks
        modelViewMatrixStack.emplace();
        projectionMatrixStack.emplace();
    }

    void setOpenGLFunctions(QOpenGLFunctions_3_3_Core* f) {
        this->f = f;
    }

    QOpenGLFunctions_3_3_Core* getOpenGLFunctions() {
        return f;
    }

    void loadIdentityModelViewMatrix() {
        loadIdentity(modelViewMatrixStack);
    }

    void loadIdentityProjectionMatrix() {
        loadIdentity(projectionMatrixStack);
    }

    void pushModelViewMatrix() {
        modelViewMatrixStack.push(modelViewMatrixStack.top());
    }

    void popModelViewMatrix() {
        if (modelViewMatrixStack.size() > 1)
            modelViewMatrixStack.pop();
        else loadIdentityModelViewMatrix();
    }

    void pushProjectionMatrix() {
        projectionMatrixStack.push(projectionMatrixStack.top());
    }

    void popProjectionMatrix() {
        if (projectionMatrixStack.size() > 1)
            projectionMatrixStack.pop();
        else loadIdentityProjectionMatrix();
    }

    QMatrix4x4& getCurrentProjectionMatrix() { return projectionMatrixStack.top(); }
    QMatrix4x4& getCurrentModelViewMatrix() { return modelViewMatrixStack.top(); }
    const QMatrix4x4& getCurrentModelViewMatrix() const { return modelViewMatrixStack.top(); }
    const QMatrix4x4& getCurrentProjectionMatrix() const { return projectionMatrixStack.top(); }
    QMatrix3x3 calculateNormalMatrix() const { return getCurrentModelViewMatrix().normalMatrix(); }
    GLuint getCurrentProgram() const { return activeProgram; }
    GLuint getStandardProgram() const { return standardProgram; }

    void setCurrentProgram(GLuint nextProgram) {
        f->glUseProgram(nextProgram);
        activeProgram = nextProgram;
        modelViewMatrixUniform = f->glGetUniformLocation(activeProgram, "modelView");
        projectionMatrixUniform = f->glGetUniformLocation(activeProgram, "projection");
        normalMatrixUniform = f->glGetUniformLocation(activeProgram, "normalMatrix");
        lightPositionUniform = f->glGetUniformLocation(activeProgram, "lightPosition");
        cameraPositionUniform = f->glGetUniformLocation(activeProgram, "cameraPosition");
        textureUniform = f->glGetUniformLocation(activeProgram, "diffuseTexture");
        normalMapUniform = f->glGetUniformLocation(activeProgram, "normalMap");
        useTextureUniform = f->glGetUniformLocation(activeProgram, "useTexture");
        ambientColorUniform = f->glGetUniformLocation(activeProgram, "ambientColor");
        diffuseColorUniform = f->glGetUniformLocation(activeProgram, "diffuseColor");
        specularColorUniform = f->glGetUniformLocation(activeProgram, "specularColor");
        shininessUniform = f->glGetUniformLocation(activeProgram, "shininess");
        depthMapUniform = f->glGetUniformLocation(activeProgram, "depthMap");
        lightMatrixUniform = f->glGetUniformLocation(activeProgram, "lightMatrix");
        lightIntensityUniform = f->glGetUniformLocation(activeProgram, "lightIntensity");
        ambientIntensityUniform = f->glGetUniformLocation(activeProgram, "ambientIntensity");
    }

    void setStandardProgram(GLuint standardProgram) {
        f->glUseProgram(standardProgram);
        activeProgram = standardProgram;
        this->standardProgram = standardProgram;

        modelViewMatrixUniformStandard = f->glGetUniformLocation(activeProgram, "modelView");
        projectionMatrixUniformStandard = f->glGetUniformLocation(activeProgram, "projection");
        normalMatrixUniformStandard = f->glGetUniformLocation(activeProgram, "normalMatrix");
        lightPositionUniformStandard = f->glGetUniformLocation(activeProgram, "lightPosition");
        cameraPositionUniformStandard = f->glGetUniformLocation(activeProgram, "cameraPosition");
        textureUniformStandard = f->glGetUniformLocation(activeProgram, "diffuseTexture");
        normalMapUniformStandard = f->glGetUniformLocation(activeProgram, "normalMap");
        useTextureUniformStandard = f->glGetUniformLocation(activeProgram, "useTexture");
        ambientColorUniformStandard = f->glGetUniformLocation(activeProgram, "ambientColor");
        diffuseColorUniformStandard = f->glGetUniformLocation(activeProgram, "diffuseColor");
        specularColorUniformStandard = f->glGetUniformLocation(activeProgram, "specularColor");
        shininessUniformStandard = f->glGetUniformLocation(activeProgram, "shininess");
        depthMapUniformStandard = f->glGetUniformLocation(activeProgram, "depthMap");
        lightMatrixUniformStandard = f->glGetUniformLocation(activeProgram, "lightMatrix");
        lightIntensityUniformStandard = f->glGetUniformLocation(activeProgram, "lightIntensity");
        ambientIntensityUniformStandard = f->glGetUniformLocation(activeProgram, "ambientIntensity");
    }

    void switchToStandardProgram() {
        f->glUseProgram(standardProgram);
        activeProgram = standardProgram;

        modelViewMatrixUniform = modelViewMatrixUniformStandard;
        projectionMatrixUniform = projectionMatrixUniformStandard;
        normalMatrixUniform = normalMatrixUniformStandard;
        lightPositionUniform = lightPositionUniformStandard;
        cameraPositionUniform = cameraPositionUniformStandard;
        textureUniform = textureUniformStandard;
        normalMapUniform = normalMapUniformStandard;
        useTextureUniform = useTextureUniformStandard;
        ambientColorUniform = ambientColorUniformStandard;
        diffuseColorUniform = diffuseColorUniformStandard;
        specularColorUniform = specularColorUniformStandard;
        shininessUniform = shininessUniformStandard;
        depthMapUniform = depthMapUniformStandard;
        lightMatrixUniform = lightMatrixUniformStandard;
        lightIntensityUniform = lightIntensityUniformStandard;
        ambientIntensityUniform = ambientIntensityUniformStandard;
    }

    GLint getModelViewUniform() const { return modelViewMatrixUniform; }
    GLint getProjectionUniform() const { return projectionMatrixUniform; }
    GLint getNormalMatrixUniform() const { return normalMatrixUniform; }
    GLint getLightPositionUniform() const { return lightPositionUniform; }
    GLint getCameraPositionUniform() const { return cameraPositionUniform; }
    GLint getTextureUniform() const { return textureUniform; }
    GLint getNormalMapUniform() const { return normalMapUniform; }
    GLint getUseTextureUniform() const { return useTextureUniform; }
    GLint getAmbientColorUniform() const { return ambientColorUniform; }
    GLint getDiffuseColorUniform() const { return diffuseColorUniform; }
    GLint getSpecularColorUniform() const { return specularColorUniform; }
    GLint getShininessUniform() const { return shininessUniform; }
    GLint getDepthMapUniform() const { return depthMapUniform; }
    GLint getLightMatrixUniform() const { return lightMatrixUniform; }
    GLint getLightIntensityUniform() const { return lightIntensityUniform; }
    GLint getAmbientIntensityUniform() const { return ambientIntensityUniform; }

    Light& getLight() {
        return sceneLight;
    }
    const Light& getLight() const {
        return sceneLight;
    }

    void setLightUniform() {
        const auto& pos = getLight().position;
        QVector4D Qlp4d(pos.x(), pos.y(), pos.z(), 1.0f);
        const QVector3D Qlp = getCurrentModelViewMatrix().map(Qlp4d).toVector3DAffine();
        f->glUniform3f(getLightPositionUniform(), Qlp.x(), Qlp.y(), Qlp.z());
        f->glUniform1f(getLightIntensityUniform(), getLight().lightIntensity);
        f->glUniform1f(getAmbientIntensityUniform(), getLight().ambientIntensity);
    }

    void setMatrices() {
        f->glUniformMatrix4fv(getProjectionUniform(), 1, GL_FALSE, getCurrentProjectionMatrix().constData());
        f->glUniformMatrix4fv(getModelViewUniform(), 1, GL_FALSE, getCurrentModelViewMatrix().constData());
        f->glUniformMatrix3fv(getNormalMatrixUniform(), 1, GL_FALSE, calculateNormalMatrix().constData());
    }
};

#endif //UEBUNG_03_RENDERSTATE_H
