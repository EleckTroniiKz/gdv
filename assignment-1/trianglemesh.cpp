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

#include <iostream>
#include <fstream>
#include <cfloat>

#include <QtMath>
#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>

#include "trianglemesh.h"
#include "globals.h"

// Hilfsfunktion, die das Skalarprodukt zweier Vektoren berechnet
float scarProd(Vec3f a, Vec3f b)
{
    return a.x() * b.x() + a.y() * b.y() + a.z() * b.z();
}

/* TASK- 5 : 
    In der Anwendung ist ein Inputfeld zu sehen.Die Zahl "x" die in diesem Feld eingegeben
    wird(int), wird nach einem Button Click auf den Knopf darunter dafür sorgen,
    dass das aktuelle Modell x mal gerendertert wird.Es wird jedoch nur nach einem Objekt aussehen,
    weil die Position nicht verändert wird.

    An einem Rechner mit einer RX5700XT Grafikkarte mit 8GB VRAM, wurden diese Werte festgestellt:
    - 1 Ballon: 6.000 Triangles; 144 FPS
    - 10 Ballons: 60.000 Triangles; 144 FPS
    - 100 Ballons: 600.000 Triangles; 75-85 FPS
    - 250 Ballons: 1.500.000 Triangles; 35-45 FPS
    - 320 Ballons: 1.920.000 Triangles; 30 FPS
    - 500 Ballons: 3.000.000 Triangles; 20 FPS
    - 1000 Ballons: 6.000.000 Trignales; 10 FPS
    - 2000 Ballons: 12.000.000 Triangles; 5 FPS
    - 5000 Ballons: 30.000.000 Triangles; 2 FPS
    
    Was man nun als Flüssig interpretiert ist verschieden. Aber unserer Meinung nach kann 
    man bei dieser Hardware zwischen 1 Million und 2 Million Dreiecke flüssig darstellen.
    Die View ist zwar bei mehreren Triangles noch nutzbar, aber etwas ... unangenehm.
*/


// TASK-4: Berechnung der Normalen

void TriangleMesh::calculateNormals(bool weightByAngle)
{
    normals.clear();
    normals.resize(vertices.size());
    // TODO: 4a) calculate normals for each vertex

    for (const auto &triangle : triangles) {
        int v0 = triangle[0];
        int v1 = triangle[1];
        int v2 = triangle[2];

        // Berechnung Kantenvekotren aus den Eckpunkten des Dreiecks
        Vec3f e1 = vertices[v1] - vertices[v0];
        Vec3f e2 = vertices[v2] - vertices[v0];
        Vec3f e3 = vertices[v2] - vertices[v1];

        Vec3f crossProduct = cross(e1, e2); // Kreuzprodukt berechnung

        // TODO: 4b) weight normals by angle if weightByAngle is true
        if (weightByAngle) {
            Vec3f e1N = e1.normalized();
            Vec3f e2N = e2.normalized();
            Vec3f e3N = e3.normalized();

            // Winkel berechnung zwischen Kanten durch Skalarprodukt der normierten Kantenvektoren
            float a0 = acos(std::clamp(scarProd(e1N, e2N), -1.0f, 1.0f));
            float a1 = acos(std::clamp(scarProd(e1N, e3N), -1.0f, 1.0f));
            float a2 = acos(std::clamp(scarProd(e2N, e3N * (-1.0f)), -1.0f, 1.0f));
    
            // Aufsummierung der Normaleneckpunkte des Dreiecks (mit Berücksichtigung der Winkel)
            normals[v0] += crossProduct * a0;
            normals[v1] += crossProduct * a1;
            normals[v2] += crossProduct * a2;
        } else {
            // Aufsummierung der Normaleneckpunkte des Dreiecks (ohne Berücksichtigung der Winkel)
            normals[v0] += crossProduct;
            normals[v1] += crossProduct;
            normals[v2] += crossProduct;
        }
        
        /*
         * Frage aus Aufgabenblatt: Warum sind die Eckpunkt-Normalen durch diese Methode nach Dreiecksfläche gewichtet?
         * Antwort: größere Dreiecke haben größere Normalenvektoren. Beim Aufsummieren tragen die größeren Dreiecke mehr zur Eckpunkt-Normale beia ls kleinere Dreiecke. 
         * -> tragen propotional zum Winkel bei -> bessere Glättung
         */
    }

    // Eckpunkt-Normalen werden normiert
    for (auto &normal : normals) {
        if (!normal.normalize()) {
            cerr << "Warning: Normalization failed!" << endl;
        }
    }
}

