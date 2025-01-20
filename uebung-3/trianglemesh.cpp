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
    : staticColor(1.f, 1.f, 1.f), f(f)
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
    colors.clear();
    texCoords.clear();
    // clear bounding box data
    boundingBoxMin = Vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
    boundingBoxMax = Vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    boundingBoxMid.zero();
    boundingBoxSize.zero();
    // draw mode data
    coloringType = ColoringType::STATIC_COLOR;
    withBB = false;
    withNormals = false;
    textureID.val = 0;
    cleanupVBO();
}

void TriangleMesh::coutData() {
    std::cout << std::endl;
    std::cout << "=== MESH DATA ===" << std::endl;
    std::cout << "nr. triangles: " << triangles.size() << std::endl;
    std::cout << "nr. vertices:  " << vertices.size() << std::endl;
    std::cout << "nr. normals:   " << normals.size() << std::endl;
    std::cout << "nr. colors:    " << colors.size() << std::endl;
    std::cout << "nr. texCoords: " << texCoords.size() << std::endl;
    std::cout << "BB: (" << boundingBoxMin << ") - (" << boundingBoxMax << ")" << std::endl;
    std::cout << "  BBMid: (" << boundingBoxMid << ")" << std::endl;
    std::cout << "  BBSize: (" << boundingBoxSize << ")" << std::endl;
    std::cout << "  VAO ID: " << VAO() << ", VBO IDs: f=" << VBOf() << ", v=" << VBOv() << ", n=" << VBOn() << ", c=" << VBOc() << ", t=" << VBOt() << std::endl;
    std::cout << "coloring using: ";
    switch (coloringType) {
        case ColoringType::STATIC_COLOR:
            std::cout << "a static color" << std::endl;
            break;
        case ColoringType::COLOR_ARRAY:
            std::cout << "a color array" << std::endl;
            break;
        case ColoringType::TEXTURE:
            std::cout << "a texture" << std::endl;
            break;

        case ColoringType::BUMP_MAPPING:
            std::cout << "a bump map" << std::endl;
            break;
    }
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
    // calculate texture coordinates
    calculateTexCoordsSphereMapping();
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

void TriangleMesh::calculateTexCoordsSphereMapping() {
    texCoords.clear();
    // texCoords by central projection on unit sphere
    // optional ...
    for (const auto& vertex : vertices) {
        const auto dist = vertex - boundingBoxMid;
        float u = (M_1_PI / 2) * std::atan2(dist.x(), dist.z()) + 0.5;
        float v = M_1_PI * std::asin(dist.y() / std::sqrt(dist.x() * dist.x() + dist.y() * dist.y() + dist.z() * dist.z()));
        texCoords.push_back(TexCoord{ u, v });
    }

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
    if (colors.size() == vertices.size()) {
        VBOc.val = createVBO(f, colors.data(), colors.size() * sizeof(Color), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
        f->glEnableVertexAttribArray(COLOR_LOCATION);
    }
    if (texCoords.size() == vertices.size()) {
        VBOt.val = createVBO(f, texCoords.data(), texCoords.size() * sizeof(TexCoord), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
        f->glEnableVertexAttribArray(TEXCOORD_LOCATION);
    }
    if (tangents.size() == vertices.size()) {
        VBOtan.val = createVBO(f, tangents.data(), tangents.size() * sizeof(Tangent), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
        f->glEnableVertexAttribArray(TANGENT_LOCATION);   
    }

    // bind VBOs to VAO object
    f->glBindVertexArray(VAO.val);
    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOf.val);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBOv.val);
    f->glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(POSITION_LOCATION);
    f->glBindBuffer(GL_ARRAY_BUFFER, VBOn.val);
    f->glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    f->glEnableVertexAttribArray(NORMAL_LOCATION);
    if (VBOc.val) {
        f->glBindBuffer(GL_ARRAY_BUFFER, VBOc.val);
        f->glVertexAttribPointer(COLOR_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        f->glEnableVertexAttribArray(COLOR_LOCATION);
    }

    if (VBOt.val) {
        f->glBindBuffer(GL_ARRAY_BUFFER, VBOt.val);
        f->glVertexAttribPointer(TEXCOORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        f->glEnableVertexAttribArray(TEXCOORD_LOCATION);
    }
    if (VBOtan.val) {
        f->glBindBuffer(GL_ARRAY_BUFFER, VBOtan.val);
        f->glVertexAttribPointer(TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        f->glEnableVertexAttribArray(TANGENT_LOCATION);   
    }

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
    if (VBOc.val != 0) f->glDeleteBuffers(1, &VBOc.val);
    if (VBOt.val != 0) f->glDeleteBuffers(1, &VBOt.val);
    if (VBOtan.val != 0) f->glDeleteBuffers(1, &VBOtan.val);
    if (VAObb.val != 0) f->glDeleteVertexArrays(1, &VAObb.val);
    if (VBOvbb.val != 0) f->glDeleteBuffers(1, &VBOvbb.val);
    if (VBOfbb.val != 0) f->glDeleteBuffers(1, &VBOfbb.val);
    if (VAOn.val != 0) f->glDeleteVertexArrays(1, &VAOn.val);
    if (VBOvn.val != 0) f->glDeleteBuffers(1, &VBOvn.val);
    VBOv.val = 0;
    VBOn.val = 0;
    VBOf.val = 0;
    VBOc.val = 0;
    VBOt.val = 0;
    VBOtan.val = 0;
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

    //Bug in Qt: They flagged glVertexAttrib3f as deprecated in modern OpenGL, which is not true.
    //We have to load it manually. Make it static so we do it only once.
    static auto glVertexAttrib3fv = reinterpret_cast<glVertexAttrib3fvPtr>(QOpenGLContext::currentContext()->getProcAddress("glVertexAttrib3fv"));
    
    // The VAO keeps track of all the buffers and the element buffer, so we do not need to bind else except for the VAO
    f->glBindVertexArray(VAO.val);
    f->glUniformMatrix4fv(state.getModelViewUniform(), 1, GL_FALSE, state.getCurrentModelViewMatrix().data());
    f->glUniformMatrix3fv(state.getNormalMatrixUniform(), 1, GL_FALSE, state.calculateNormalMatrix().data());
    switch (coloringType) {
        case ColoringType::TEXTURE:
            if (textureID.val != 0) {
                f->glUniform1ui(state.getUseTextureUniform(), GL_TRUE);
                f->glActiveTexture(GL_TEXTURE0);
                f->glBindTexture(GL_TEXTURE_2D, textureID.val);
                f->glUniform1i(state.getTextureUniform(), 0);
                break;
            }
            //[[fallthrough]];

        case ColoringType::COLOR_ARRAY:
            if (VBOc.val != 0) {
                f->glUniform1ui(state.getUseTextureUniform(), GL_FALSE);
                f->glEnableVertexAttribArray(COLOR_LOCATION);
                break;
            }
            //[[fallthrough]];

        case ColoringType::STATIC_COLOR:
            f->glUniform1ui(state.getUseTextureUniform(), GL_FALSE);
            f->glDisableVertexAttribArray(COLOR_LOCATION); //By disabling the attribute array, it uses the value set in the following line.
            glVertexAttrib3fv(2, reinterpret_cast<const GLfloat*>(&staticColor));
            break;

        case ColoringType::BUMP_MAPPING:
            // Use static color as base color.
            f->glDisableVertexAttribArray(COLOR_LOCATION);
            glVertexAttrib3fv(2, reinterpret_cast<const GLfloat*>(&staticColor));

            GLint location;
            auto program = state.getCurrentProgram();

            location = f->glGetUniformLocation(program, "useDiffuse");
            f->glUniform1ui(location, enableDiffuseTexture);

            location = f->glGetUniformLocation(program, "useNormal");
            f->glUniform1ui(location, enableNormalMapping);

            location = f->glGetUniformLocation(program, "useDisplacement");
            f->glUniform1ui(location, enableDisplacementMapping);

            location = f->glGetUniformLocation(program, "diffuseTexture");
            f->glUniform1i(location, 0);
            f->glActiveTexture(GL_TEXTURE0);
            f->glBindTexture(GL_TEXTURE_2D, textureID.val);

            location = f->glGetUniformLocation(program, "normalTexture");
            f->glUniform1i(location, 1);
            f->glActiveTexture(GL_TEXTURE1);
            f->glBindTexture(GL_TEXTURE_2D, normalMapID.val);

            location = f->glGetUniformLocation(program, "displacementTexture");
            f->glUniform1i(location, 3);
            f->glActiveTexture(GL_TEXTURE3);
            f->glBindTexture(GL_TEXTURE_2D, displacementMapID.val);
            break;
    }
    f->glDrawElements(GL_TRIANGLES, 3*triangles.size(), GL_UNSIGNED_INT, nullptr);
}

// ===========
// === VFC ===
// ===========

bool TriangleMesh::boundingBoxIsVisible(const RenderState& state) {


    // Retrieve the view-projection matrix from the RenderState
    QMatrix4x4 viewProjectionMatrix = state.getCurrentProjectionMatrix() * state.getCurrentModelViewMatrix();

    // Planes des View Frustums aus der projection Matrix rausziehen
    // https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf Appendix B
    std::array<ClipPlane, 6> planes = {
        // Left Plane
        ClipPlane(
            viewProjectionMatrix(0, 3) + viewProjectionMatrix(0, 0),
            viewProjectionMatrix(1, 3) + viewProjectionMatrix(1, 0),
            viewProjectionMatrix(2, 3) + viewProjectionMatrix(2, 0),
            viewProjectionMatrix(3, 3) + viewProjectionMatrix(3, 0)
        ),
        // Right Plane
        ClipPlane(
            viewProjectionMatrix(0, 3) - viewProjectionMatrix(0, 0),
            viewProjectionMatrix(1, 3) - viewProjectionMatrix(1, 0),
            viewProjectionMatrix(2, 3) - viewProjectionMatrix(2, 0),
            viewProjectionMatrix(3, 3) - viewProjectionMatrix(3, 0)
        ),
        // Bottom Plane
        ClipPlane(
            viewProjectionMatrix(0, 3) + viewProjectionMatrix(0, 1),
            viewProjectionMatrix(1, 3) + viewProjectionMatrix(1, 1),
            viewProjectionMatrix(2, 3) + viewProjectionMatrix(2, 1),
            viewProjectionMatrix(3, 3) + viewProjectionMatrix(3, 1)
        ),
        // Top Plane
        ClipPlane(
            viewProjectionMatrix(0, 3) - viewProjectionMatrix(0, 1),
            viewProjectionMatrix(1, 3) - viewProjectionMatrix(1, 1),
            viewProjectionMatrix(2, 3) - viewProjectionMatrix(2, 1),
            viewProjectionMatrix(3, 3) - viewProjectionMatrix(3, 1)
        ),
        // Near Plane
        ClipPlane(
            viewProjectionMatrix(0, 3) + viewProjectionMatrix(0, 2),
            viewProjectionMatrix(1, 3) + viewProjectionMatrix(1, 2),
            viewProjectionMatrix(2, 3) + viewProjectionMatrix(2, 2),
            viewProjectionMatrix(3, 3) + viewProjectionMatrix(3, 2)
        ),
        // Far Plane
        ClipPlane(
            viewProjectionMatrix(0, 3) - viewProjectionMatrix(0, 2),
            viewProjectionMatrix(1, 3) - viewProjectionMatrix(1, 2),
            viewProjectionMatrix(2, 3) - viewProjectionMatrix(2, 2),
            viewProjectionMatrix(3, 3) - viewProjectionMatrix(3, 2)
        )
    };

    // Alle Eckpunkte der Bounding Box vom aktuellen Triangle Mesh
    std::array<Vec3f, 8> corners = {
        boundingBoxMin, // A
        Vec3f(boundingBoxMax.x(), boundingBoxMin.y(), boundingBoxMin.z()), // B
        Vec3f(boundingBoxMin.x(), boundingBoxMax.y(), boundingBoxMin.z()), // C 
        Vec3f(boundingBoxMin.x(), boundingBoxMin.y(), boundingBoxMax.z()), // D
        Vec3f(boundingBoxMax.x(), boundingBoxMax.y(), boundingBoxMin.z()), // E
        Vec3f(boundingBoxMax.x(), boundingBoxMin.y(), boundingBoxMax.z()), // F
        Vec3f(boundingBoxMin.x(), boundingBoxMax.y(), boundingBoxMax.z()), // G
        boundingBoxMax // H
    };

    for (auto& plane : planes) {
        bool meshNotVisible = true; // Annahme: alle Punkte sind außerhalb der Plane
        for (const auto& corner : corners) {
            if (plane.evaluatePoint(corner) > 0) {
                /* Resultat von evaluate Point : 
                    >0 = Punkt liegt auf sichtbaren Seite der Ebene
                    == 0 = Punkt liegt auf der Ebene
                    <0 = Punkt liegt auf der nicht sichtbaren Seite der Ebene
                */
                meshNotVisible = false;
                break;
            }
        }
        if (meshNotVisible) {
            return false; // Bounding box is outside this plane
        }
    }
    return true; // Bounding box is inside or intersects all planes
}

void TriangleMesh::setStaticColor(Vec3f color) {
    staticColor = color;
}

void TriangleMesh::drawBB(RenderState &state) {
    auto* f = state.getOpenGLFunctions();
    f->glBindVertexArray(VAObb.val);
    //Transform BB to correct position.
    state.pushModelViewMatrix();
    state.getCurrentModelViewMatrix().translate(boundingBoxMid.x(), boundingBoxMid.y(), boundingBoxMid.z());
    state.getCurrentModelViewMatrix().scale(boundingBoxSize.x(), boundingBoxSize.y(), boundingBoxSize.z());
    f->glUniformMatrix4fv(state.getModelViewUniform(), 1, GL_FALSE, state.getCurrentModelViewMatrix().data());
    //Set color to constant white.
    //Bug in Qt: They flagged glVertexAttrib3f as deprecated in modern OpenGL, which is not true.
    //We have to load it manually. Make it static so we do it only once.
    static auto glVertexAttrib3f = reinterpret_cast<glVertexAttrib3fPtr>(QOpenGLContext::currentContext()->getProcAddress("glVertexAttrib3f"));
    glVertexAttrib3f(2, 1.0f, 1.0f, 1.0f);

    f->glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, nullptr);
    state.popModelViewMatrix();
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

void TriangleMesh::generateSphere(QOpenGLFunctions_3_3_Core* f) {
    // The sphere consists of latdiv rings of longdiv faces.
    int longdiv = 200; // minimum 4
    int latdiv  = 100; // minimum 2

    setGLFunctionPtr(f);

    // Generate vertices.
    for (int latitude = 0; latitude <= latdiv; latitude++) {
        float v = static_cast<float>(latitude) / static_cast<float>(latdiv);
        float latangle = v * M_PI;

        float extent = std::sin(latangle);
        float y = -std::cos(latangle);

        for (int longitude = 0; longitude <= longdiv; longitude++) {
            float u = static_cast<float>(longitude) / static_cast<float>(longdiv);
            float longangle = u * 2.0f * M_PI;

            float z = std::sin(longangle) * extent;
            float x = std::cos(longangle) * extent;

            Vec3f pos(x, y, z);

            vertices.push_back(pos);
            normals.push_back(pos);
            texCoords.push_back({ 2.0f - 2.0f * u, v });
            tangents.push_back(cross(Vec3f(0, 1, 0), pos));
        }
    }

    for (int latitude = 0; latitude < latdiv; latitude++) {
        unsigned int bottomBase = latitude * (longdiv + 1);
        unsigned int topBase = (latitude + 1) * (longdiv + 1);
        for (int longitude = 0; longitude < longdiv; longitude++) {
            unsigned int bottomCurrent = bottomBase + longitude;
            unsigned int bottomNext = bottomBase + (longitude + 1);
            unsigned int topCurrent = topBase + longitude;
            unsigned int topNext = topBase + (longitude + 1);
            triangles.emplace_back(bottomCurrent, bottomNext, topNext);
            triangles.emplace_back(topNext, topCurrent, bottomCurrent);
        }
    }

    boundingBoxMid = Vec3f(0, 0, 0);
    boundingBoxSize = Vec3f(2, 2, 2);
    boundingBoxMin = Vec3f(-1, -1, -1);
    boundingBoxMax = Vec3f(1, 1, 1);

    createAllVBOs();
}

void TriangleMesh::generateTerrain(unsigned int h, unsigned int w, unsigned int iterations) {
    // TODO(3.1): Implement terrain generation.

    coloringType = ColoringType::COLOR_ARRAY; // Setzt den Farbschema-Typ auf Farbarray

    vertices.clear();   // Entfernt alle vorhandenen Vertices
    triangles.clear();  // Entfernt alle vorhandenen Dreiecke
    colors.clear();     // Entfernt alle vorhandenen Farben

    vertices.reserve((h + 1) * (w + 1)); // Reserviert Speicherplatz für die Vertices
    colors.reserve((h + 1) * (w + 1));    // Reserviert Speicherplatz für die Farben

    std::random_device rd;                  // Erstellt ein zufälliges Gerät zur Generierung von Zufallszahlen
    std::mt19937 gen(rd());                 // Initialisiert den Mersenne-Twister Zufallszahlengenerator mit dem Seed vom Zufallsgerät

    // Noise-Parameter
    const int octaves = 6;                   // Anzahl der Oktaven für detailliertere Höhen
    const float persistence = 0.5f;          // Faktor zur Reduzierung der Amplitude pro Oktave
    const float lacunarity = 2.0f;           // Faktor zur Erhöhung der Frequenz pro Oktave
    const float baseFrequency = 0.1f;        // Basisfrequenz für die Höhenberechnung
    const float baseAmplitude = 1.0f;        // Basisamplitude für die Höhenberechnung

    // Phasenverschiebungen für Zufälligkeit bei jedem Lauf
    std::uniform_real_distribution<float> phaseDist(0.0f, 2.0f * M_PI); // Gleichverteilung für Phasenverschiebungen zwischen 0 und 2π
    std::vector<float> phaseX(octaves);  // Vektor zur Speicherung der Phasenverschiebungen in x-Richtung
    std::vector<float> phaseZ(octaves);  // Vektor zur Speicherung der Phasenverschiebungen in z-Richtung
    for(int i = 0; i < octaves; ++i) {
        phaseX[i] = phaseDist(gen);       // Generiert eine zufällige Phasenverschiebung für x
        phaseZ[i] = phaseDist(gen);       // Generiert eine zufällige Phasenverschiebung für z
    }

    // Kleines Rauschen für zusätzliche Variation
    std::uniform_real_distribution<float> noiseDist(-0.2f, 0.2f); // Gleichverteilung für Rauschen zwischen -0.2 und 0.2

    // Parameter für Bergzentren
    const int numMountains = 5; // Anzahl der Berge/Bergketten pro Terrain
    std::uniform_real_distribution<float> mountainDistX(0.25f * w, 0.75f * w); // Gleichverteilung für x-Position der Bergzentren zwischen 25% und 75% der Breite
    std::uniform_real_distribution<float> mountainDistZ(0.25f * h, 0.75f * h); // Gleichverteilung für z-Position der Bergzentren zwischen 25% und 75% der Höhe
    std::uniform_real_distribution<float> mountainRadiusDist(3.0f, 10.0f);    // Gleichverteilung für Bergradien zwischen 3.0 und 10.0
    std::uniform_real_distribution<float> mountainHeightDist(1.5f, 3.0f);    // Gleichverteilung für Berghöhen zwischen 1.5 und 3.0

    // Struktur zur Darstellung eines Berges
    struct Mountain {
        float x;      // x-Position des Bergzentrums
        float z;      // z-Position des Bergzentrums
        float radius; // Radius des Berges
        float height; // Höhe des Berges
    };

    // Generiere zufällige Bergzentren
    std::vector<Mountain> mountains; // Vektor zur Speicherung aller Berge
    for(int i = 0; i < numMountains; ++i) {
        Mountain m;
        m.x = mountainDistX(gen);       // Setzt eine zufällige x-Position für den Berg
        m.z = mountainDistZ(gen);       // Setzt eine zufällige z-Position für den Berg
        m.radius = mountainRadiusDist(gen);   // Setzt einen zufälligen Radius für den Berg
        m.height = mountainHeightDist(gen);   // Setzt eine zufällige Höhe für den Berg
        mountains.push_back(m);          // Fügt den Berg dem Vektor hinzu
    }

    // Schleife über alle Reihen des Terrains
    for (unsigned int row = 0; row <= h; ++row) {
        // Schleife über alle Spalten des Terrains
        for (unsigned int col = 0; col <= w; ++col) {
            float y = 0.0f;               // Initialisiert die Höhe für den aktuellen Vertex
            float frequency = baseFrequency; // Setzt die Anfangsfrequenz
            float amplitude = baseAmplitude; // Setzt die Anfangsamplitude

            // Generiere die Höhe mithilfe von mehrschichtigen Sinus- und Kosinusfunktionen mit Phasenverschiebungen
            for (int i = 0; i < octaves; ++i) {
                y += amplitude * std::sin(col * frequency + phaseX[i]) * std::cos(row * frequency + phaseZ[i]); // Addiert die Höhe basierend auf Sinus- und Kosinusfunktionen
                frequency *= lacunarity; // Erhöht die Frequenz für die nächste Oktave
                amplitude *= persistence; // Verringert die Amplitude für die nächste Oktave
            }

            y += noiseDist(gen); // Fügt eine kleine zufällige Abweichung für natürliche Variation hinzu

            // Hinzufügen der Höhen durch Bergzentren
            for(const auto& mountain : mountains) {
                float dx = static_cast<float>(col) - mountain.x; // Berechnet den Abstand in x-Richtung zum Bergzentrum
                float dz = static_cast<float>(row) - mountain.z; // Berechnet den Abstand in z-Richtung zum Bergzentrum
                float distance = std::sqrt(dx * dx + dz * dz);    // Berechnet den Gesamtabstand zum Bergzentrum
                if(distance < mountain.radius) { // Überprüft, ob der Punkt innerhalb des Bergradius liegt
                    // Gaußsche Gipfel
                    y += mountain.height * std::exp(-(distance * distance) / (2 * (mountain.radius / 2) * (mountain.radius / 2))); // Erhöht die Höhe basierend auf einer gaußschen Funktion
                }
            }

            // Begrenze die Höhe, um zu hohe oder zu tiefe Terrains zu verhindern
            y = std::clamp(y, -0.5f, 7.0f); // Beschränkt die Höhe auf den Bereich von -0.5 bis 7.0

            vertices.emplace_back(static_cast<float>(col), y, static_cast<float>(row)); // Fügt den berechneten Vertex zur Vertexliste hinzu

            // Weisen Sie Farben basierend auf der Höhe zu
            Vec3f color; // Farbvektor
            if (y > 4.0f) {
                color = Vec3f(1.0f, 1.0f, 1.0f); // Weiß für Schnee
            }
            else if (y > 3.0f) {
                color = Vec3f(0.7f, 0.7f, 0.6f); // Grau für felsige Bereiche
            }
            else if (y > 2.0f) {
                color = Vec3f(0.6f, 0.3f, 0.2f); // Braun für erdige oder felsige Bereiche
            }
            else if (y > 1.0f) {
                color = Vec3f(0.3f, 0.8f, 0.2f); // Hellgrün für Grasland
            }
            else if (y > 0.0f) {
                color = Vec3f(0.0f, 0.4f, 0.0f); // Dunkelgrün für dichte Vegetation
            }
            else {
                color = Vec3f(0.0f, 0.0f, 1.0f); // Blau für Wasserflächen
            }
            colors.emplace_back(color); // Fügt die Farbe zum Farbarray hinzu
        }
    }

    // Generiere Dreieck-Indizes für das Terrain
    triangles.reserve(h * w * 2); // Reserviert Speicherplatz für die Dreiecke
    for (unsigned int row = 0; row < h; ++row) {
        for (unsigned int col = 0; col < w; ++col) {
            unsigned int i0 = row * (w + 1) + col;          // Index des aktuellen Vertex
            unsigned int i1 = i0 + 1;                       // Index des nächsten Vertex in der Reihe
            unsigned int i2 = (row + 1) * (w + 1) + col;    // Index des Vertex in der nächsten Reihe
            unsigned int i3 = i2 + 1;                       // Index des nächsten Vertex in der nächsten Reihe

            triangles.emplace_back(i0, i2, i1); // Erstes Dreieck des Quadrats
            triangles.emplace_back(i1, i2, i3); // Zweites Dreieck des Quadrats
        }
    }

    // Erstelle und binde die Vertex Buffer Objects (VBOs)
    createAllVBOs(); // Initialisiert und bindet alle VBOs für das Rendering


    vertices.reserve(4);
    vertices.emplace_back(0, 0, 0);
    vertices.emplace_back(0, 0, 10);
    vertices.emplace_back(10, 0, 10);
    vertices.emplace_back(10, 0, 0);

    triangles.reserve(2);
    triangles.emplace_back(0, 1, 2);
    triangles.emplace_back(0, 2, 3);

    calculateNormalsByArea();
    calculateBB();
    createAllVBOs();
}
