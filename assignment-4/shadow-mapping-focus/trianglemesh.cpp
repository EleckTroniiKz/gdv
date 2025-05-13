// ========================================================================= //
// Authors: Daniel Rutz, Daniel Ströter, Roman Getto, Matthias Bein          //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// Content: Simple class for reading and rendering triangle meshes, SOLUTION //
//   * readOFF                                                               //
//   * draw                                                                  //
//   * transformations                                                       //
// ========================================================================= //

#include <cmath>
#include <array>
#include <cfloat>
#include <algorithm>
#include <random>
#include <array>

#include <fstream>
#include <iostream>
#include <iomanip>

#include <QtMath>
#include <QOpenGLFunctions_3_3_Core>

#include "trianglemesh.h"
#include "renderstate.h"
#include "utilities.h"
#include "clipplane.h"
#include "shader.h"

using glVertexAttrib3fvPtr = void (*)(GLuint index, const GLfloat* v);
using glVertexAttrib3fPtr = void (*)(GLuint index, GLfloat v1, GLfloat v2, GLfloat v3);

TriangleMesh::TriangleMesh(QOpenGLFunctions_3_3_Core* f)
    : f(f)
{
    clear();
}

TriangleMesh::~TriangleMesh() {
    // clear data
    clear();
}

void TriangleMesh::clear() {
    // clear mesh data
    vertices.clear();
    triangles.clear();
    normals.clear();
    // clear bounding box data
    boundingBoxMin = Vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
    boundingBoxMax = Vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    boundingBoxMid.zero();
    boundingBoxSize.zero();
    // draw mode data
    withBB = false;
    withNormals = false;
    cleanupVBO();
}

void TriangleMesh::coutData() {
    std::cout << std::endl;
    std::cout << "=== MESH DATA ===" << std::endl;
    std::cout << "nr. triangles: " << triangles.size() << std::endl;
    std::cout << "nr. vertices:  " << vertices.size() << std::endl;
    std::cout << "nr. normals:   " << normals.size() << std::endl;
    std::cout << "BB: (" << boundingBoxMin << ") - (" << boundingBoxMax << ")" << std::endl;
    std::cout << "  BBMid: (" << boundingBoxMid << ")" << std::endl;
    std::cout << "  BBSize: (" << boundingBoxSize << ")" << std::endl;
    std::cout << "  VAO ID: " << VAO() << ", VBO IDs: f=" << VBOf() << ", v=" << VBOv() << ", n=" << VBOn() << std::endl;
    std::cout << "coloring using: ";
}

// ================
// === RAW DATA ===
// ================

