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

#include <QTimer>
#include <QElapsedTimer>
#include <QOpenGLFunctions_2_1>
#include <QObject>
#include <vector>
#include <QOpenGLWidget>

#include "trianglemesh.h"
#include "vec3.h"

class OpenGLView : public QOpenGLWidget
{
    Q_OBJECT
public:
    OpenGLView(QWidget *parent = nullptr);

public slots:
    void setDefaults();
    void reInitializeObjects(int objectAmount = 1);
    void refreshFpsCounter();
    void recalcNormals(bool weightByAngle = false);
    void triggerLightMovement(bool shouldMove = true);
    void triggerNormalRendering(bool shouldRender = true);
    void triggerRenderOverClock();
    void textChanged(const QString &text);
    void cameraMoves(float deltaX, float deltaY, float deltaZ);
    void cameraRotates(float deltaX, float deltaY);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

signals:
    void fpsCountChanged(int newFps);
    void triangleCountChanged(int newTriangles);

private:
    QOpenGLFunctions_2_1 *f;

    // scene Information
    Vec3f centerPos;
    float angleX, angleY;

    // light information
    Vec3f lightPos;
    float lightMotionSpeed;

    // rendered objects
    TriangleMesh ballon;
    TriangleMesh sphere;
    
    std::vector<TriangleMesh> meshes; // To test how much we can render smoothly :)

    // FPS counter, needed for FPS calculation
    unsigned int frameCounter = 0;

    // timer for counting FPS
    QTimer fpsCounterTimer;

    // timer for counting delta time of a frame, needed for light movement
    QElapsedTimer deltaTimer;
    bool lightMoves = false;

    void drawCS();
    void drawLight();
    void moveLight();
    unsigned int getTriangleCount() const;
};

#endif // OPENGLVIEW_H
