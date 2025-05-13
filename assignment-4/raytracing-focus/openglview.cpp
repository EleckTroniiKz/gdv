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

#include <cmath>

#include <QtDebug>
#include <QMatrix4x4>
#include <QOpenGLVersionFunctionsFactory>

#include "shader.h"
#include "openglview.h"

// These variables are missing on MacOS -> This should fix them.
#ifndef GL_MAX_FRAMEBUFFER_WIDTH
#define GL_MAX_FRAMEBUFFER_WIDTH          0x9315
#endif

#ifndef GL_MAX_FRAMEBUFFER_HEIGHT
#define GL_MAX_FRAMEBUFFER_HEIGHT         0x9316
#endif

const unsigned int SHADOW_MAP_SIZE = 2048;

GLuint OpenGLView::csVAO = 0;
GLuint OpenGLView::csVBOs[2] = {0, 0};

GLuint OpenGLView::rayTraceVAO = 0;
GLuint OpenGLView::rayTraceVBOs[2] = {0, 0};

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

void OpenGLView::initializeGL()
{
    f = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(QOpenGLContext::currentContext());
    const GLubyte* versionString = f->glGetString(GL_VERSION);
    std::cout << "The current OpenGL version is: " << versionString << std::endl;
    state.setOpenGLFunctions(f);

    //black screen
    f->glClearColor(0.f, 0.f, 0.f, 1.f);
    f->glEnable(GL_DEPTH_TEST);
    GLint result;
    f->glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &result);
    std::cout << "Maximal Framebuffer Height: "<< result << std::endl;
    f->glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &result);
    std::cout << "Maximal Framebuffer Height: "<< result << std::endl;

    //Load the sphere of the light
    sphereMesh.setGLFunctionPtr(f);
    sphereMesh.loadOFF("../uebung-4/Models/sphere.off");

    //load meshes
    meshes.emplace_back(f);
    meshes[0].loadOFF("../uebung-4/Models/doppeldecker.off");
    meshes.emplace_back(f);
    meshes[1].loadOFF("../uebung-4/Models/cube.off");

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f), 
        Vec3f(0.6f, 0.3f, 0.3f), 
        Vec3f(0.4f, 0.4f, 0.4f), 
        100.0f, 0.2f, 
        meshes[0],
        Vec3f(-4.0f, 0.0f, 0.0f),
        Vec3(1.0f, 1.0f, 1.0f), 
        0.0f, 1.5f);

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f),
        Vec3f(0.6f, 0.3f, 0.3f),
        Vec3f(0.4f, 0.4f, 0.4f),
        100.f, 0.2f,
        meshes[0]);

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f),
        Vec3f(0.6f, 0.3f, 0.3f),
        Vec3f(0.4f, 0.4f, 0.4f),
        100.f, 0.1f,
        meshes[0],
        Vec3f(2.0f, 0.0f, 0.0f));

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f),
        Vec3f(0.6f, 0.3f, 0.3f),
        Vec3f(0.4f, 0.4f, 0.4f),
        100.f, 0.4f,
        meshes[0],
        Vec3f(-2.0f, 0.0f, 0.0f));

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f),
        Vec3f(0.6f, 0.3f, 0.3f),
        Vec3f(0.4f, 0.4f, 0.4f),
        100.f, 0.3f,
        meshes[1],
        Vec3f(0.0f, -5.0f, 0.0f),
        Vec3f(10.f, 0.2f, 10.f));

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f),
        Vec3f(0.6f, 0.3f, 0.3f),
        Vec3f(0.4f, 0.4f, 0.4f),
        100.f, 0.f,
        meshes[1],
        Vec3f(0.0f, 0.0f, 10.0f),
        Vec3f(10.f, 10.0f, 0.2f));

    objects.emplace_back(Vec3f(0.2f, 0.1f, 0.1f),
        Vec3f(0.6f, 0.3f, 0.3f),
        Vec3f(0.4f, 0.4f, 0.4f),
        100.f, 0.1f,
        meshes[1],
        Vec3f(0.0f, -4.0f, 2.0f));

    //load coordinate system
    csVAO = genCSVAO();
    //load raytracing face
    rayTraceVAO = genRayTraceVAO();

    //load shaders
    GLuint lightShaderID = readShaders(f, "../uebung-4/Shader/only_mvp.vert", "../uebung-4/Shader/constant_color.frag");
    programIDs.push_back(lightShaderID);
    state.setStandardProgram(lightShaderID);
    currentProgramID = lightShaderID;
    // TODO: Ex 4.1a Implement shader for Phong lighting

    // Load Shaders 
    GLuint phongShaderID = readShaders(f, "../uebung-4/Shader/phong.vert", "../uebung-4/Shader/phong.frag");
    // Check if shaders were coimpiled nicely
    if (phongShaderID != 0) {
        // add shader program id to list of avaible shader probrams
        programIDs.push_back(phongShaderID);
        // set current programId to phong shader
        currentProgramID = phongShaderID;
    }
    // use phong shader for rendering (if it was successfuly compiled)
    state.setCurrentProgram(currentProgramID);

    // TODO: Ex 4.2b Implement shader for Blinn-Phong lighting using shadow map
    for (const auto& i : { std::pair<const char*, const char*>
            {"../uebung-4/Shader/only_mvp.vert", "../uebung-4/Shader/blinn_phong_shadow.frag"},
            {"../uebung-4/Shader/only_mvp.vert", "../uebung-4/Shader/phong.frag"},
        }) {
        GLuint shaderID = readShaders(f, i.first, i.second);
        if (shaderID != 0) programIDs.push_back(shaderID);
    }

    // TODO: Ex 4.2a Implement shader for calculation of shadow map

    // TODO: Ex 4.2a Generate shadow map texture and framebuffer, generate light projection matrix

    state.setCurrentProgram(currentProgramID);

    for (std::size_t i = 0; i < programIDs.size(); ++i) emit shaderCompiled(i);
}

