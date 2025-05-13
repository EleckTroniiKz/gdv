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

class TriangleMesh {
public:
  // typedefs for data
  using Triangle = Vec3ui;
  using Normal = Vec3f;
  using Vertex = Vec3f;
  using Triangles = std::vector<Triangle>;
  using Normals = std::vector<Normal>;
  using Vertices = std::vector<Vertex>;
private:

  // mesh data
  Triangles triangles;
  Normals normals;
  Vertices vertices;

  // VBO ids for vertices, normals, faces
  GLuint VBOv, VBOn, VBOf;

  //OpenGL functions
  mutable QOpenGLFunctions_2_1* f;

  // bounding box data
  Vec3f boundingBoxMin;
  Vec3f boundingBoxMax;
  Vec3f boundingBoxMid;
  Vec3f boundingBoxSize;

  // =========================
  // === PRIVATE FUNCTIONS ===
  // =========================

  // calculate normals
  void calculateNormals();

  //Sync contents of VBOs with current content of TriangleMesh vectors
  void rewriteAllVBOs();
  
  // create VBOs for vertices, faces and normals
  void createAllVBOs();

  // clean up VBO data (delete from gpu memory)
  void cleanupVBO();  

  // clean up all data (including VBO data)
  void cleanup();


public:
  // ===============================
  // === CONSTRUCTOR, DESTRUCTOR ===
  // ===============================

  TriangleMesh(QOpenGLFunctions_2_1* f = nullptr);
  ~TriangleMesh();

  // disable copy and copy assignment as these require to copy VBOs using unique ids
  TriangleMesh(const TriangleMesh& o) = delete;
  TriangleMesh& operator=(const TriangleMesh& o) = delete;

  void setGLFunctionPtr(QOpenGLFunctions_2_1* f) { this->f = f; }
    
  // ================
  // === RAW DATA ===
  // ================

  // get raw data references
  Vertices& getVertices() { return vertices; }
  Triangles& getTriangles() { return triangles; }
  Normals& getNormals() { return normals; }
  
  const Vertices& getVertices() const { return vertices; }
  const Triangles& getTriangles() const { return triangles; }
  const Normals& getNormals() const { return normals; }

  // get size of all elements
  std::size_t getNumVertices() const { return vertices.size(); }
  std::size_t getNumNormals() const { return normals.size(); }
  std::size_t getNumTriangles() const { return triangles.size(); }

  // flip all normals
  void flipNormals();

  // translates vertices so that the bounding box center is at newBBmid
  void translateToCenter(const Vec3f& newBBmid);

  // scales vertices so that the largest bounding box size has length newLength
  void scaleToLength(float newLength);

  // =================
  // === LOAD MESH ===
  // =================

  // read from an OFF file. also calculates normals if not given in the file.
  void loadOFF(const char* filename);

  // read from an OFF file. also calculates normals if not given in the file.
  // translates and scales vertices with bounding box center at BBmid and largest side BBlength
  void loadOFF(const char* filename, const Vec3f& BBmid, float BBlength);

  // ==============
  // === RENDER ===
  // ==============
  
  // draw mesh with immediate mode
  void drawImmediate();

  // draw mesh with vertex array
  void drawArray(); 

  // draw VBO
  void drawVBO();
};

#endif

