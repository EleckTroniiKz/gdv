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
#include <vector>

#include <QtDebug>
#include <QMatrix4x4>
#include <QOpenGLVersionFunctionsFactory>

#include "shader.h"
#include "openglview.h"

OpenGLView::OpenGLView(QWidget* parent) : QOpenGLWidget(parent) {
    setDefaults();

    connect(&fpsCounterTimer, &QTimer::timeout, this, &OpenGLView::refreshFpsCounter);
    fpsCounterTimer.setInterval(1000);
    fpsCounterTimer.setSingleShot(false);
    fpsCounterTimer.start();
}

void OpenGLView::setGridSize(int gridSize)
{
    this->gridSize = gridSize;
    emit triangleCountChanged(getTriangleCount());
}

void OpenGLView::initializeSolarSystem() {
    char const* path = "../uebung-2/Modelle/sphere.off";

    // Setze GLFunctionPtr für SolarObjekte, weil es auch für die anderen Objhekte gemacht wurde.
    sun.setGLFunctionPtr(f);
    mercury.setGLFunctionPtr(f);
    venus.setGLFunctionPtr(f);
    earth.setGLFunctionPtr(f);
    mars.setGLFunctionPtr(f);
    moon.setGLFunctionPtr(f);

    sun.loadOFF(path);
    mercury.loadOFF(path);
    venus.loadOFF(path);
    earth.loadOFF(path);
    mars.loadOFF(path);
    moon.loadOFF(path);
}

void OpenGLView::initializeGL()
{
    //load OpenGL functions
    f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_2_1>(QOpenGLContext::currentContext());
    const GLubyte* versionString = f->glGetString(GL_VERSION);
    qDebug("The current OpenGL version is: %s\n", versionString);

    // set OpenGL ptrs for the TriangleMeshes
    sphereMesh.setGLFunctionPtr(f);
    triMesh.setGLFunctionPtr(f);

    //Load ballon mesh
    char const* filename = "../uebung-2/Modelle/ballon.off";
    triMesh.loadOFF(filename);

    //Load the sphere of the light
    sphereMesh.loadOFF("../uebung-2/Modelle/sphere.off");
    initializeSolarSystem();

    // black screen
    f->glClearColor(0.f,0.f,0.f,1.f);
    // enable depth buffer
    f->glEnable(GL_DEPTH_TEST);
    // set shading model
    f->glShadeModel(GL_SMOOTH);
    // set lighting and material
    GLfloat global_ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat ambientLight[]   = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat diffuseLight[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat specularLight[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
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
    GLfloat specularLightMaterial[] =  { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat shininessMaterial = 128.0f;
    f->glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininessMaterial);
    f->glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularLightMaterial);
}

void OpenGLView::resizeGL(int w, int h)
{
    //Calculate new projection matrix
    QMatrix4x4 projectionMatrix;
    const float aspectRatio = static_cast<float>(w) / static_cast<float>(h);
    projectionMatrix.perspective(65.f, aspectRatio, 0.1f, 100.f);

    //Resize viewport
    f->glViewport(0, 0, w, h);

    //Set projection matrix
    f->glMatrixMode(GL_PROJECTION);
    f->glLoadIdentity();
    f->glMultMatrixf(projectionMatrix.constData());
    f->glMatrixMode(GL_MODELVIEW);
    f->glLoadIdentity();
    
    update();
}

void OpenGLView::paintGridObject()
{
    
    drawCS();

    if (lightMoves) {
        moveLight();
    }

    // draw sphere for light still without lighting
    drawLight();
    
    // scale scene to fixed box size
    float scale = 5.0f / std::max(1, gridSize);
    f->glScalef(scale, scale, scale);

    // draw objects
    f->glEnable(GL_LIGHTING);
    f->glColor3f(1.f,0.1f,0.1f);
    for (int i = -gridSize; i <= gridSize; ++i) {
        for (int j = -gridSize; j <= gridSize; ++j) {
            if (i != 0 || j != 0) {
                float r = (float) i / (2.0f * gridSize) + 0.5f;
                float g = (float) j / (2.0f * gridSize) + 0.5f;
                float b = 1.0f - 0.5f * r - 0.5f * g;
                f->glColor3f(r, g, b);
            } else f->glColor3f(1, 1, 1);
            f->glPushMatrix();
            f->glTranslatef(4.0f * i, 0.0f, 4.0f * j);
            switch (currentRenderMode) {
                case RenderMode::ARRAY:
                    triMesh.drawArray();
                    break;
                case RenderMode::VBO:
                    triMesh.drawVBO();
                    break;
                default:
                    triMesh.drawImmediate();
                    break;
            }
            f->glPopMatrix();
        }
    }
}

float calculateDT() {
    /*
    Berechne delta-t, um so die Planeten im Solarsystem zu bewegen. 
    Methode prüft wie lange es her ist, dass diese Methode ausgeführt wurde. 
    Zum nachlesen: https://en.cppreference.com/w/cpp/chrono/steady_clock
    */
    static std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = now - lastTime;
    lastTime = now;
    return elapsed.count(); 
}

void OpenGLView::setupSunAndLight() {
    // Set lighting and background
    f->glEnable(GL_LIGHTING);
    f->glEnable(GL_LIGHT0);  // Enable the first light
    f->glEnable(GL_NORMALIZE);

    // Set ambient, diffuse, and specular components for the light
    GLfloat ambient[] = { 0.05f, 0.05f, 0.05f, 1.0f };
    GLfloat diffuse[] = { 0.3f, 0.3f, 0.0f, 1.0f };
    GLfloat specular[] = { 0.05f, 0.05f, 0.05f, 1.0f };

    // Apply light properties
    f->glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    f->glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    f->glLightfv(GL_LIGHT0, GL_SPECULAR, specular);

    // Position the light at the origin (assuming the Sun is at the origin)
    GLfloat lightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };  // W component is 1 for positional light
    f->glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    GLfloat emissiveMaterial[] = { 0.65f, 0.65f, 0.0f, 1.0f };

    // Draw the Sun as a bright sphere at the light's position
    f->glPushMatrix();
    f->glTranslatef(0.0f, 0.0f, 0.0f); // Translate to the origin
    f->glScalef(2.0f, 2.0f, 2.0f);     // Scale up the Sun for visual prominence
    f->glMaterialfv(GL_FRONT, GL_EMISSION, emissiveMaterial);
    f->glColor3f(1.0f, 1.0f, 0.0f);    // Color the Sun yellow
    sun.drawVBO();        
    f->glPopMatrix();

    GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    f->glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
}

