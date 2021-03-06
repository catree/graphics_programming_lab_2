#ifndef MOUSECONTROLLER_H
#define MOUSECONTROLLER_H

#include <QObject>
#include <QMouseEvent>
#include <QVector2D>
#include <QVector3D>
#include <viewport.h>
#include <camera.h>
#include <model.h>


class MouseController : public QObject
{
    Q_OBJECT

public:
    explicit MouseController(QObject *parent = 0, Viewport *viewport = 0, Camera *camera = 0, Model *model = 0);


signals:
    void updateViewport();
    void setViewportActive(Model::ViewportType type);
    void setClickedId(int x, int y);
    void setActivePrimitive(float id);

public slots:

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void setFeedbackHeightTextureID(GLuint id);

private:
    Viewport *viewport_;
    Camera *camera_;
    float trackballRadius_;
    Model *model_;

    QVector2D lastTranslationPoint_;
    QVector3D lastRotationPoint_;
    QVector2D lastScalingPoint_;

    QVector3D mapPointToTrackball(float x, float y);
    bool checkCollision();
    GLuint feedbackHeightTextureID;
};

#endif // MOUSECONTROLLER_H



