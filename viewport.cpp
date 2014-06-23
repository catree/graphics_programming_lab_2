#include <GL/glew.h>
#include "viewport.h"
#include <camera.h>
#include "GL/glu.h"
#include <QMatrix4x4>
#include <model.h>
#include <iostream>
#include <cube.h>
#include <torus.h>
#include <cone.h>
#include <cylinder.h>
#include <grid.h>
#include <primitive.h>
#include <volume.h>
#include <terrain.h>

inline void glMultMatrix(const GLfloat* matrix) { glMultMatrixf(matrix); }
inline void glMultMatrix(const GLdouble* matrix) { glMultMatrixd(matrix); }

Viewport::Viewport(QWidget *parent, QGLFormat format, Model::ViewportType type, Model *model) :
    QGLWidget(format, parent)
{
    type_ = type;
    model_ = model;

    showGrid_ = false;
    mip_ = false;

    gridSize_ = 5;
    stepSize_ = 1;


    // set up default Camera
    camera_ = new Camera();
}


QSize Viewport::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize Viewport::sizeHint() const
{
    return QSize(800, 800);
}

void Viewport::initializeGL()
{
    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        exit(EXIT_FAILURE);
    }

    // enable depth testing
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);

    // enable 3D textures
    glEnable(GL_TEXTURE_3D_EXT);

    // init shading model to flat shading
    glShadeModel(GL_FLAT);

    // enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // position lightsource
    light0Position_[0] = 1.0f;
    light0Position_[1] = 1.0f;
    light0Position_[2] = 3.5f;
    light0Position_[3] = 1.0f;

    // color lightsource
    GLfloat light0Ambient[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    GLfloat light0Diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat light0Specular[4] = {0.6f, 0.6f, 0.6f, 1.0f};

    // Assign created components to GL_LIGHT0
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0Specular);

    //float specularReflection[4] = {1.0, 1.0, 1.0, 1.0};
    int shininess = 120.0f;

    //glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specularReflection);
    glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, shininess);


    // set clear color
    glClearColor(0, 0.5, 0.5, 1);

    // set patch vertices for tesselation shader
    GLint maxPatchVertices = 0;
    glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
    std::cout << "Max supported patch vertices: " << maxPatchVertices << std::endl;
    glPatchParameteri(GL_PATCH_VERTICES, 4);    // we want to tesselate quads



    // ==== CREATE FRAMEBUFFER AND ITS TEXTURES ==== //

    // create a texture for the colors
    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width(), height(), 0, GL_BGRA, GL_FLOAT, 0);

    // create a texture for the object IDs
    glGenTextures(1, &idTexture_);
    glBindTexture(GL_TEXTURE_2D, idTexture_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width(), height(), 0, GL_RED, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // create a renderbuffer object to store depth info - openGL needs this for depth test
    glGenRenderbuffersEXT(1, &depthTexture_);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthTexture_);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width(), height());
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

    // create a framebuffer object
    glGenFramebuffersEXT(1, &framebuffer_);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_);

    // attach the textures to FBO color attachment points
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,        // 1. fbo target: GL_FRAMEBUFFER
                           GL_COLOR_ATTACHMENT0_EXT,  // 2. attachment point
                           GL_TEXTURE_2D,         // 3. tex target: GL_TEXTURE_2D
                           colorTexture_,             // 4. tex ID
                           0);                    // 5. mipmap level: 0(base)

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,        // 1. fbo target: GL_FRAMEBUFFER
                           GL_COLOR_ATTACHMENT1_EXT,  // 2. attachment point
                           GL_TEXTURE_2D,         // 3. tex target: GL_TEXTURE_2D
                           idTexture_,             // 4. tex ID
                           0);                    // 5. mipmap level: 0(base)

    // attach the renderbuffer to depth attachment point
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,      // 1. fbo target: GL_FRAMEBUFFER
                              GL_DEPTH_ATTACHMENT_EXT, // 2. attachment point
                              GL_RENDERBUFFER_EXT,     // 3. rbo target: GL_RENDERBUFFER
                              depthTexture_);     // 4. rbo ID

    checkFramebufferStatus();
    // define the index array for the outputs
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
    glDrawBuffers(2,  attachments);

    // ============================================================ //

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // compile and link shaders
    // PHONG SHADER
    phongProgram = new QGLShaderProgram(this);

    phongVertexShader = new QGLShader(QGLShader::Vertex, this);
    phongFragmentShader = new QGLShader(QGLShader::Fragment, this);

    phongVertexShader->compileSourceFile(":/shaders/phongVertexShader.vertexShader");
    phongFragmentShader->compileSourceFile(":/shaders/phongFragmentShader.fragmentShader");

    phongProgram->addShader(phongVertexShader);
    phongProgram->addShader(phongFragmentShader);
    phongProgram->link();

    // SELECTION SHADER

    selectionProgram = new QGLShaderProgram(this);
    selectionVertexShader = new QGLShader(QGLShader::Vertex, this);
    selectionFragmentShader = new QGLShader(QGLShader::Fragment, this);

    selectionVertexShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/selectionVertexShader.vertexShader");
    selectionFragmentShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/selectionFragmentShader.fragmentShader");

    selectionProgram->addShader(selectionVertexShader);
    selectionProgram->addShader(selectionFragmentShader);
    selectionProgram->link();

    // GRID SHADER

    gridProgram = new QGLShaderProgram(this);
    gridVertexShader = new QGLShader(QGLShader::Vertex, this);
    gridFragmentShader = new QGLShader(QGLShader::Fragment, this);

    gridVertexShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/gridVertexShader.vertexShader");
    gridFragmentShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/gridFragmentShader.fragmentShader");

    gridProgram->addShader(gridVertexShader);
    gridProgram->addShader(gridFragmentShader);
    gridProgram->link();

    // VOLUME SHADER

    volumeProgram = new QGLShaderProgram(this);
    volumeVertexShader = new QGLShader(QGLShader::Vertex, this);
    volumeFragmentShader = new QGLShader(QGLShader::Fragment, this);

    volumeVertexShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/volumeVertexShader.vertexShader");
    volumeFragmentShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/volumeFragmentShader.fragmentShader");


    volumeProgram->addShader(volumeVertexShader);
    volumeProgram->addShader(volumeFragmentShader);
    volumeProgram->link();

    // TERRAIN SHADER

    terrainProgram = new QOpenGLShaderProgram(this);
    terrainVertexShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    terrainTesselationControlShader = new QOpenGLShader(QOpenGLShader::TessellationControl, this);
    terrainTesselationEvaluationShader = new QOpenGLShader(QOpenGLShader::TessellationEvaluation, this);
    terrainFragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, this);

    terrainVertexShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/terrainVertexShader.vertexShader");
    terrainTesselationControlShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/terrainTCS.tesselationControlShader");
    terrainTesselationEvaluationShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/terrainTES.tesselationEvaluationShader");
    terrainFragmentShader->compileSourceFile("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/shaders/terrainFragmentShader.fragmentShader");


    terrainProgram->addShader(terrainVertexShader);
    //terrainProgram->addShader(terrainTesselationControlShader);
    //terrainProgram->addShader(terrainTesselationEvaluationShader);
    terrainProgram->addShader(terrainFragmentShader);
    terrainProgram->link();

    // distribute uniform IDs

    phongIdID_ = glGetUniformLocation(phongProgram->programId(), "id");
    phongColorID_ = glGetUniformLocation(phongProgram->programId(), "color");

    idTextureID_ = glGetUniformLocation(selectionProgram->programId(), "idTexture");
    colorTextureID_ = glGetUniformLocation(selectionProgram->programId(), "colorTexture");
    selectionIdID_ = glGetUniformLocation(selectionProgram->programId(), "id");
    offsetXID_ = glGetUniformLocation(selectionProgram->programId(), "offsetX");
    offsetYID_ = glGetUniformLocation(selectionProgram->programId(), "offsetY");
    activeViewportID_ = glGetUniformLocation(selectionProgram->programId(), "active");

    gridColorID_ = glGetUniformLocation(gridProgram->programId(), "color");

    volumeIdID_ = glGetUniformLocation(volumeProgram->programId(), "id");
    volumeTextureID_ = glGetUniformLocation(volumeProgram->programId(), "volumeTexture");
    transferTextureID_ = glGetUniformLocation(volumeProgram->programId(), "transferTexture");
    aspectRatioID_ = glGetUniformLocation(volumeProgram->programId(), "aspectRatio");
    mipID_ = glGetUniformLocation(volumeProgram->programId(), "mip");
    maxResolutionID_ = glGetUniformLocation(volumeProgram->programId(), "maxResolution");

    terrainIdID_ = glGetUniformLocation(terrainProgram->programId(), "id");
    modelMatrixID_ = glGetUniformLocation(terrainProgram->programId(), "modelMatrix");
    projectionMatrixID_ = glGetUniformLocation(terrainProgram->programId(), "projectionMatrix");


    glBindAttribLocation(phongProgram->programId(), 1, "normal_in");
    //glBindAttribLocation(phongProgram->programId(), 2, "color_in");

    //std::cout << "1: " << glGetAttribLocation(phongProgram->programId(), "normal_in") << std::endl;
    //std::cout << "2: " << glGetAttribLocation(phongProgram->programId(), "color_in") << std::endl;


    grid_ = new Grid(Primitive::float3(0.72, 0.72, 0.72), gridSize_, stepSize_);
    grid_->copyVAOToCurrentContext();

    quad_ = new Quad();
    quad_->copyVAOToCurrentContext();
}