void TriangleMesh::flipNormals(bool createVBOs) {
    for (auto& n : normals) n *= -1.0f;
    //correct VBO
    if (createVBOs && VBOn() != 0) {
        if (!f) return;
        f->glBindBuffer(GL_ARRAY_BUFFER, VBOn());
        f->glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * sizeof(Normal), normals.data());
        f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void TriangleMesh::translateToCenter(const Vec3f& newBBmid, bool createVBOs) {
    Vec3f trans = newBBmid - boundingBoxMid;
    for (auto& vertex : vertices) vertex += trans;
    boundingBoxMin += trans;
    boundingBoxMax += trans;
    boundingBoxMid += trans;
    // data changed => delete VBOs and create new ones (not efficient but easy)
    if (createVBOs) {
        cleanupVBO();
        createAllVBOs();
    }
}

void TriangleMesh::scaleToLength(const float newLength, bool createVBOs) {
    float length = std::max(std::max(boundingBoxSize.x(), boundingBoxSize.y()), boundingBoxSize.z());
    float scale = newLength / length;
    for (auto& vertex : vertices) vertex *= scale;
    boundingBoxMin *= scale;
    boundingBoxMax *= scale;
    boundingBoxMid *= scale;
    boundingBoxSize *= scale;
    // data changed => delete VBOs and create new ones (not efficient but easy)
    if (createVBOs) {
        cleanupVBO();
        createAllVBOs();
    }
}

// =================
// === LOAD MESH ===
// =================

void TriangleMesh::loadOFF(const char* filename, bool createVBOs) {
    // clear any existing mesh
    clear();
    // load from off
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cout << "loadOFF: can not find " << filename << std::endl;
        return;
    }
    const int MAX = 256;
    char s[MAX];
    in >> std::setw(MAX) >> s;
    // differentiate between OFF (vertices only) and NOFF (vertices and normals)
    bool noff = false;
    if (s[0] == 'O' && s[1] == 'F' && s[2] == 'F')
        ;
    else if (s[0] == 'N' && s[1] == 'O' && s[2] == 'F' && s[3] == 'F')
        noff = true;
    else
        return;
    // get number of vertices nv, faces nf and edges ne
    int nv,nf,ne;
    in >> std::setw(MAX) >> nv;
    in >> std::setw(MAX) >> nf;
    in >> std::setw(MAX) >> ne;
    if (nv <= 0 || nf <= 0) return;
    // read vertices
    vertices.resize(nv);
    for (int i = 0; i < nv; ++i) {
        in >> std::setw(MAX) >> vertices[i][0];
        in >> std::setw(MAX) >> vertices[i][1];
        in >> std::setw(MAX) >> vertices[i][2];
        boundingBoxMin[0] = std::min(vertices[i][0], boundingBoxMin[0]);
        boundingBoxMin[1] = std::min(vertices[i][1], boundingBoxMin[1]);
        boundingBoxMin[2] = std::min(vertices[i][2], boundingBoxMin[2]);
        boundingBoxMax[0] = std::max(vertices[i][0], boundingBoxMax[0]);
        boundingBoxMax[1] = std::max(vertices[i][1], boundingBoxMax[1]);
        boundingBoxMax[2] = std::max(vertices[i][2], boundingBoxMax[2]);
        if (noff) {
            in >> std::setw(MAX) >> normals[i][0];
            in >> std::setw(MAX) >> normals[i][1];
            in >> std::setw(MAX) >> normals[i][2];
        }
    }
    boundingBoxMid = 0.5f*boundingBoxMin + 0.5f*boundingBoxMax;
    boundingBoxSize = boundingBoxMax - boundingBoxMin;
    // read triangles
    triangles.resize(nf);
    for (int i = 0; i < nf; ++i) {
        int three;
        in >> std::setw(MAX) >> three;
        in >> std::setw(MAX) >> triangles[i][0];
        in >> std::setw(MAX) >> triangles[i][1];
        in >> std::setw(MAX) >> triangles[i][2];
    }
    // close ifstream
    in.close();
    // calculate normals if not given
    if (!noff) calculateNormalsByArea();
    // createVBO
    if (createVBOs) {
        createAllVBOs();
    }
}

void TriangleMesh::loadOFF(const char* filename, const Vec3f& BBmid, const float BBlength) {
    loadOFF(filename, false);
    translateToCenter(BBmid, false);
    scaleToLength(BBlength, true);
}

void TriangleMesh::calculateNormalsByArea() {
    // sum up triangle normals in each vertex
    normals.resize(vertices.size());
    for (auto& triangle : triangles) {
        unsigned int
            id0 = triangle[0],
            id1 = triangle[1],
            id2 = triangle[2];
        Vec3f
            vec1 = vertices[id1] - vertices[id0],
            vec2 = vertices[id2] - vertices[id0],
            normal = cross(vec1, vec2);
        normals[id0] += normal;
        normals[id1] += normal;
        normals[id2] += normal;
    }
    // normalize normals
    for (auto& normal : normals) normal.normalize();
}

