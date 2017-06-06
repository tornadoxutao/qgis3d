#ifndef WINDOW3D_H
#define WINDOW3D_H

#include <Qt3DExtras>

#include <QTimer>

class CameraController;
class MapTextureGenerator;
class SidePanel;

class Window3D : public Qt3DExtras::Qt3DWindow
{
public:
  Window3D(SidePanel* p, MapTextureGenerator* mapGen);

  Qt3DCore::QEntity *createScene();

protected:
  void resizeEvent(QResizeEvent *ev) override;

private slots:
  void onTimeout();
  void onFrameTriggered(float dt);
  void onCameraViewMatrixChanged();

private:
  CameraController* cc;
  SidePanel* panel;
  MapTextureGenerator* mapGen;   //!< provider of rendered map tiles

  QTimer timer;
  int frames = 0;
  Qt3DLogic::QFrameAction* mFrameAction;
};

#endif // WINDOW3D_H