void Viewport::paintGL()
{
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_);
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
    glDrawBuffers(2,  attachments);

    // clear framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // set modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMultMatrix(camera_->getCameraMatrix().constData());



    if (showGrid_) {

        glDisable(GL_DEPTH_TEST);
        gridProgram->bind();
        glUniform3f(gridColorID_, grid_->getColor()->x_, grid_->getColor()->y_, grid_->getColor()->z_);
        grid_->draw();
        gridProgram->release();
        glEnable(GL_DEPTH_TEST);
    }


    // DRAW PRIMITIVES

    phongProgram->bind();

    glLightfv(GL_LIGHT0, GL_POSITION, light0Position_);

    QList<Primitive*> *primitives = model_->getScenegraph();

    // draw all primitives that are not a volume
    for (int i = 0; i < primitives->size(); i++) {
        if (!primitives->at(i)->isVolume() && !primitives->at(i)->isTerrain()) {
            glUniform1f(phongIdID_, primitives->at(i)->getID());
            glUniform3f(phongColorID_, primitives->at(i)->getColor()->x_, primitives->at(i)->getColor()->y_, primitives->at(i)->getColor()->z_);

            glPushMatrix();
            glMultMatrix(primitives->at(i)->getModelMatrix().constData());

            primitives->at(i)->draw();

            glPopMatrix();
        }
    }


    // DRAW VOLUMES

    phongProgram->release();
    volumeProgram->bind();

    for (int i = 0; i < primitives->size(); i++) {
        if (primitives->at(i)->isVolume()) {
            Volume* volume = static_cast<Volume*>(primitives->at(i));
            glUniform1f(volumeIdID_, primitives->at(i)->getID());
            glUniform1i(mipID_, mip_);
            glUniform1i(maxResolutionID_, volume->getMaxResolution());
            // volume texture will be in GL_TEXTURE1. It is bound in the volume's draw method.
            glUniform1i(volumeTextureID_, 1);
            // transfer texture will be in GL_TEXTURE2. It is bound in the volume's draw method.
            glUniform1i(transferTextureID_, 2);
            glUniform3f(aspectRatioID_, volume->getAspectRatio().x_, volume->getAspectRatio().y_, volume->getAspectRatio().z_);

            glPushMatrix();
            glMultMatrix(primitives->at(i)->getModelMatrix().constData());

            primitives->at(i)->draw();

            glPopMatrix();
        }
    }

    // DRAW TERRAINS

    volumeProgram->release();
    terrainProgram->bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (int i = 0; i < primitives->size(); i++) {
        if (primitives->at(i)->isTerrain()) {

            glUniform1f(terrainIdID_, primitives->at(i)->getID());

            // ===== POTENTIAL ISSUE ===== //
            // TODO could constData return double* on different architecture??
            float* viewMatrix = (float*) malloc(16 * sizeof(float));
            glGetFloatv(GL_MODELVIEW_MATRIX, viewMatrix);   // column-major
            QMatrix4x4 modelViewMatrix = QMatrix4x4(viewMatrix).transposed() * primitives->at(i)->getModelMatrix();
            glUniformMatrix4fv(modelMatrixID_, 1, GL_TRUE, modelViewMatrix.constData());

            float* matrix = (float*) malloc(16 * sizeof(float));
            glGetFloatv(GL_PROJECTION_MATRIX, matrix);  // column-major
            QMatrix4x4 projectionMatrix = QMatrix4x4(matrix).transposed();  // row-major
            glUniformMatrix4fv(projectionMatrixID_, 1, GL_TRUE, projectionMatrix.constData());  // GL_TRUE accepts row-major

            // TODO Maybe start with some simple examples...it cannot be that hard!!!

            std::cout << "ModelViewMatrix[" << type_ << "]: " << std::endl;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    std::cout << "[" << i*4+j << "]: " << modelViewMatrix.constData()[i*4+j] << "  ";
                }
                std::cout << std::endl;
            }
            std::cout << "ProjectionMatrix[" << type_ << "]: " << std::endl;
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    std::cout << "[" << i*4+j << "]: " << projectionMatrix.constData()[i*4+j] << "  ";
                }
                std::cout << std::endl;

            }

            primitives->at(i)->draw();
        }
    }



    // HIGHLIGHT SELECTED OBJECT

    terrainProgram->release();
    selectionProgram->bind();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glUniform1i(colorTextureID_, 1);


    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, idTexture_);
    glUniform1i(idTextureID_, 2);

    float id;


    if (model_->getActivePrimitive() != NULL) {
        id = model_->getActivePrimitive()->getID();
    } else {
        id = 0;
    }


    bool viewportActive = model_->isActiveViewport(type_);


    glUniform1f(selectionIdID_, id);
    glUniform1f(offsetXID_, 1.0f / width());
    glUniform1f(offsetYID_, 1.0f / height());
    glUniform1i(activeViewportID_, viewportActive);

    quad_->draw();

    selectionProgram->release();

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);


}

