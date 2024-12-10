// ========================================================================= //
// Authors: Daniel Rutz, Daniel Ströter, Roman Getto, Matthias Bein          //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// Content: Widget for showing OpenGL scene                                  //
// ========================================================================= //

#ifndef OPENGLVIEW_H
#define OPENGLVIEW_H

#include <QByteArray>
#include <QTimer>
#include <QString>
#include <QElapsedTimer>
#include <QOpenGLFunctions_2_1>
#include <QObject>
#include <QOpenGLWidget>

#include "trianglemesh.h"
#include "vec3.h"

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
    void togglePlanetSceneRender(bool shouldRenderPlanets = false);
    void cameraMoves(float deltaX, float deltaY, float deltaZ);
    void cameraRotates(float deltaX, float deltaY);
    void changeShader(unsigned int index);
    void changeRenderMode(unsigned int index);
    void compileShader(const QString& vertexShaderPath, const QString& fragmentShaderPath);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

signals:
    void fpsCountChanged(int newFps);
    void triangleCountChanged(unsigned int newTriangles);
    void shaderCompiled(unsigned int index);

private:
    //current render mode
    enum class RenderMode {
        IMMEDIATE = 0,
        ARRAY = 1,
        VBO = 2,
    } currentRenderMode;
    
    QOpenGLFunctions_2_1* f;

    // scene Information
    Vec3f centerPos;
    float angleX, angleY;

    //light information
    Vec3f lightPos;
    float lightMotionSpeed;

    //rendered objects
    TriangleMesh triMesh;
    TriangleMesh sphereMesh;

    //count of objects to render
    int gridSize;

    //FPS counter, needed for FPS calculation
    unsigned int frameCounter = 0;

    //timer for counting FPS
    QTimer fpsCounterTimer;

    //timer for counting delta time of a frame, needed for light movement
    QElapsedTimer deltaTimer;
    bool lightMoves = false;

    bool renderPlanetScene = false;

    //shaders
    std::vector<GLuint> programIDs;

    void drawCS();
    void drawLight();
    void moveLight();
    unsigned int getTriangleCount() const;
};

#endif // OPENGLVIEW_H