void TriangleMesh::calculateBB() {
    // clear bounding box data
    boundingBoxMin = Vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
    boundingBoxMax = Vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    boundingBoxMid.zero();
    boundingBoxSize.zero();
    // iterate over vertices
    for (auto& vertex : vertices) {
        boundingBoxMin[0] = std::min(vertex[0], boundingBoxMin[0]);
        boundingBoxMin[1] = std::min(vertex[1], boundingBoxMin[1]);
        boundingBoxMin[2] = std::min(vertex[2], boundingBoxMin[2]);
        boundingBoxMax[0] = std::max(vertex[0], boundingBoxMax[0]);
        boundingBoxMax[1] = std::max(vertex[1], boundingBoxMax[1]);
        boundingBoxMax[2] = std::max(vertex[2], boundingBoxMax[2]);
    }
    boundingBoxMid = 0.5f*boundingBoxMin + 0.5f*boundingBoxMax;
    boundingBoxSize = boundingBoxMax - boundingBoxMin;
}

GLuint TriangleMesh::createVBO(QOpenGLFunctions_3_3_Core* f, const void* data, int dataSize, GLenum target, GLenum usage) {

    // 0 is reserved, glGenBuffers() will return non-zero id if success
    GLuint id = 0;
    // create a vbo
    f->glGenBuffers(1, &id);
    // activate vbo id to use
    f->glBindBuffer(target, id);
    // upload data to video card
    f->glBufferData(target, dataSize, data, usage);
    // check data size in VBO is same as input array, if not return 0 and delete VBO
    int bufferSize = 0;
    f->glGetBufferParameteriv(target, GL_BUFFER_SIZE, &bufferSize);
    if(dataSize != bufferSize) {
        f->glDeleteBuffers(1, &id);
        id = 0;
        std::cout << "createVBO() ERROR: Data size (" << dataSize << ") is mismatch with input array (" << bufferSize << ")." << std::endl;
    }
    // unbind after copying data
    f->glBindBuffer(target, 0);
    return id;
}

void TriangleMesh::createBBVAO(QOpenGLFunctions_3_3_Core* f) {
    f->glGenVertexArrays(1, &VAObb.val);

    // create VBOs of bounding box
    VBOvbb.val = createVBO(f, BoxVertices, BoxVerticesSize, GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    VBOfbb.val = createVBO(f, BoxLineIndices, BoxLineIndicesSize, GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);

    // bind VAO of bounding box
    f->glBindVertexArray(VAObb.val);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBOvbb.val);
    f->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOfbb.val);

    f->glEnableVertexAttribArray(POSITION_LOCATION);
    f->glBindVertexArray(0);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleMesh::createNormalVAO(QOpenGLFunctions_3_3_Core* f) {
    if (vertices.size() != normals.size()) return;
    std::vector<Vec3f> normalArrowVertices;
    normalArrowVertices.reserve(2 * vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        normalArrowVertices.push_back(vertices[i]);
        normalArrowVertices.push_back(vertices[i] + 0.1 * normals[i]);
    }

    f->glGenVertexArrays(1, &VAOn.val);
    VBOvn.val = createVBO(f, normalArrowVertices.data(), normalArrowVertices.size() * sizeof(Vertex), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    f->glBindVertexArray(VAOn.val);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBOvn.val);
    f->glEnableVertexAttribArray(POSITION_LOCATION);
    f->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glBindVertexArray(0);
    f->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void TriangleMesh::createAllVBOs() {
    if (!f) return;
    // create VAOs
    f->glGenVertexArrays(1, &VAO.val);

    // create VBOs
    VBOf.val = createVBO(f, triangles.data(), triangles.size() * sizeof(Triangle), GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
    VBOv.val = createVBO(f, vertices.data(), vertices.size() * sizeof(Vertex), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
    VBOn.val = createVBO(f, normals.data(), normals.size() * sizeof(Normal), GL_ARRAY_BUFFER, GL_STATIC_DRAW);

    // bind VBOs to VAO object
    f->glBindVertexArray(VAO.val);
    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOf.val);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBOv.val);
    f->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(POSITION_LOCATION);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBOn.val);
    f->glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(NORMAL_LOCATION);

    f->glBindVertexArray(0);

    createBBVAO(f);

    createNormalVAO(f);
}