void OpenGLView::paintSolarSystem() 
{
    setupSunAndLight();
    drawCS();

    // Sonnensystemkonstanten
    const float distances[] = {6.0f, 9.0f, 12.0f, 16.0f, 3.0f}; // Distanzen der Objekte zur Sonne (außer letzter Eintrag. Das ist die Distanz zwischen Mond und Erde)
    TriangleMesh* planets[] = {&mercury, &venus, &earth, &mars}; // Um die schleife später schöner zu machen speichern wir Referenzen zu den Variablen in eine Liste
    const float scaleFactors[] = {0.38f, 0.95f, 1.0f, 0.53f, 0.27f}; // SKalierungswerte der Celestial Objekte
    const float orbitalPeriods[] = { 88.0f, 224.78f, 365.25f, 687.0f };
    GLfloat colors[][3] = {
        {0.5f, 0.5f, 0.5f}, // mercury farbe
        {1.0f, 0.8f, 0.6f}, // venus farbe
        {0.0f, 0.0f, 1.0f}, // earth farbe
        {1.0f, 0.0f, 0.0f}, // mars farbe
        {0.6f, 0.7f, 0.6f} // moon farbe
    };
      
    float timeFactor = 1.0f; // Orbit geschwindigkeit

    // Beleuchtungskonstanten Mond
    GLfloat ambientLight[] = { 0.05f, 0.05f, 0.05f, 1.0f }; 
    GLfloat diffuseLight[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat specularLight[] = { 0.05f, 0.05f, 0.05f, 1.0f };

    f->glEnable(GL_LIGHT1);
    f->glEnable(GL_LIGHTING);

    for (int i = 0; i < 4; ++i) {
       /*
        Keplers Law aber vereinfacht, sodass die Bewegung streng circular ist statt spherical. 
       */
        float angle = 2 * M_PI * (t / orbitalPeriods[i] * 10.0f);
        float x = cos(angle) * distances[i];
        float z = sin(angle) * distances[i];

        f->glColor3fv(colors[i]); // setze Farve vom aktuellen planeten
        f->glPushMatrix(); // aktuelle Transformation in MAtrix Stack speichern. 
        f->glTranslatef(x, 0.0f, z); 
        f->glScalef(scaleFactors[i], scaleFactors[i], scaleFactors[i]); // Planeten herunterskalieren, sodass alle in eine VIew passen
        planets[i]->drawVBO(); // Rendere Planet (VBO Mode wurde gewählt, weil dieser sehr viel schnelelr ist als immediate und Array)

        // Male den Mooond!
        if (planets[i] == &earth) {

            float moonAngle = t * timeFactor; // Winkel wird basierend auf Zeit berechnet.
            float moonX = cos(moonAngle) * distances[4];
            float moonZ = sin(moonAngle) * distances[4];
            GLfloat moonLightPos[] = { moonX, 0.0f, moonZ, 1.0f}; // Aktuelle Mond Position definieren, um diese für das Licht zu targeten.

            f->glLightfv(GL_LIGHT1, GL_POSITION, moonLightPos); // Lichtposition setzen für Mond
            f->glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight); // indirektes Licht 
            f->glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight); // directes Licht
            f->glLightfv(GL_LIGHT1, GL_SPECULAR, specularLight); // Reflektiertes Licht


            // Material definieren sodass der Mond etwas "glowed" und wirklich so wirkt als wäre es eine Lichtquelle
            GLfloat moonMaterial[] = { 0.65f, 0.65f, 0.65f, 1.0f };
            f->glMaterialfv(GL_FRONT, GL_EMISSION, moonMaterial);

            // Selber prozess wie oben
            f->glPushMatrix();
            f->glTranslatef(moonX, 0.0f, moonZ);
            f->glScalef(scaleFactors[4], scaleFactors[4], scaleFactors[4]);
            f->glColor3fv(colors[4]);
            moon.drawVBO();
            f->glPopMatrix();

            // Material resetten, sonst werden die OBjekte die danach gemalt werden auch glowen.
            GLfloat noEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            f->glMaterialfv(GL_FRONT, GL_EMISSION, noEmission);
        }

        f->glPopMatrix(); // Vorherigen Transformationsstand wiederherstellen, sodass vorherige Transformationen nicht "überschrieben"/reverted werden.
    }
}

