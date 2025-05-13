// ========================================================================= //
// Authors: Roman Getto, Matthias Bein                                       //
// mailto:roman.getto@gris.informatik.tu-darmstadt.de                        //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// Content: Simple class for reading and rendering triangle meshes            //
// ========================================================================== //
#ifndef TRIANGLEMESH_H
#define TRIANGLEMESH_H

#include <array>
#include <vector>

#include <QOpenGLFunctions_2_1>

#include "vec3.h"

using namespace std;

class TriangleMesh
{

private:
    // typedefs for data
    typedef Vec3i Triangle;
    typedef Vec3f Normal;
    typedef Vec3f Vertex;
    typedef vector<Triangle> Triangles;
    typedef vector<Normal> Normals;
    typedef vector<Vertex> Vertices;

    // data of TriangleMesh
    Vertices vertices;
    Normals normals;
    Triangles triangles;

public:
    // ================
    // === RAW DATA ===
    // ================

    // get raw data references
    vector<Vertex> &getPoints();
    vector<Triangle> &getTriangles();
    vector<Normal> &getNormals();

    const vector<Vertex> &getPoints() const;
    const vector<Triangle> &getTriangles() const;
    const vector<Normal> &getNormals() const;

    // flip all normals
    void flipNormals();
    void calculateNormals(bool weightByAngle = false);
    void drawNormals(QOpenGLFunctions_2_1 *f);

    // =================
    // === LOAD MESH ===
    // =================

    // === Hilfsmethoden ===
    void loadTriangles(std::ifstream &in, int nf);

    // read from an LSA file. also calculates normals.
    void loadLSA(const char *filename);

    // read from an OFF file. also calculates normals.
    void loadOFF(const char *filename);

    // ==============
    // === RENDER ===
    // ==============

    // draw mesh with set transformation
    void draw(QOpenGLFunctions_2_1 *f);
};

#endif
