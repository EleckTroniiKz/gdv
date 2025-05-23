// ========================================================================= //
// Authors: Daniel Rutz, Daniel Ströter, Roman Getto, Matthias Bein          //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// Content: Simple class for reading and rendering triangle meshes           //
// ========================================================================= //

#include <cfloat>
#include <fstream>
#include <iostream>
#include <iomanip>

#include <QtMath>
#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>

#include "trianglemesh.h"

// =========================
// === PRIVATE FUNCTIONS ===
// =========================

void TriangleMesh::calculateNormals() {
    normals.clear();
    normals.resize(vertices.size());
    for (const auto& face : triangles) {
        const GLuint iX = face[0];
        const GLuint iY = face[1];
        const GLuint iZ = face[2];

        const Vertex& vecX = vertices[iX];
        const Vertex& vecY = vertices[iY];
        const Vertex& vecZ = vertices[iZ];

        const Vertex vec1 = vecY - vecX;
        const Vertex vec2 = vecZ - vecX;
        const Vertex normal = cross(vec1, vec2);

        normals[iX] += normal;
        normals[iY] += normal;
        normals[iZ] += normal;
    }

    for (auto& normal : normals) {
        normal.normalize();
    }
}

void TriangleMesh::rewriteAllVBOs() {
    //TODO: Fill VBO buffers with data (Ex. 2)

	f->glBindBuffer(GL_ARRAY_BUFFER, VBOv);
	f->glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

	// Normalen-Daten in VBO schreiben
	f->glBindBuffer(GL_ARRAY_BUFFER, VBOn);
	f->glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(Vertex), normals.data(), GL_STATIC_DRAW);

	// Index-Daten in VBO schreiben
	f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOf);
	f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(Triangle), triangles.data(), GL_STATIC_DRAW);

	// Bindung aufheben
	f->glBindBuffer(GL_ARRAY_BUFFER, 0);
	f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void TriangleMesh::createAllVBOs() {
	//TODO: Generate Buffers (Ex. 2)
    

	f->glGenBuffers(1, &VBOv); // Vertex-VBO erzeugen
	f->glGenBuffers(1, &VBOn); // Normalen-VBO erzeugen
	f->glGenBuffers(1, &VBOf); // Index-VBO erzeugen

	rewriteAllVBOs(); // Daten in die VBOs schreiben
}

void TriangleMesh::cleanupVBO() {
	// delete VBO
    if (VBOv != 0) f->glDeleteBuffers(1, &VBOv);
	if (VBOn != 0) f->glDeleteBuffers(1, &VBOn);
	if (VBOf != 0) f->glDeleteBuffers(1, &VBOf);
	VBOv = 0;
	VBOn = 0;
	VBOf = 0;
}

