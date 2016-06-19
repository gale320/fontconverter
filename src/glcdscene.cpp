#include "glcdscene.h"
#include "mainwindow.h"
#include <QGraphicsSceneMouseEvent>


GlcdScene::GlcdScene(Glcd *glcd, QObject* parent):
    glcd(glcd), QGraphicsScene(parent)
{
    mainWin = qobject_cast<MainWindow*>(parent);
}

void GlcdScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QPoint pos = glcd->translatePos(event->scenePos().toPoint());
    mainWin->pixelHovered(pos);
}