void TriangleMesh::cleanupVBO() {
    if (!f) return;
    cleanupVBO(f);
}

void TriangleMesh::cleanupVBO(QOpenGLFunctions_3_3_Core* f) {
    // delete VAO
    if (VAO.val != 0) f->glDeleteVertexArrays(1, &VAO.val);
    // delete VBO
    if (VBOv.val != 0) f->glDeleteBuffers(1, &VBOv.val);
    if (VBOn.val != 0) f->glDeleteBuffers(1, &VBOn.val);
    if (VBOf.val != 0) f->glDeleteBuffers(1, &VBOf.val);
    if (VAObb.val != 0) f->glDeleteVertexArrays(1, &VAObb.val);
    if (VBOvbb.val != 0) f->glDeleteBuffers(1, &VBOvbb.val);
    if (VBOfbb.val != 0) f->glDeleteBuffers(1, &VBOfbb.val);
    if (VAOn.val != 0) f->glDeleteVertexArrays(1, &VAOn.val);
    if (VBOvn.val != 0) f->glDeleteBuffers(1, &VBOvn.val);
    VBOv.val = 0;
    VBOn.val = 0;
    VBOf.val = 0;
    VAO.val = 0;
    VAObb.val = 0;
    VBOfbb.val = 0;
    VBOvbb.val = 0;
    VAOn.val = 0;
    VBOvn.val = 0;
}

unsigned int TriangleMesh::draw(RenderState& state) {
    if (!boundingBoxIsVisible(state)) return 0;
    if (VAO.val == 0) return 0;
    state.setMatrices();
    if (withBB || withNormals) {
        GLuint formerProgram = state.getCurrentProgram();
        state.switchToStandardProgram();
        if (withBB) drawBB(state);
        if (withNormals) drawNormals(state);
        state.setCurrentProgram(formerProgram);
    }
    drawVBO(state);

    return triangles.size();
}

void TriangleMesh::drawVBO(RenderState& state) {
    auto* f = state.getOpenGLFunctions();

    // The VAO keeps track of all the buffers and the element buffer, so we do not need to bind else except for the VAO
    f->glBindVertexArray(VAO.val);

    f->glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, nullptr);
}

// ===========
// === VFC ===
// ===========