void OpenGLView::paintGL()
{
    performanceTimer.start(); // Starte Time um zu messen wie lange es dauert, ein Frame zu "malen"

    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Farb buffer und Tiefenbuffer clearen. Verhindert visuelle Artifakte weil Screen "resetteed"  wird
    f->glLoadIdentity(); // aktuelle Matrix wird zur Identity Matrix resetted, sodass keine vorherigen Transformationen noch beeinflussen.
    f->glEnable(GL_NORMALIZE); // Zusatz von Moodle: Macht Beleuchtung schöner.
    f->glTranslatef(centerPos.x(), centerPos.y(), centerPos.z()); // Szene Zentrieren
    f->glDisable(GL_LIGHTING); // VBeleuchtung ausschalten (fü+r jetzt)
    f->glRotatef(angleX, 0.0f, 1.0f, 0.0f);
    f->glRotatef(angleY, 1.0f, 0.0f, 0.0f);

    if(renderPlanetScene){
        paintSolarSystem(); // Rendere das Solarsystem (Aufg. 5)
        t += calculateDT();
    }
    else{
        paintGridObject(); // Rendere die Ansicht mit den Heißluftballons im Grid
    }

    qint64 elapsedTime = performanceTimer.elapsed(); // Messe wie lange es gedauert hat einen Frame zu malen -> Gibt in Millisekunden die Performance an. Zum Prüfen Gitter Wert erhöhen bei Luftbalons.
    
    timeToDraw = elapsedTime; // Speichere REnder-Time in MS für die Ausgabe
    ++frameCounter;
    update();
}

void OpenGLView::drawCS()
{
    // Draw coordinate system

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
    f->glColor3f(1,1,0);
    sphereMesh.drawImmediate();
    f->glPopMatrix();
}

void OpenGLView::moveLight()
{
    lightPos.rotY(lightMotionSpeed * (deltaTimer.restart() / 1000.f));
}

unsigned int OpenGLView::getTriangleCount() const
{
    return gridSize * gridSize * triMesh.getTriangles().size() + sphereMesh.getTriangles().size();
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
    // Senden hier auch die Performance Metrics mit, weil es aus irgendeinem Grund nicht geklappt hat mit neuem Signal
    emit fpsCountChanged(frameCounter, timeToDraw);
    frameCounter = 0;
}

void OpenGLView::togglePlanetSceneRender(bool shouldRenderPlanets)
{
    renderPlanetScene = shouldRenderPlanets;
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

void OpenGLView::changeShader(unsigned int index) {
    makeCurrent();
    switch (index) {
        case 0:
            f->glUseProgram(0);
            f->glShadeModel(GL_SMOOTH);
            break;
        case 1:
            f->glUseProgram(0);
            f->glShadeModel(GL_FLAT);
            break;
        default:
            index -= 2;
            try {
                f->glUseProgram(programIDs.at(index));
            } catch (std::out_of_range& ex) {
                qFatal("Tried to access shader index that has not been loaded! %s", ex.what());
            }
    }
    doneCurrent();
}

void OpenGLView::changeRenderMode(unsigned int index) {
    switch (index) {
        default:
            qCritical("Tried to set rendermode to unknown index! Assuming immediate mode...");
            // [[fallthrough]];
        case 0:
            currentRenderMode = RenderMode::IMMEDIATE;
            break;
        case 1:
            currentRenderMode = RenderMode::ARRAY;
            break;
        case 2:
            currentRenderMode = RenderMode::VBO;
            break;
    }
}

void OpenGLView::compileShader(const QString& vertexShaderPath, const QString& fragmentShaderPath) {
    GLuint programHandle = readShaders(f, vertexShaderPath, fragmentShaderPath);
    if (programHandle) {
        programIDs.push_back(programHandle);
        emit shaderCompiled(programIDs.size() - 1);
    }
}
