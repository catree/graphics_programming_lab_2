#include <GL/glew.h>
#include "view.h"
#include <model.h>
#include <controller.h>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    /* Model-View-Controller pattern.
     * The Controller knows the View and the Model. It can change the data stored in
     * the model and it can select and change the view that should present this data.
     * The View knows the Model as it has to call several get() functions to get the
     * data it should represent.
     * The Model needs to notify the View when the data has changed. This is done via
     * a signal that the model emits. It is connected here to a slot in the View.
     * Similarly, the View needs to notify the Controller of user inputs and can
     * emit several signals which are dealt with in the respective slots of the Controller.
     */
    Model model;
    View view;
    Controller controller;

    view.setModel(&model);

    controller.setModel(&model);
    controller.setView(&view);
    controller.createMouseControllers();




    view.show();

    model.addCube();
    model.addCube();
    model.addCube();
    model.addCube();
    model.addCube();
    model.addCube();
    model.addCube();

    model.getScenegraph()->at(0)->translate(QVector3D(0, 4, 0));
    model.getScenegraph()->at(1)->translate(QVector3D(0, 5, 0));
    model.getScenegraph()->at(2)->translate(QVector3D(0, 6, 0));
    model.getScenegraph()->at(3)->translate(QVector3D(0, 7, 0));
    model.getScenegraph()->at(4)->translate(QVector3D(0, 8, 0));
    model.getScenegraph()->at(5)->translate(QVector3D(0, 9, 1));

    model.addTerrain(QString("/home/claudia/OpenGL Praktikum/Assignment 4/Terrain Modeling Tool/terrain/grand_canyon.pgm"));


    return a.exec();
}