void Viewport::resizeGL(int width, int height)
{

    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_FLOAT, 0);

    glBindTexture(GL_TEXTURE_2D, idTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthTexture_);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, width, height);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);


    glViewport(0, 0, width, height);

    // set projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (camera_->getProjectionMode() == Camera::PERSPECTIVE) {
        if (height != 0) gluPerspective(45.0d, ((double) width) / ((double) height), NEAR_PLANE, FAR_PLANE);
    } else if (camera_->getProjectionMode() == Camera::ORTHOGRAPHIC) {
        updateProjectionMatrix();
    }
}


// ====================== SETTERS ========================= //

void Viewport::setCamera(Camera *camera) {
    camera_ = camera;
    connect(camera_, SIGNAL(zoomChanged()), this, SLOT(updateProjectionMatrix()));
}

// ====================== GETTERS ========================= //

Model::ViewportType Viewport::getType() {
    return type_;
}

// ====================== SLOTS =========================== //

void Viewport::setClickedId(int x, int y) {
    makeCurrent();

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_);
    GLfloat pixels[4];
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(x, y, 1, 1, GL_RED, GL_FLOAT, pixels);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

    float id = pixels[0];


    // do not select background when in object interaction mode
    if (model_->getInteractionMode() != Model::OBJECT || id != 0) {
        emit setActivePrimitive(id);
    }

}