bool TriangleMesh::boundingBoxIsVisible(const RenderState& state) {
    // openGL stores matrices in column major order:
    // float pm = _11, _21, _31, _41, _12, _22, _32, _42, _13, _23, _33, _43, _14, _24, _34, _44
    //     index:  0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
    // TODO ...

    //Calculate ModelViewProjection Matrix
    const QMatrix4x4 mvp = state.getCurrentProjectionMatrix() * state.getCurrentModelViewMatrix();
    const float* combinedMatrix = mvp.constData();

    std::array<ClipPlane, 6> planes{
            // Left
            ClipPlane{combinedMatrix[3] + combinedMatrix[0],
                      combinedMatrix[7] + combinedMatrix[4],
                      combinedMatrix[11] + combinedMatrix[8],
                      combinedMatrix[15] + combinedMatrix[12]},

            // Right
            ClipPlane{combinedMatrix[3] - combinedMatrix[0],
                      combinedMatrix[7] - combinedMatrix[4],
                      combinedMatrix[11] - combinedMatrix[8],
                      combinedMatrix[15] - combinedMatrix[12]},

            // Top
            ClipPlane{combinedMatrix[3] - combinedMatrix[1],
                      combinedMatrix[7] - combinedMatrix[5],
                      combinedMatrix[11] - combinedMatrix[9],
                      combinedMatrix[15] - combinedMatrix[13]},

            // Bottom
            ClipPlane{combinedMatrix[3] + combinedMatrix[1],
                      combinedMatrix[7] + combinedMatrix[5],
                      combinedMatrix[11] + combinedMatrix[9],
                      combinedMatrix[15] + combinedMatrix[13]},

            // Near
            ClipPlane{combinedMatrix[3] + combinedMatrix[2],
                      combinedMatrix[7] + combinedMatrix[6],
                      combinedMatrix[11] + combinedMatrix[10],
                      combinedMatrix[15] + combinedMatrix[14]},

            // Far
            ClipPlane{combinedMatrix[3] - combinedMatrix[2],
                      combinedMatrix[7] - combinedMatrix[6],
                      combinedMatrix[11] - combinedMatrix[10],
                      combinedMatrix[15] - combinedMatrix[14]}
    };

    std::array<Vec3f, 8> boundBoxEdges {
            boundingBoxMin,                                                                               // back left bottom
            boundingBoxMin + Vec3f(0, boundingBoxSize.y(), 0),                                  // back left top
            boundingBoxMin + Vec3f(boundingBoxSize.x(), 0, 0),                                  // back right bottom
            boundingBoxMin + Vec3f(boundingBoxSize.x(), boundingBoxSize.y(), 0),                  // back right top
            boundingBoxMin + Vec3f(0, 0, boundingBoxSize.z()),                                  // front left bottom
            boundingBoxMin + Vec3f(0, boundingBoxSize.y(), boundingBoxSize.z()),                  // front left top
            boundingBoxMin + Vec3f(boundingBoxSize.x(), 0, boundingBoxSize.z()),                  // front right bottom
            boundingBoxMin + Vec3f(boundingBoxSize.x(), boundingBoxSize.y(), boundingBoxSize.z())   // front right top
    };

    for (auto& plane : planes) {
        bool allOutside = true;
        for (auto& edge : boundBoxEdges) {
            const float distance = plane.evaluatePoint(edge);
            if (distance > 0) {
                allOutside = false;
                break;
            }
        }
        if (allOutside) {
            return false;
        }
    }
    return true;
}

void TriangleMesh::drawBB(RenderState &state) {
    auto* f = state.getOpenGLFunctions();
    f->glBindVertexArray(VAObb.val);
    //Transform BB to correct position.
    state.pushModelViewMatrix();
    state.getCurrentModelViewMatrix().translate(boundingBoxMid.x(), boundingBoxMid.y(), boundingBoxMid.z());
    state.getCurrentModelViewMatrix().scale(boundingBoxSize.x(), boundingBoxSize.y(), boundingBoxSize.z());
    state.setMatrices();
    f->glUniformMatrix4fv(state.getModelViewUniform(), 1, GL_FALSE, state.getCurrentModelViewMatrix().data());
    //Set color to constant white.
    //Bug in Qt: They flagged glVertexAttrib3f as deprecated in modern OpenGL, which is not true.
    //We have to load it manually. Make it static so we do it only once.
    static auto glVertexAttrib3f = reinterpret_cast<glVertexAttrib3fPtr>(QOpenGLContext::currentContext()->getProcAddress("glVertexAttrib3f"));
    glVertexAttrib3f(2, 1.0f, 1.0f, 1.0f);

    f->glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    state.popModelViewMatrix();
    state.setMatrices();
}

void TriangleMesh::drawNormals(RenderState &state) {
    auto* f = state.getOpenGLFunctions();
    f->glBindVertexArray(VAOn.val);
    f->glUniformMatrix4fv(state.getModelViewUniform(), 1, GL_FALSE, state.getCurrentModelViewMatrix().data());

    //Set color to constant white.
    //Bug in Qt: They flagged glVertexAttrib3f as deprecated in modern OpenGL, which is not true.
    //We have to load it manually. Make it static so we do it only once.
    static auto glVertexAttrib3f = reinterpret_cast<glVertexAttrib3fPtr>(QOpenGLContext::currentContext()->getProcAddress("glVertexAttrib3f"));
    glVertexAttrib3f(2, 1.0f, 1.0f, 1.0f);

    f->glDrawArrays(GL_LINES, 0, vertices.size() * 2);
}
