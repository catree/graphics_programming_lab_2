#include "terrain.h"
#include <iostream>
#include <QFile>
#include <QGLWidget>
#include <QImage>

Terrain::Terrain(std::string name, int id, int tesselation, float3 color) :
    Primitive(0, name, id, tesselation, color),
    horizontalScale_(30),
    verticalScale_(30),
    width_(0),
    height_(0),
    nrMaterials_(0)
{
    isTerrain_ = true;
}

void Terrain::parseHeightMap(QString fileName) {
    QFile file(fileName);

    if (file.open(QFile::ReadOnly)) {
        // up to 64 - 1 = 63 ASCII characteres can be read in this line. One is reserved for the \0 termination.
        QByteArray line = file.readLine(64); // do nothing, throw this line away
        line = file.readLine(64);
        width_ = line.trimmed().toInt();
        line = file.readLine(64);
        height_ = line.trimmed().toInt();
        line = file.readLine(64);
        int maxValue = line.trimmed().toInt();
        std::cout << "width: " << width_ << ", height: " << height_ << ", maxValue: " << maxValue << std::endl;

        int sizeOfTexture = width_ * height_;

        // allocate array for height values
        heightValues_ = (unsigned short*) malloc(sizeOfTexture * sizeof(unsigned short));

        unsigned short currentHeightValue = 0;
        char currentByte;
        unsigned short minHeight = 65535;
        unsigned short maxHeight = 0;
        for (uint i = 0; i < sizeOfTexture && !file.atEnd(); i++) {
            // copy first byte into short
            if (!file.getChar(&currentByte)) std::cout << "getChar() failed at pos " << i << std::endl;
            currentHeightValue = (uchar) currentByte;
            // bit shift by 1 byte
            currentHeightValue = currentHeightValue << 8;

            //std::cout << "byte 1: " << (unsigned short) currentByte;

            // copy second byte into short
            if (!file.getChar(&currentByte)) std::cout << "getChar() failed at pos " << i << std::endl;
            currentHeightValue += (uchar) currentByte;

            heightValues_[i] = currentHeightValue;
            if (heightValues_[i] > maxHeight) maxHeight = heightValues_[i];
            if (heightValues_[i] < minHeight) minHeight = heightValues_[i];

            //std::cout << ", byte 2: " << (unsigned short) currentByte;
            //std::cout << ", height value[" << i << "]: " << currentHeightValue << std::endl;

            //uchar temp = (uchar) ((float) currentHeightValue / 65536.0f * 256.0f);
            //uchar temp = (uchar) currentHeightValue;
//                std::cout << "tmp: " << (uint) temp << std::endl;
            // copy into float array
            //floatData_[i] =  temp; // == 2^16
        }

        std::cout << "minHeight: " << minHeight << ", maxHeight: " << maxHeight << std::endl;

        for (uint i = 0; i < sizeOfTexture; i++) {
            heightValues_[i] -= minHeight;
        }

    } else {
        std::cout << "Failed to open file " << fileName.toStdString() << std::endl;
    }


    createVBO();
}

void Terrain::createTextures() {

    checkGLErrors("before height map upload");

    glGenTextures(1, &heightTexture_);
    glBindTexture(GL_TEXTURE_2D, heightTexture_);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, resolution_[0], resolution_[1], 0, GL_RED, GL_FLOAT, floatData_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16, width_, height_, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, heightValues_);
    glGenerateMipmap(GL_TEXTURE_2D);

    checkGLErrors("after height map upload");

    materialTextures_ = (GLuint*) malloc(4 * sizeof(GLuint));
    glGenTextures(4, materialTextures_);

    QImage material = QGLWidget::convertToGLFormat(QImage(":/terrain/facture_sand.png"));
    glBindTexture(GL_TEXTURE_2D, materialTextures_[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, material.width(), material.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, material.bits());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    material = QGLWidget::convertToGLFormat(QImage(":/terrain/facture_stone.png"));
    glBindTexture(GL_TEXTURE_2D, materialTextures_[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, material.width(), material.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, material.bits());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    material = QGLWidget::convertToGLFormat(QImage(":/terrain/facture_rock_0.png"));
    glBindTexture(GL_TEXTURE_2D, materialTextures_[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, material.width(), material.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, material.bits());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    material = QGLWidget::convertToGLFormat(QImage(":/terrain/facture_rock_1.png"));
    glBindTexture(GL_TEXTURE_2D, materialTextures_[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, material.width(), material.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, material.bits());
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    checkGLErrors("after uploading material textures");
}

void Terrain::bindVAOToShader() {


    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferPositions_);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );



    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);



    // bind volume and transfer textures
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, heightTexture_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, materialTextures_[0]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, materialTextures_[1]);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, materialTextures_[2]);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, materialTextures_[3]);
    glActiveTexture(GL_TEXTURE0);


}




void Terrain::draw() {

    bindVAOToShader();

    glDrawElements(
        GL_QUADS,      // mode
        indicesList_.size(),    // count
        GL_UNSIGNED_INT,       // type
        (void*)0           // element array buffer offset
    );

}

void Terrain::createVBO() {

    vertexPositions_.resize(0);
    indicesList_.resize(0);



    for (int i = -verticalScale_; i < verticalScale_; i++) {
        for (int j = -horizontalScale_; j < horizontalScale_; j++) {
            vertexPositions_.push_back(float3(i, 0, j));
            vertexPositions_.push_back(float3(i, 0, j + 1));
            vertexPositions_.push_back(float3(i + 1, 0, j + 1));
            vertexPositions_.push_back(float3(i + 1, 0, j));

            /*
            vertexPositions_.push_back(float3(i / (float) 20, heightValues_[(i + verticalScale_) * 4096 + (j + horizontalScale_)] / (float) 200, j / (float) 20));
            vertexPositions_.push_back(float3(i / (float) 20, heightValues_[(i + verticalScale_)* 4096 + (j + horizontalScale_) + 1] / (float) 200, (j + 1) / (float) 20));
            vertexPositions_.push_back(float3((i + 1) / (float) 20, heightValues_[((i + verticalScale_)+ 1) * 4096 + (j + horizontalScale_) + 1] / (float) 200, (j + 1) / (float) 20));
            vertexPositions_.push_back(float3((i + 1) / (float) 20, heightValues_[((i + verticalScale_)+ 1) * 4096 + (j + horizontalScale_)] / (float) 200, j / (float) 20));
            */
        }
    }

    // set indices list
    for (uint i = 0; i < vertexPositions_.size(); i++) {
        indicesList_.push_back(i);
    }

}

void Terrain::setHorizontalScale(int horizontalScale) {
    horizontalScale_ = horizontalScale;
    createVBO();
}

void Terrain::setVerticalScale(int verticalScale) {
    verticalScale_ = verticalScale;
    createVBO();
}

void Terrain::uploadMaterial(QString fileName) {
    // TODO
}

void Terrain::changeRange(int materialID, int minRange, int maxRange) {
    // TODO
}

void Terrain::checkGLErrors(const char *label) {
    GLenum errCode;
    const GLubyte *errStr;
    if ((errCode = glGetError()) != GL_NO_ERROR) {
        errStr = gluErrorString(errCode);
        printf("OpenGL ERROR: ");
        printf((char*)errStr);
        printf("(Label: ");
        printf(label);
        printf(")\n.");
    }
}
