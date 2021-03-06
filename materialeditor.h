#ifndef MATERIALEDITOR_H
#define MATERIALEDITOR_H


#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalMapper>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QPixmap>
#include <primitive.h>
#include <materialpreview.h>

class MaterialEditor : public QWidget
{
    Q_OBJECT
public:
    explicit MaterialEditor(QWidget *parent = 0, MaterialPreview *preview = 0);
    //~MaterialEditor();
    enum Component {RED, GREEN, BLUE};
    static const int NR_COMPONENTS = 3;

    void adjustToPrimitive(Primitive* p);



signals:
    void ambientColorChanged(uint red, uint green, uint blue);
    void diffuseColorChanged(uint red, uint green, uint blue);
    void specularColorChanged(uint red, uint green, uint blue);
    void roughnessChanged(int roughness);
    void roughnessSpecularChanged(int roughness);
    void refractionIndexChanged(int refractionIndex);
    void textureChecked(Primitive::Textures x, bool status);
    void textureUploaded(Primitive::Textures x, QImage texture);
    void diffuseShaderChanged(int id);
    void specularShaderChanged(int id);
    void kaChanged(int ka);
    void kdChanged(int kd);
    void ksChanged(int ks);

public slots:
    void ambientComponentChanged(QString value);
    void diffuseComponentChanged(QString value);
    void specularComponentChanged(QString value);
    void textureChecked(bool status);
    void textureUploadInitialized();
    void loadMaterial();
    void saveMaterial();
    void diffuseShaderChangedSlot(int id);
    void specularShaderChangedSlot(int id);
    void kaChangedSlot(int value);
    void kdChangedSlot(int value);
    void ksChangedSlot(int value);

private:

    QGridLayout *gridLayout;

    // material preview
    MaterialPreview *preview_;

    // Labels first row
    QLabel *componentLabels[NR_COMPONENTS];

    // ambient color
    QLabel *ambientLabel;
    QLineEdit *ambientEdits[NR_COMPONENTS];
    uint ambientColor[NR_COMPONENTS];

    // diffuse color
    QLabel *diffuseLabel;
    QLineEdit *diffuseEdits[NR_COMPONENTS];
    uint diffuseColor[NR_COMPONENTS];

    // specular color
    QLabel *specularLabel;
    QLineEdit *specularEdits[NR_COMPONENTS];
    uint specularColor[NR_COMPONENTS];

    // ka, kd, ks factors
    QLabel *factorsLabel;
    QSlider *kaSlider;
    QSlider *kdSlider;
    QSlider *ksSlider;

    // roughness diffuse
    QLabel *roughnessLabel;
    QSlider *roughnessSlider;

    // roughness specular
    QLabel *roughnessSpecularLabel;
    QSlider *roughnessSpecularSlider;


    // refraction index for fresnel term
    QLabel *refractionIndexLabel;
    QSlider *refractionIndexSlider;

    // texture uploads
    QLabel *texturesLabel;
    QCheckBox *textureCheckboxes[Primitive::NR_TEXTURES];
    QLabel *textureLabels[Primitive::NR_TEXTURES];
    QPushButton *textureUploadButtons[Primitive::NR_TEXTURES];
    QLabel *texturePreviewsLabels[Primitive::NR_TEXTURES];
    QPixmap *texturePreviewPixmaps[Primitive::NR_TEXTURES];
    QImage *textureImage[Primitive::NR_TEXTURES];
    bool hasTexture[Primitive::NR_TEXTURES];

    // shader buttons
    QLabel *shadersLabel;

    // QButtonGroup *diffuseShadersGroup;
    QButtonGroup *diffuseShadersGroup;
    QLabel *diffuseShadersLabel;
    QPushButton *noDiffuseButton;
    QPushButton *lambertButton;
    QPushButton *orenNayarButton;

    QButtonGroup *specularShadersGroup;
    QLabel *specularShadersLabel;
    QPushButton *noSpecularButton;
    QPushButton *phongButton;
    QPushButton *cookTorranceButton;

    // load and save buttons
    QPushButton *loadButton;
    QPushButton *saveButton;

    bool changedKd;
    bool changedKs;
    bool changedKa;

    void uploadTexture(int index, QString fileName);
    void resetProperties();
    QString componentToString(Component x);
    QString textureToString(Primitive::Textures x);
};

#endif // MATERIALEDITOR_H