void TriangleMesh::cleanup() {
	// clear mesh data
	vertices.clear();
	triangles.clear();
	normals.clear();
	// clear bounding box data
	boundingBoxMin = Vec3f(FLT_MAX, FLT_MAX, FLT_MAX);
	boundingBoxMax = Vec3f(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// clear VBO data
	cleanupVBO();
}

// ===============================
// === CONSTRUCTOR, DESTRUCTOR ===
// ===============================

TriangleMesh::TriangleMesh(QOpenGLFunctions_2_1* f) :
	VBOv(0), VBOn(0), VBOf(0), f(f)
{ }

TriangleMesh::~TriangleMesh()
{
	// delete VBO
	if (VBOv != 0) f->glDeleteBuffers(1, &VBOv);
	if (VBOn != 0) f->glDeleteBuffers(1, &VBOn);
	if (VBOf != 0) f->glDeleteBuffers(1, &VBOf);
}

// ================
// === RAW DATA ===
// ================

void TriangleMesh::flipNormals() {
  for (auto& normal : normals) {
    normal *= -1.0;
  }
}

void TriangleMesh::translateToCenter(const Vec3f& newBBmid) {
	Vec3f trans = newBBmid - boundingBoxMid;
	for (auto& vertex : vertices) {
        vertex += trans;
    }
	boundingBoxMin += trans;
	boundingBoxMax += trans;
	boundingBoxMid += trans;
	// data changed => fix VBOs
	rewriteAllVBOs();
}

void TriangleMesh::scaleToLength(const float newLength) {
	float length = std::max(std::max(boundingBoxSize.x(), boundingBoxSize.y()), boundingBoxSize.z());
	float scale = newLength / length;
	for (auto& vertex : vertices) {
        vertex *= scale;
    }
	boundingBoxMin *= scale;
	boundingBoxMax *= scale;
	boundingBoxMid *= scale;
	boundingBoxSize *= scale;
	// data changed => fix VBOs
	rewriteAllVBOs();
}

// =================
// === LOAD MESH ===
// =================

void TriangleMesh::loadOFF(const char* filename) {
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
	// clear any existing mesh
	cleanup();
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
	if (!noff) calculateNormals();
	// createVBO
	createAllVBOs();
}

void TriangleMesh::loadOFF(const char* filename, const Vec3f& BBmid, const float BBlength) {
	loadOFF(filename);
	translateToCenter(BBmid);
	scaleToLength(BBlength);
}

// ==============
// === RENDER ===
// ==============

void TriangleMesh::drawImmediate() {
    if (triangles.empty()) return;

    f->glBegin(GL_TRIANGLES);
    for (const auto& face : triangles) {
        const auto& vertex1 = vertices[face[0]];
        const auto& vertex2 = vertices[face[1]];
        const auto& vertex3 = vertices[face[2]];

        const auto& normal1 = normals[face[0]];
        const auto& normal2 = normals[face[1]];
        const auto& normal3 = normals[face[2]];

        f->glNormal3f(normal1.x(), normal1.y(), normal1.z());
        f->glVertex3f(vertex1.x(), vertex1.y(), vertex1.z());

        f->glNormal3f(normal2.x(), normal2.y(), normal2.z());
        f->glVertex3f(vertex2.x(), vertex2.y(), vertex2.z());

        f->glNormal3f(normal3.x(), normal3.y(), normal3.z());
        f->glVertex3f(vertex3.x(), vertex3.y(), vertex3.z());
    }
    f->glEnd();
}

void TriangleMesh::drawArray() {
	if (triangles.empty()) return;
	//TODO: Implement Array Mode (Ex. 1)

	// Normal und Vertex Arrays aktivieren
	f->glEnableClientState(GL_VERTEX_ARRAY);
	f->glEnableClientState(GL_NORMAL_ARRAY);

	// Pointer für Vertex und Normals übergeben
	f->glVertexPointer(3, GL_FLOAT, 0, vertices.data());
	f->glNormalPointer(GL_FLOAT, 0, normals.data());

	// Zeichnen
	f->glDrawElements(GL_TRIANGLES, triangles.size() * 3, GL_UNSIGNED_INT, triangles.data());

	// State cleanen 
	f->glDisableClientState(GL_VERTEX_ARRAY);
	f->glDisableClientState(GL_NORMAL_ARRAY);
}

void TriangleMesh::drawVBO() {
	if (triangles.empty()) return;
	if (VBOv == 0 || VBOn == 0 || VBOf == 0) return;
    //TODO: Implement VBO Mode (Ex. 2)

	// OpenGL sagen, dass Daten von diesem buffer verwendet werden sollen
	f->glBindBuffer(GL_ARRAY_BUFFER, VBOv); // Diesmal Vertex Buffer für GPU storing und schnelle Berechnung
	f->glEnableClientState(GL_VERTEX_ARRAY); //s.o.
	f->glVertexPointer(3, GL_FLOAT, 0, nullptr);

	// gleiches für Normalen
	f->glBindBuffer(GL_ARRAY_BUFFER, VBOn);
	f->glEnableClientState(GL_NORMAL_ARRAY);
	f->glNormalPointer(GL_FLOAT, 0, nullptr);

	f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, VBOf);

	// 4. Zeichnen
	f->glDrawElements(GL_TRIANGLES, triangles.size() * 3, GL_UNSIGNED_INT, nullptr);

	// 5. Buffer und State Clean
	f->glDisableClientState(GL_VERTEX_ARRAY);
	f->glDisableClientState(GL_NORMAL_ARRAY);
	f->glBindBuffer(GL_ARRAY_BUFFER, 0);
	f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
