#ifndef GLCDSCENE_H
#define GLCDSCENE_H

#include <QGraphicsScene>
#include "glcd.h"

class MainWindow;
class GlcdScene : public QGraphicsScene
{
    Q_OBJECT

public:
    GlcdScene(Glcd *glcd, QObject* parent = 0);

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
    Glcd *glcd;
    MainWindow *mainWin;
};


#endif // GLCDSCENE_H
