// =========================================================================           //
// Authors: Daniel Rutz, Daniel Ströter, Roman Getto, Matthias Bein                    //
//                                                                                     //
// GRIS - Graphisch Interaktive Systeme                                                //
// Technische Universität Darmstadt                                                    //
// Fraunhoferstrasse 5                                                                 //
// D-64283 Darmstadt, Germany                                                          //
//                                                                                     //
// Content: Main Window class, mouse handling, init of UI signal-slot conns            //
// =========================================================================           //

#include <functional>

#include <QFileDialog>
#include <QMouseEvent>

#include "mainwindow.h"
#include "./ui_mainwindow.h"

void MainWindow::refreshStatusBarMessage() const {
    statusBar()->showMessage(tr("FPS: %1, Triangles: %2").arg(fpsCount).arg(triangleCount));
}

void MainWindow::changeTriangleCount(unsigned int triangles)
{
    triangleCount = triangles;
    refreshStatusBarMessage();
}

void MainWindow::changeFpsCount(unsigned int fps)
{
    fpsCount = fps;
    refreshStatusBarMessage();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->exitButton, &QPushButton::clicked, qApp, &QApplication::exit);
    connect(ui->gridSizeSpinBox, &QSpinBox::valueChanged, ui->openGLWidget, &OpenGLView::setGridSize);
    connect(ui->lightMovementCheckBox, &QCheckBox::clicked, ui->openGLWidget, &OpenGLView::triggerLightMovement);
    connect(ui->planetSceneRenderCheckBox, &QCheckBox::clicked, ui->openGLWidget, &OpenGLView::togglePlanetSceneRender);
    connect(ui->resetViewButton, &QPushButton::clicked, ui->openGLWidget, &OpenGLView::setDefaults);
    connect(ui->drawModeComboBox, &QComboBox::currentIndexChanged, ui->openGLWidget, &OpenGLView::changeRenderMode);
    connect(ui->shaderComboBox, &QComboBox::currentIndexChanged, ui->openGLWidget, &OpenGLView::changeShader);
    connect(ui->loadNewShaderButton, &QPushButton::clicked, this, &MainWindow::openShaderLoadingDialog);

    connect(ui->openGLWidget, &OpenGLView::triangleCountChanged, this, &MainWindow::changeTriangleCount);
    connect(ui->openGLWidget, &OpenGLView::fpsCountChanged, this, &MainWindow::changeFpsCount);
    connect(ui->openGLWidget, &OpenGLView::shaderCompiled, this, &MainWindow::addShaderToList);

    ui->openGLWidget->setGridSize(ui->gridSizeSpinBox->value());

    statusBar()->showMessage(tr("OpenGL-Fenster geöffnet."));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *ev)
{
    mousePos = ev->pos();
}

void MainWindow::mouseMoveEvent(QMouseEvent *ev)
{
    const auto newPos = ev->pos();
    //rotate
    if (ev->buttons() & Qt::LeftButton) {
        ui->openGLWidget->cameraRotates((newPos.x() - mousePos.x()) * mouseSensitivy, (newPos.y() - mousePos.y()) * mouseSensitivy);
    }

    //zoom (here translation in z)
    if (ev->buttons() & Qt::RightButton) {
        ui->openGLWidget->cameraMoves(0.f, 0.f, -(newPos.y() - mousePos.y()) * mouseSensitivy);
    }

    // translation in xy
    if (ev->buttons() & Qt::MiddleButton) {
        ui->openGLWidget->cameraMoves(0.2f * (newPos.x() - mousePos.x()) * mouseSensitivy, -0.2f * (newPos.y() - mousePos.y()) * mouseSensitivy, 0.f);
    }

    mousePos = ev->pos();
}

void MainWindow::openShaderLoadingDialog() {
    const auto vertexShaderFileName = QFileDialog::getOpenFileName(this, QStringLiteral("Vertexshader auswählen"), QString(), QStringLiteral("Vertex Shader File (*.vert)"), nullptr, QFileDialog::DontUseNativeDialog);
    if (vertexShaderFileName.isEmpty()) return;
    const auto fragmentShaderFileName = QFileDialog::getOpenFileName(this, QStringLiteral("Fragmentshader auswählen"), QString(), QStringLiteral("Fragment Shader File (*.frag)"), nullptr, QFileDialog::DontUseNativeDialog);
    
    if (fragmentShaderFileName.isEmpty()) return;

    ui->openGLWidget->compileShader(vertexShaderFileName, fragmentShaderFileName);
}

void MainWindow::addShaderToList(unsigned int index) {
    ui->shaderComboBox->addItem(QStringLiteral("Shader %1").arg(index));
}