// ================
// === RAW DATA ===
// ================

vector<TriangleMesh::Vertex> &TriangleMesh::getPoints()
{
    return vertices;
}
vector<TriangleMesh::Triangle> &TriangleMesh::getTriangles()
{
    return triangles;
}

vector<TriangleMesh::Normal> &TriangleMesh::getNormals()
{
    return normals;
}

const vector<TriangleMesh::Vertex> &TriangleMesh::getPoints() const
{
    return vertices;
}

const vector<TriangleMesh::Triangle> &TriangleMesh::getTriangles() const
{
    return triangles;
}

const vector<TriangleMesh::Normal> &TriangleMesh::getNormals() const
{
    return normals;
}

void TriangleMesh::flipNormals()
{
    for (auto &normal : normals) {
        normal *= -1.0;
    }
}

// =================
// === LOAD MESH ===
// =================


// === Hilfsmethoden ===

// In beiden Datentypen (OFF, LSA) wird anfangs nach selbem Schema die Datei geprüft. Das wurde in diese Methode ausgelagert.
bool validateFile(std::ifstream &in, const char* filename, const char* expectedHeader) {
    
    if (!in.is_open()) {
        cout << "Error: Cannot open " << filename << endl;
        return false;
    }

    char s[256];
    in >> s;
    
    // Expected Header ist hierbei entweder LSA oder OFF
    if (!(s[0] == expectedHeader[0] && s[1] == expectedHeader[1] && s[2] == expectedHeader[2])) {
        std::cerr << "Error: Invalid header in " << filename << " (expected: " << expectedHeader<< ")" << std::endl;
        return false;
    }

    return true;
}

// Das Einlesen von Dreiecken ist bei LSA und OFF gleich, deswegen wurde es in diese Methode ausgelagert
void TriangleMesh::loadTriangles(std::ifstream& in, int faceCount) {
    for (int i = 0; i < faceCount; i++) {
        int vertexCount;
        in >> vertexCount;

        if (vertexCount == 3) {
            int v1, v2, v3;
            in >> v1 >> v2 >> v3;
            triangles.emplace_back(v1, v2, v3);
        } else {
            std::cerr << "Warnung: Face " << i << " wurde übersprungen, da es sich nicht um ein Dreieck handelt. -> (Anzahl gegebener Knoten: " << vertexCount << ")\n";

            // Lese die restlichen Vertices ein, um mit der nächsten Zeile fortsetzen zu können
            for (int j = 0; j < vertexCount; ++j) {
                int dummy;
                in >> dummy;
            }
        }
    }
}

// TASK-2: Einlesen von LSA-Dateien
void TriangleMesh::loadLSA(const char *filename)
{
    const constexpr auto DEG_TO_RAD = M_PI / 180.;

    // Datei öffnen
    std::ifstream in(filename);

    // Datei validieren
    if (!validateFile(in, filename, "LSA")) {
        return;
    }

    // Anzahl der Knoten (nodeCount), Faces (faceCount), Kanten (edgeCount) und Baseline auslesen
    int nodeCount, faceCount, edgeCount;
    float baseline;
    in >> nodeCount;
    in >> faceCount;
    in >> edgeCount;
    in >> baseline;

    // Wenn keine Knoten oder Faces vorhanden sind, return
    if (nodeCount <= 0 || faceCount <= 0)
        return;

    // Sicherstellen, dass alle Knoten eingelesen werden können in Vektor 
    vertices.reserve(nodeCount);

    // TODO: 2) Alpha-, Beta- und Gammawerte einlesen und Knotenkoordinaten berechnen
    for (int i = 0; i < nodeCount; i++) {
        float a, b, c; // alpha, beta, gamma
        in >> a >> b >> c;

        // Radientwerte ausrechnen
        float aRad = a * DEG_TO_RAD;
        float bRad = b * DEG_TO_RAD;
        float cRad = c * DEG_TO_RAD;

        float x, y, z;

        // Formeln aus der Theorieübung verwenden um die Knotenkoordinaten zu berechnen
        z = baseline / (tan(aRad) + tan(bRad));
        x = z * tan(bRad);
        y = z * tan(cRad);

        vertices.emplace_back(x, y, z);
    }

    // Sicherstellen, dass alle Faces eingelesen werden können in den Vektor
    triangles.reserve(faceCount);

    // TODO: 2) Alle Dreiecke in die Datei einlesen

    // Dreieck-Laden ausgelagert
    loadTriangles(in, faceCount);

    // Normalen berechnen
    calculateNormals();
}