void OpenGLView::resizeGL(int width, int height) {
    //Calculate new projection matrix
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    state.loadIdentityProjectionMatrix();
    state.getCurrentProjectionMatrix().perspective(65.f, aspectRatio, 0.5f, 10000.f);

    //set projection matrix in OpenGL shader
    state.switchToStandardProgram();
    f->glUniformMatrix4fv(state.getProjectionUniform(), 1, GL_FALSE, state.getCurrentProjectionMatrix().constData());
    for (GLuint progID : programIDs) {
        state.setCurrentProgram(progID);
        f->glUniformMatrix4fv(state.getProjectionUniform(), 1, GL_FALSE, state.getCurrentProjectionMatrix().constData());
    }

    //Resize viewport
    f->glViewport(0, 0, width, height);
}

void OpenGLView::paintGL() {
    f->glClearDepth(1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    state.loadIdentityModelViewMatrix();
    if (showRayTracing) {
        state.setCurrentProgram(rayTracingProgramID);
        f->glBindVertexArray(rayTraceVAO);
        f->glActiveTexture(GL_TEXTURE0);
        f->glBindTexture(GL_TEXTURE_2D, raytracedTextureID);
        f->glUniform1i(state.getTextureUniform(), 0);

        f->glDrawArrays(GL_TRIANGLE_FAN, 0, 5);
        f->glBindVertexArray(0);
        f->glBindTexture(GL_TEXTURE_2D, 0);
    } else {
        static const QVector3D upVector(0.0f, 1.0f, 0.0f);
        if (lightMoves) moveLight();

        //TODO: Ex 4.2a Render depth map

        //translate to center, rotate and render coordinate system and light sphere
        QVector3D cameraLookAt = cameraPos + cameraDir;

        state.getCurrentModelViewMatrix().lookAt(cameraPos, cameraLookAt, upVector);
        state.switchToStandardProgram();
        drawCS();

        drawLight();

        state.setCurrentProgram(currentProgramID);
        state.setLightUniform();

        //TODO: Ex 4.2b Bind depth map texture

        // draw objects. count triangles and objects drawn.
        unsigned int triangles, trianglesDrawn = 0, objectsDrawn = 0;
        for (auto& object : objects) {
            trianglesDrawn += object.draw(state);
        }
        // cout number of objects and triangles if different from last run
        if (trianglesDrawn != trianglesLastRun) {
            trianglesLastRun = trianglesDrawn;
            emit triangleCountChanged(trianglesDrawn);
        }
        frameCounter++;
        f->glBindTexture(GL_TEXTURE_2D, GL_NONE);
    }
    GLenum error;
    while ((error = f->glGetError()) != GL_NO_ERROR) {
        std::cout << "Error: " << error << std::endl;
    }
    update();
}

void OpenGLView::drawCS() {
    f->glUniformMatrix4fv(state.getModelViewUniform(), 1, GL_FALSE, state.getCurrentModelViewMatrix().constData());
    f->glBindVertexArray(csVAO);
    f->glDrawArrays(GL_LINES, 0, 6);
    f->glBindVertexArray(GL_NONE);
}

void OpenGLView::drawLight() {
    // draw yellow sphere for light source
    state.pushModelViewMatrix();
    Vec3f& lp = state.getLight().position;
    state.getCurrentModelViewMatrix().translate(lp.x(), lp.y(), lp.z());
    sphereMesh.draw(state);
    state.popModelViewMatrix();
}

void OpenGLView::moveLight()
{
    state.getLight().position.rotY(lightMotionSpeed * (deltaTimer.restart() / 1000.f));
}

unsigned int OpenGLView::getTriangleCount() const
{
    size_t result = 0;

    return result;
}

void OpenGLView::setDefaults() {
    // scene Information
    cameraPos = QVector3D(0.0f, 0.0f, -3.0f);
    cameraDir = QVector3D(0.f, 0.f, -1.f);
    movementSpeed = 0.02f;
    angleX = 0.0f;
    angleY = 0.0f;
    // light information
    Light defaultLight;
    defaultLight.position = Vec3f(0.0f, 5.0f, 7.0f);
    defaultLight.lightIntensity = 1.f;
    defaultLight.ambientIntensity = 0.4f;
    state.getLight() = defaultLight;
    lightMotionSpeed = 10.f;
    // mouse information
    mouseSensitivy = 1.0f;

    gridSize = 3;
    // last run: 0 objects and 0 triangles
    objectsLastRun = 0;
    trianglesLastRun = 0;
}

void OpenGLView::refreshFpsCounter()
{
    emit fpsCountChanged(frameCounter);
    frameCounter = 0;
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
    QVector3D ortho(-cameraDir.z(),0.0f,cameraDir.x());
    QVector3D up = QVector3D::crossProduct(cameraDir, ortho).normalized();

    cameraPos += deltaX * ortho;
    cameraPos += deltaY * up;
    cameraPos += deltaZ * cameraDir;

    update();
}

void OpenGLView::cameraRotates(float deltaX, float deltaY)
{
    angleX = std::fmod(angleX + deltaX, 360.f);
    angleY += deltaY;
    angleY = std::max(-70.f, std::min(angleY, 70.f));

    cameraDir.setX(std::sin(angleX * M_RadToDeg) * std::cos(angleY * M_RadToDeg));
    cameraDir.setZ(-std::cos(angleX * M_RadToDeg) * std::cos(angleY * M_RadToDeg));
    cameraDir.setY(std::max(0.0f, std::min(std::sqrt(1.0f - cameraDir.x() * cameraDir.x() - cameraDir.z() * cameraDir.z()), 1.0f)));

    if (angleY < 0.f) cameraDir.setY(-cameraDir.y());

    update();
}

void OpenGLView::changeShader(unsigned int index) {
    makeCurrent();
    try {
        GLuint progID = programIDs.at(index);
        currentProgramID = progID;
    } catch (std::out_of_range& ex) {
        qFatal("Tried to access shader index that has not been loaded! %s", ex.what());
    }
    doneCurrent();
}

void OpenGLView::compileShader(const QString& vertexShaderPath, const QString& fragmentShaderPath) {
    GLuint programHandle = readShaders(f, vertexShaderPath, fragmentShaderPath);
    if (programHandle) {
        programIDs.push_back(programHandle);
        emit shaderCompiled(programIDs.size() - 1);
    }
}

void OpenGLView::triggerRaytracing(bool shouldRaytrace)
{
    if (!shouldRaytrace) {
        showRayTracing = false;
    } else {
        raytrace();
        showRayTracing = true;
    }
}

// This creates a VAO that represents the coordinate system
GLuint OpenGLView::genCSVAO() {
    GLuint VAOresult;
    f->glGenVertexArrays(1, &VAOresult);
    f->glGenBuffers(2, csVBOs);

    f->glBindVertexArray(VAOresult);
    f->glBindBuffer(GL_ARRAY_BUFFER, csVBOs[0]);
    const static float vertices[] = {
            0.f, 0.f, 0.f,
            5.f, 0.f, 0.f,
            0.f, 0.f, 0.f,
            0.f, 5.f, 0.f,
            0.f, 0.f, 0.f,
            0.f, 0.f, 5.f,
    };
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    f->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(POSITION_LOCATION);
    f->glBindBuffer(GL_ARRAY_BUFFER, csVBOs[1]);
    const static float colors[] = {
            1.f, 0.f, 0.f,
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 1.f, 0.f,
            0.f, 0.f, 1.f,
            0.f, 0.f, 1.f,
    };
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    f->glVertexAttribPointer(COLOR_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(COLOR_LOCATION);
    f->glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
    f->glBindVertexArray(GL_NONE);
    return VAOresult;
}

Vec3f OpenGLView::traceRay(const Ray<float>& ray, int recursion_depth, unsigned int& intersectionTests) {
    // 1. Termination condition: If depth is zero, then stop shooting and tracing rays
    if (recursion_depth <= 0) {
        return Vec3f(0.0f, 0.0f, 0.0f); // Schwarz
    }

    // 2. look for next intersection
    float t = std::numeric_limits<float>::max(); // intiialize to possible maximum
    float u = 0.f, v = 0.f; // barycentric coordinates for triangle intersection
    unsigned int hitTri = 0; // id of triangle that was hit
    auto hitObjIt = intersectRayObjectsEarliest(objects.cbegin(), objects.cend(), ray, t, u, v, hitTri, intersectionTests);

    // If no object hit, show black background color
    if (hitObjIt == objects.cend()) {
        return Vec3f(0.0f, 0.0f, 0.0f);
    }

    // 3. calculate intersection point
    const SceneObject& hitObject = *hitObjIt;
    Vec3f intersectionPoint = ray.o + t * ray.d;

    // triangle vertices from object's mesh
    const auto& vertices = hitObject.mesh.getVertices();
    const auto& triangles = hitObject.mesh.getTriangles();
    const Vec3ui& tri = triangles[hitTri];
    Vec3f p0 = vertices[tri[0]];
    Vec3f p1 = vertices[tri[1]];
    Vec3f p2 = vertices[tri[2]];

    // normal vector of intersected triangle
    Vec3f normal = cross(p1 - p0, p2 - p0).normalized();

    // 4. Shadow Test
    // offset iwth epsilon, so it does not intersect it self
    const float eps = 1e-3f;
    Vec3f lightPos = state.getLight().position;
    Vec3f lightDir = (lightPos - intersectionPoint).normalized();
    float lightDist = (lightPos - intersectionPoint).length();

    // shoot shadow ray to light source
    Ray<float> shadowRay(intersectionPoint + normal * eps, lightDir);
    float shadowT;
    unsigned int shadowTri;
    float dummyU, dummyV;
    auto shadowHit = intersectRayObjectsEarliest(objects.cbegin(), objects.cend(), shadowRay, shadowT, dummyU, dummyV, shadowTri, intersectionTests);

    // If object is hit before the light, poiint is in a shadow
    float S_i = 1.0f; // 1 = lit, 0 = not so lit
    if (shadowHit != objects.cend() && shadowT < lightDist) {
        S_i = 0.0f;
    }

    // 5. calculate phong lighting at intersection
    Vec3f viewDir = (ray.o - intersectionPoint).normalized();
    Vec3f ambient = hitObject.ambientColor * state.getLight().ambientIntensity;

    // diffuse
    float NdotL = std::max(0.0f, dot(normal, lightDir));
    Vec3f diffuse = hitObject.diffuseColor * NdotL * state.getLight().lightIntensity;
    
    // specular
    Vec3f reflectDir = (2.0f * normal * (normal * lightDir) - lightDir).normalized();
    float RdotV = std::max(0.0f, dot(reflectDir, viewDir));
    Vec3f specular = hitObject.specularColor * std::pow(RdotV, hitObject.shininess) * state.getLight().lightIntensity;

    // combine ambient, diffuse and specular color with the shadow factor to have the phong Color
    Vec3f phongColor = ambient + S_i * (diffuse + specular);

    // 6. recursive
    float k_r = hitObject.reflectionIntensity; // intensity of reflection (I)
    if (k_r > 0.0f) {
        // generate reflection
        Ray<float> reflectionRay(intersectionPoint + normal * eps, reflectDir);
        Vec3f reflectionColor = traceRay(reflectionRay, recursion_depth - 1, intersectionTests); //trace recursively

        // add reflection into the phong  Color
        phongColor += k_r * reflectionColor;
    }

    // 7. transparency
    float k_t = hitObject.transparency; // transparency factor
    if (k_t > 0.0f) {
        float refractionIndex = hitObject.refractiveIndex; // refractive index of the object
        Vec3f refractDir = refract(ray.d, normal, refractionIndex);// refraction direction
        // if the refractDirection is valid, acually calculated the color and add the color value
        if (refractDir.length() > 0) {
            // generate refraction ray with offset
            Ray<float> refractionRay(intersectionPoint - normal * eps, refractDir);
            // recursive tracing the refraction ray
            Vec3f refractionColor = traceRay(refractionRay, recursion_depth - 1, intersectionTests);

            // add transparency part of coloring
            phongColor += k_t * refractionColor;
        }
    }

    return phongColor;
}

Vec3f OpenGLView::refract(const Vec3f& incident, const Vec3f& normal, float eta) {
    float NdotI = dot(normal, incident);
    // discriminant for refraction equation
    float k = 1.0f - eta * eta * (1.0f - NdotI * NdotI);

    // if negative/zero, no refraction occurs
    if (k <= 0.0f) {
        return Vec3f(0.0f, 0.0f, 0.0f); // Totale interne Reflexion
    }
    else {
        // refracted direction 
        // https://registry.khronos.org/OpenGL-Refpages/gl4/html/refract.xhtml
        return eta * incident - (eta * NdotI + sqrt(k)) * normal;
    }
}

void OpenGLView::raytrace() {
    // initalization
    size_t w = width() * 0.75, h = height() * 0.75;
    size_t viewPortSize = w * h;
    std::vector<Vec3f> pictureRGB(viewPortSize);
    unsigned int intersectionTests = 0, hits = 0;
    auto clockStart = std::chrono::system_clock::now();
    std::cout << "   10   20   30   40   50   60   70   80   90  100" << std::endl;
    std::cout << "====|====|====|====|====|====|====|====|====|====|" << std::endl;
    // iterate over all pixel
    unsigned int pixelCounter = 0;
    int maxDepth = 5;
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // get pixel index for addressing pictureRGB array
            size_t pixel = y * w + x;
            QVector3D eye(x, y, -1.f), end(x, y, 1.f);
            eye = eye.unproject(state.getCurrentModelViewMatrix(), state.getCurrentProjectionMatrix(), QRect(0, 0, w, h));
            end = end.unproject(state.getCurrentModelViewMatrix(), state.getCurrentProjectionMatrix(), QRect(0, 0, w, h));
            const auto& eyeF(QVector3DToVec3f(eye));
            const auto& endF(QVector3DToVec3f(end));

            Ray<float> ray(eyeF, endF);
            pictureRGB[pixel] = traceRay(ray, maxDepth, intersectionTests);

            // cout "." every 1/50 of all pixels

            #pragma omp critical
            {
                pixelCounter++;
                if (pixelCounter % (viewPortSize / 50) == 0) std::clog << ".";
            };
        }

    }
    auto clockEnd = std::chrono::system_clock::now();
    auto passedTime = clockEnd - clockStart;
    std::cout << std::endl << "finished. tests: " << intersectionTests << ", hits: " << hits << ", ms: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(passedTime).count() << std::endl;
    // generate openGL texture
    float mul = 255.0f; // multiply rgb values within [0,1] by 255
    std::cout << "normalizing picture with multiplicator " << mul << std::endl << std::endl;
    std::unique_ptr<GLubyte[]> picture{new GLubyte[viewPortSize * 4]};

    for (unsigned int y = 0; y < h; y++) {
        for (unsigned int x = 0; x < w; x++) {
            // get pixel index for addressing pictureRGB array
            size_t pixel = y * w + x;
            // cap R G B within 0 and 255
            int R = std::max(0, std::min((int) (mul * pictureRGB[pixel][0]), 255));
            int G = std::max(0, std::min((int) (mul * pictureRGB[pixel][1]), 255));
            int B = std::max(0, std::min((int) (mul * pictureRGB[pixel][2]), 255));
            // enter R G B values into GLuint array
            picture[4 * pixel] = R;
            picture[4 * pixel + 1] = G;
            picture[4 * pixel + 2] = B;
            picture[4 * pixel + 3] = 255;
        }
    }
    // generate texture
    if (!raytracedTextureID) f->glGenTextures(1, &raytracedTextureID);
    f->glBindTexture(GL_TEXTURE_2D, raytracedTextureID);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
        picture.get());
}

GLuint OpenGLView::genRayTraceVAO() {
    GLuint result;
    f->glGenVertexArrays(1, &result);
    const static GLfloat vertices[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
    };

    const static GLfloat texCoords[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            1.0f, 1.0f,
            0.0f, 1.0f,
    };

    f->glGenBuffers(2, rayTraceVBOs);
    f->glBindVertexArray(result);
    f->glBindBuffer(GL_ARRAY_BUFFER, rayTraceVBOs[0]);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    f->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(POSITION_LOCATION);
    f->glBindBuffer(GL_ARRAY_BUFFER, rayTraceVBOs[1]);
    f->glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    f->glVertexAttribPointer(TEXCOORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(TEXCOORD_LOCATION);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    f->glBindVertexArray(0);

    return result;
}