void Viewport::copyVAOData(Primitive *p) {
    makeCurrent();
    p->copyVAOToCurrentContext();
    if (p->isTerrain()) {
        static_cast<Terrain*>(p)->createTextures();
    }
}

void Viewport::copyVolumeData(Volume *volume) {
    if (volume != NULL) {
        makeCurrent();
        volume->uploadTransferFunction();
    }
    updateGL();

}

void Viewport::copyVolumeData() {
    QList<Primitive*> *primitives = model_->getScenegraph();
    for (int i = 0; i < primitives->size() && primitives->at(i)->isVolume(); i++) {
        makeCurrent();
        static_cast<Volume*>(primitives->at(i))->uploadTransferFunction();
    }
    updateGL();
}

void Viewport::updateProjectionMatrix() {

    if (camera_->getProjectionMode() == Camera::ORTHOGRAPHIC) {

        makeCurrent();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        float zoom = camera_->getZoom();
        float aspectRatio = 0;
        if (height() != 0) aspectRatio = (float) width() / height();
        float goodZoomFactor = zoom / 10;

        glOrtho(- aspectRatio * (1 - goodZoomFactor), aspectRatio * (1 - goodZoomFactor), -1 + goodZoomFactor, 1 - goodZoomFactor, NEAR_PLANE, FAR_PLANE);
    }
}


bool Viewport::checkFramebufferStatus() {
    GLenum status;
    status=(GLenum)glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
    switch(status) {
        case GL_FRAMEBUFFER_COMPLETE_EXT:
            return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
            printf("Framebuffer incomplete,incomplete attachment\n");
            return false;
        case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
            printf("Unsupported framebuffer format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
            printf("Framebuffer incomplete,missing attachment\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
            printf("Framebuffer incomplete,attached images must have same dimensions\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
             printf("Framebuffer incomplete,attached images must have same format\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
            printf("Framebuffer incomplete,missing draw buffer\n");
            return false;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
            printf("Framebuffer incomplete,missing read buffer\n");
            return false;
    }
    return false;
}

void Viewport::showGrid(bool on) {
    showGrid_ = on;

    updateGL();
}

void Viewport::setMip(bool on) {
    mip_ = on;

    updateGL();
}

void Viewport::setGridSize(int i) {
    makeCurrent();
    grid_->setGridSize(i);
    updateGL();

}

void Viewport::setStepSize(int i) {
    makeCurrent();
    grid_->setStepSize(i);
    updateGL();

}
