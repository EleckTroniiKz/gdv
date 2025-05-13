// ========================================================================= //
// Authors: Daniel Rutz, Daniel Ströter, Roman Getto, Matthias Bein          //
//                                                                           //
// GRIS - Graphisch Interaktive Systeme                                      //
// Technische Universität Darmstadt                                          //
// Fraunhoferstrasse 5                                                       //
// D-64283 Darmstadt, Germany                                                //
//                                                                           //
// Content: Initialisation: Set basic parameters and create OpenGL window    //
// ========================================================================= //

#include <QApplication>
#include <QSurfaceFormat>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    //Change default QSurfaceFormat in order to enforce OpenGL version required for the exercise
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
    format.setDepthBufferSize(24); //Enable depth buffer
    format.setVersion(3, 3);
    //format.setSwapBehavior(QSurfaceFormat::SwapBehavior::DoubleBuffer); //Enable VSync
    format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
    //format.setOption(QSurfaceFormat::FormatOption::DeprecatedFunctions);
    //format.setOption(QSurfaceFormat::FormatOption::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
