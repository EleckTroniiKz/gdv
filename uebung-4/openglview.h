// ========================================================================= //
// Authors: Daniel Rutz, Daniel Ströter, Roman Getto, Matthias Bein          //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// Content: Widget for showing OpenGL scene, SOLUTION                        //
// ========================================================================= //

#ifndef OPENGLVIEW_H
#define OPENGLVIEW_H

#include <QByteArray>
#include <QTimer>
#include <QString>
#include <QElapsedTimer>
#include <QOpenGLFunctions_3_3_Core>
#include <QObject>
#include <QOpenGLWidget>
#include <QVector3D>

#include "trianglemesh.h"
#include "vec3.h"
#include "renderstate.h"
#include "sceneobject.h"

class OpenGLView : public QOpenGLWidget
{
    Q_OBJECT
public:
    OpenGLView(QWidget* parent = nullptr);

public slots:
    void setGridSize(int gridSize);
    void setDefaults();
    void refreshFpsCounter();
    void triggerLightMovement(bool shouldMove = true);
    void cameraMoves(float deltaX, float deltaY, float deltaZ);
    void cameraRotates(float deltaX, float deltaY);
    void changeShader(unsigned int index);
    void compileShader(const QString& vertexShaderPath, const QString& fragmentShaderPath);
    void triggerRaytracing(bool shouldRaytrace);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

signals:
    void fpsCountChanged(int newFps);
    void triangleCountChanged(unsigned int newTriangles);
    void shaderCompiled(unsigned int index);

private:
    QOpenGLFunctions_3_3_Core* f;

    // camera Information
    QVector3D cameraPos;
    QVector3D cameraDir;
    float angleX, angleY, movementSpeed;

    // mouse information
    QPoint mousePos;
    float mouseSensitivy;

    //rendered objects
    unsigned int objectsLastRun, trianglesLastRun;
    std::vector<TriangleMesh> meshes;
    std::vector<SceneObject> objects;
    TriangleMesh sphereMesh; // sun

    static GLuint csVAO, csVBOs[2];
    int gridSize;

    //light information
    float lightMotionSpeed;

    //FPS counter, needed for FPS calculation
    unsigned int frameCounter = 0;

    //timer for counting FPS
    QTimer fpsCounterTimer;

    //timer for counting delta time of a frame, needed for light movement
    QElapsedTimer deltaTimer;
    bool lightMoves = false;

    //shaders
    GLuint currentProgramID;
    std::vector<GLuint> programIDs;

    //RenderState with matrix stack
    RenderState state;

    GLuint genCSVAO();
    GLuint genRayTraceVAO();

    void drawCS();
    void drawLight();
    void moveLight();
    unsigned int getTriangleCount() const;

    //raytracing
    GLuint rayTracingProgramID;
    static GLuint rayTraceVAO, rayTraceVBOs[2];
    GLuint raytracedTextureID{};
    void raytrace();
    Vec3f traceRay(const Ray<float>& ray, int recursion_depth, unsigned int& intersectionTests);
    bool showRayTracing = false;

    //shadow mapping
    GLuint shadowMapTexture;
    GLuint shadowMapFramebuffer;
    GLuint shadowMapProgramID;
    QMatrix4x4 lightProjectionMatrix;
};

#endif // OPENGLVIEW_H
