#ifndef GLCD_H
#define GLCD_H

#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QRect>
#include <QPoint>

class Glcd
{
public:
    Glcd(int width, int height, int pixelWidth, int pixelHeight, int spaceWidth, int spaceHeight);
    ~Glcd();
    void setPixelSize(int pixWidth, int pixHeight);
    void setSpaceSize(int sWidth, int sHeight);

    QSize size() { return QSize(width, height); }
    QPixmap getPixmap() { return QPixmap::fromImage(*image); }
    QSize pixmapSize() { return image->size(); }

    void setFont(uchar **newFont);
    void drawBitmap(int x, int y, int bmWidth, int bmHeight, uchar *bitmap);
    void drawImage(int x, int y, uchar *image);
    int drawChar(int x, int y, uchar ch);
    void drawStr(int x, int y, const char *str);
    void drawPixel(int x, int y, bool color);
    void drawLine(int x0, int y0, int x1, int y1, bool color);
    void fillMem(uchar data);
    void renderMem();
    void printMem();

    QPoint translatePos(QPoint pos);

private:
    void createImage();
    void renderPixel(int x, int y, bool color);

    QImage *image;
    QPainter *painter;
    int width, height, memWidth;
    int pixelWidth, pixelHeight;
    int spaceWidth, spaceHeight;
    uchar **mem;
    uchar **font;
};


#endif // GLCD_H
