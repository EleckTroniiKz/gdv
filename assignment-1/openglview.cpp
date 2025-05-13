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

#include <cmath>
#include <iostream>
#include <string>

#include <QtDebug>
#include <QMatrix4x4>
#include <QOpenGLVersionFunctionsFactory>

#include "openglview.h"
#include "globals.h"

OpenGLView::OpenGLView(QWidget *parent) : QOpenGLWidget(parent)
{
    setDefaults();
    
    reInitializeObjects(1);


    connect(&fpsCounterTimer, &QTimer::timeout, this, &OpenGLView::refreshFpsCounter);
    fpsCounterTimer.setInterval(1000);
    fpsCounterTimer.setSingleShot(false);
    fpsCounterTimer.start();
}

void OpenGLView::reInitializeObjects(int objectAmount) {
    for (int i = 0; i < objectAmount; i++) {
        TriangleMesh temp;
        temp.loadOFF("./Modelle/ballon.off");
        meshes.push_back(temp);
    }
}

void OpenGLView::initializeGL()
{
    // load OpenGL functions
    f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_2_1>(QOpenGLContext::currentContext());
    const GLubyte *versionString = f->glGetString(GL_VERSION);
    qDebug("The current OpenGL version is: %s\n", versionString);

    // black screen
    f->glClearColor(0.f, 0.f, 0.f, 0.f);
    // enable depth buffer
    f->glEnable(GL_DEPTH_TEST);
    // set shading model
    f->glShadeModel(GL_SMOOTH);
    // set lighting and material
    GLfloat global_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat ambientLight[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat diffuseLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininess = 128.0f;
    f->glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);
    f->glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    f->glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    f->glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    f->glEnable(GL_LIGHT0);
    // enable use of glColor instead of glMaterial for ambient and diffuse property
    f->glEnable(GL_COLOR_MATERIAL);
    f->glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    // white shiny specular highlights
    GLfloat specularLightMaterial[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininessMaterial = 128.0f;
    f->glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininessMaterial);
    f->glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularLightMaterial);
}

void OpenGLView::resizeGL(int w, int h)
{
    // Calculate new projection matrix
    QMatrix4x4 projectionMatrix;
    const float aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    projectionMatrix.perspective(65.f, aspectRatio, 0.1f, 100.f);

    // Resize viewport
    f->glViewport(0, 0, w, h);

    // Set projection matrix
    f->glMatrixMode(GL_PROJECTION);
    f->glLoadIdentity();
    f->glMultMatrixf(projectionMatrix.constData());
    f->glMatrixMode(GL_MODELVIEW);
    f->glLoadIdentity();

    update();
}

void OpenGLView::paintGL()
{
    // clear and set camera
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    f->glLoadIdentity();

    // translate to centerPos
    f->glTranslatef(centerPos.x(), centerPos.y(), centerPos.z());

    // disable lighting for coordinate system and light sphere
    f->glDisable(GL_LIGHTING);

    // rotate scene, then render cs
    f->glRotatef(angleX, 0.0f, 1.0f, 0.0f);
    f->glRotatef(angleY, 1.0f, 0.0f, 0.0f);
    drawCS();

    if (lightMoves) {
        moveLight();
    }

    // draw sphere for light still without lighting
    drawLight();

    // draw object
    f->glEnable(GL_LIGHTING);
    f->glColor3f(1.f, 0.1f, 0.1f);
    f->glPushMatrix();
    f->glTranslatef(1.0f, 1.0f, 1.0f);

    for (int i = 0; i < meshes.size(); i++) {
        meshes[i].draw(f);
        meshes[i].drawNormals(f);
    }

    sphere.draw(f);
    f->glPopMatrix();
    ++frameCounter;
    update();

    // Emit the triangle count to be shown in the UI.
    emit triangleCountChanged(getTriangleCount());
}

void OpenGLView::drawCS()
{
    f->glBegin(GL_LINES);
    // red X
    f->glColor3f(1.f, 0.f, 0.f);
    f->glVertex3f(0.f, 0.f, 0.f);
    f->glVertex3f(5.f, 0.f, 0.f);
    // green Y
    f->glColor3f(0.f, 1.f, 0.f);
    f->glVertex3f(0.f, 0.f, 0.f);
    f->glVertex3f(0.f, 5.f, 0.f);
    // blue Z
    f->glColor3f(0.f, 0.f, 1.f);
    f->glVertex3f(0.f, 0.f, 0.f);
    f->glVertex3f(0.f, 0.f, 5.f);
    f->glEnd();
}

void OpenGLView::drawLight()
{
    // set light position in within current coordinate system
    GLfloat lp[] = { lightPos.x(), lightPos.y(), lightPos.z(), 1.0f };
    f->glLightfv(GL_LIGHT0, GL_POSITION, lp);

    // draw yellow sphere for light source
    f->glPushMatrix();
    f->glTranslatef(lp[0], lp[1], lp[2]);
    f->glScalef(0.3f, 0.3f, 0.3f);
    f->glColor3f(1, 1, 0);
    sphere.draw(f);
    f->glPopMatrix();
}

void OpenGLView::moveLight()
{
    lightPos.rotY(lightMotionSpeed * (deltaTimer.restart() / 1000.f));
}

unsigned int OpenGLView::getTriangleCount() const
{
    // TODO: Needs to be updated for multiple rendered meshes.

    int totalCount = sphere.getTriangles().size();
    for (int i = 0; i < meshes.size(); i++) {
        totalCount += meshes[i].getTriangles().size();
    }
    return totalCount;
}

void OpenGLView::setDefaults()
{
    // scene Information
    centerPos = Vec3f(1.0f, -2.0f, -5.0f);
    angleX = 0.0f;
    angleY = 0.0f;
    // light information
    lightPos = Vec3f(-10.0f, 0.0f, 0.0f);
    lightMotionSpeed = 80.0f;
}

void OpenGLView::refreshFpsCounter()
{
    emit fpsCountChanged(frameCounter);
    frameCounter = 0;
}

void OpenGLView::recalcNormals(bool weightByAngle)
{
    for (int i = 0; i < meshes.size(); i++) {
        meshes[i].calculateNormals(weightByAngle);
    }
    
    update();
}

void OpenGLView::textChanged(const QString &text) {
    renderCount = text.toStdString();
}

void OpenGLView::triggerNormalRendering(bool shouldRender) {
    renderNormals = shouldRender;
}

void OpenGLView::triggerRenderOverClock() {
    meshes.clear();
    reInitializeObjects(std::stoi(renderCount));
}

void OpenGLView::triggerLightMovement(bool shouldMove)
{
    lightMoves = shouldMove;
    if (lightMoves) {
        if (deltaTimer.isValid()) {
            deltaTimer.restart();
        } else {
            deltaTimer.start();
        }
    }
}

void OpenGLView::cameraMoves(float deltaX, float deltaY, float deltaZ)
{
    centerPos[0] += deltaX;
    centerPos[1] += deltaY;
    centerPos[2] += deltaZ;

    update();
}

void OpenGLView::cameraRotates(float deltaX, float deltaY)
{
    angleX = std::fmod(angleX + deltaX, 360.f);
    angleY += deltaY;
}