// TASK-1: Einlesen von OFF-Dateien

void TriangleMesh::loadOFF(const char *filename)
{
    // Datei öffnen
    std::ifstream in(filename);

    // Datei validieren
    if (!validateFile(in, filename, "OFF")) {
        return;
    }

    // Anzahl der Knoten (nodeCount), Faces (faceCount), Kanten (edgeCount) auslesen
    unsigned int nodeCount, faceCount, edgeCount;
    in >> nodeCount;
    in >> faceCount;
    in >> edgeCount;

    // Wenn keine Knoten oder Faces vorhanden sind, return
    if (nodeCount <= 0 || faceCount <= 0)
        return;

    // Sicherstellen, dass alle Knoten eingelesen werden können in Vektor 
    vertices.reserve(nodeCount);

    // TODO: 1) Alle Knoten von der Datei auslesen
    for (int i = 0; i < nodeCount; i++) {
        float x, y, z;
        in >> x >> y >> z; // Splitte Zeile in drei Koordinatenpunkte (repräsentiert den Knoten)
        vertices.emplace_back(x, y, z); // push_back fügt den Inputparameter so wie er ist hinzu, emplace_back erstellt im Container ein Objekt
    }

    // Sicherstellen, dass alle Faces eingelesen werden können in den Vektor
    triangles.reserve(faceCount);
    // TODO: 1) All Faces/Dreiecke von der Datei auslesen

    // Dreieck-Laden ausgelagert
    loadTriangles(in, faceCount);

    // Normalen berechnen
    calculateNormals();
}

// ==============
// === RENDER ===
// ==============

// Hilfsmethode, die die Normalen als Linien zeichnet
void TriangleMesh::drawNormals(QOpenGLFunctions_2_1* f) {

    // Wenn die Checkbox nicht True ist, werden die Normalen nicht gezeichnet
    if (!renderNormals)
        return;

    float scale = 0.1f;

    if (normals.empty() || vertices.empty())
        return;

    // Farbe der Normalen wird auf Blau gesetzt, um diese von den Modellen unterscheiden zu können
    f->glColor3f(0.0f, 0.0f, 1.0f); // Rot: 0, Grün: 0, Blau: 1

    f->glBegin(GL_LINES);

    for (size_t i = 0; i < vertices.size(); ++i) {
        const auto &v = vertices[i]; 
        const auto &n = normals[i];

        // Beginne Linie bei Punkt des Knotens
        f->glVertex3f(v.x(), v.y(), v.z());
        
        // Ende Linie
        f->glVertex3f(v.x() + n.x() * scale, v.y() + n.y() * scale, v.z() + n.z() * scale);
        
    }
    f->glEnd();
}

// TASK-3: Zeichnen von Dreiecken im immediate mode

void TriangleMesh::draw(QOpenGLFunctions_2_1 *f)
{
    if (triangles.empty())
        return;

    // TODO: 3) draw triangles with immediate mode

    f->glBegin(GL_TRIANGLES);

    // Iteriere über jedes Dreieck und Zeichne die Vertices
    for (const auto &triangle : triangles) {
        for (int i = 0; i < 3; i++) {
            int vI = triangle[i];

            const auto &v = vertices[vI];

            f->glVertex3f(v.x(), v.y(), v.z());
        }
    }
    f->glEnd();
}