#include "glcd.h"
#include <QPainter>
#include <QDebug>


Glcd::Glcd(int width, int height, int pixelWidth, int pixelHeight, int spaceWidth, int spaceHeight):
    image(NULL),
    width(width), height(height), memWidth(width/8),
    pixelWidth(pixelWidth), pixelHeight(pixelHeight),
    spaceWidth(spaceWidth), spaceHeight(spaceHeight)
{
    mem = new uchar*[height];
    for (int i = 0; i < height; i++)
    {
        mem[i] = new uchar[memWidth]();
    }
    font = NULL;

    painter = new QPainter;
    createImage();
    renderMem();
}

Glcd::~Glcd()
{
    for (int i = 0; i < height; i++)
    {
        delete [] mem[i];
    }
    delete [] mem;

    setFont(NULL);

    delete painter;
    delete image;
}

void Glcd::createImage()
{
    delete image;
    int imgWidth = width*(pixelWidth+spaceWidth)+spaceWidth;
    int imgHeight = height*(pixelHeight+spaceHeight)+spaceHeight;
    image = new QImage(imgWidth, imgHeight, QImage::Format_RGB32);
    image->fill(Qt::white);
}

void Glcd::setPixelSize(int pixWidth, int pixHeight)
{
    pixelWidth = pixWidth;
    pixelHeight = pixHeight;
    createImage();
    renderMem();
}

void Glcd::setSpaceSize(int sWidth, int sHeight)
{
    spaceWidth = sWidth;
    spaceHeight = sHeight;
    createImage();
    renderMem();
}

void Glcd::fillMem(uchar data)
{
    for (int i = 0; i < height; i++)
    {
        memset(mem[i], data, memWidth);
    }
}

void Glcd::renderMem()
{
    painter->begin(image);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < memWidth; x++)
        {
            for (int bitNum = 0; bitNum < 8; bitNum++)
            {
                int bitMask = 1<<(7-bitNum);
                bool color = (mem[y][x] & bitMask) != 0;
                renderPixel(x*8+bitNum, y, color);
            }
        }
    }
    painter->end();
}

void Glcd::renderPixel(int x, int y, bool color)
{
    int rectX = x*(pixelWidth+spaceWidth)+spaceWidth;
    int rectY = y*(pixelHeight+spaceHeight)+spaceHeight;
    QColor rectColor(color ? Qt::black : Qt::lightGray);
    painter->fillRect(rectX, rectY, pixelWidth, pixelHeight, QBrush(rectColor));
}

void Glcd::printMem()
{
    QString debugStr;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < memWidth; j++)
        {
            debugStr += QString().sprintf("%02x ", mem[i][j]&0xFF);
        }
        debugStr += "\n";
    }
    qDebug() << debugStr;
}

void Glcd::setFont(uchar **newFont)
{
    if (font)
    {
        int first = (int)font[0];
        int last = (int)font[1];
        int size = last-first+1;
        for (int i = 2; i < size+2; i++)
        {
            delete [] font[i];
        }
        delete [] font;
    }
    font = newFont;
}

void Glcd::drawBitmap(int x, int y, int bmWidth, int bmHeight, uchar *bitmap)
{
    int maxBmHeight = height-y;
    if (bmHeight > maxBmHeight)
    {
        bmHeight = maxBmHeight;
    }
    if (bmHeight <= 0)
        return;

    bmWidth /= 8;
    int memX = x/8;
    int bmWidthCpy = bmWidth;
    int maxBmWidth = (memWidth-memX);
    if (bmWidthCpy > maxBmWidth)
    {
        bmWidthCpy = maxBmWidth;
    }
    if (bmWidthCpy <= 0)
        return;

    int i;
    for (i = 0; i < bmHeight; i++, y++)
    {
        memcpy(mem[y]+memX, bitmap, bmWidthCpy);
        bitmap += bmWidth;
    }
}

void Glcd::drawImage(int x, int y, uchar *image)
{
    uchar *imgHeader = image;
    int imgWidth = imgHeader[0];
    int imgHeight = imgHeader[1];
    uchar *bitmap = image+2;
    drawBitmap(x, y, imgWidth, imgHeight, bitmap);
}

int Glcd::drawChar(int x, int y, uchar ch)
{
    if (!font)
        return 0;

    int first = (int)font[0];
    int last = (int)font[1];
    if (ch < first || ch > last)
    {
        qDebug() << "ch" << (int)ch << "first" << first << "last" << last;
        return 0;
    }
    ch -= (first-2);
    uchar *chHeader = font[ch];
    if (!chHeader)
    {
        ch = 2;
        chHeader = font[ch];
        if (!chHeader)
        {
            return 0;
        }
    }

    int chWidth = chHeader[0];
    int chHeight = chHeader[1];
    int yoffset = chHeader[3];
    uchar *chBitmap = font[ch]+4;
    drawBitmap(x, y+yoffset, chWidth, chHeight, chBitmap);
    return chWidth;
}

void Glcd::drawStr(int x, int y, const char *str)
{
    while (*str)
    {
        char ch = *str;
        int chWidth = drawChar(x, y, ch);
        x += chWidth;
        str++;
    }
}

void Glcd::drawPixel(int x, int y, bool color)
{
    if (x >= width || y >= height)
    {
        return;
    }

    int xByte = x/8;
    int bitMask = 1<<(7-(x%8));
    if (color)
    {
        mem[y][xByte] |= bitMask;
    }
    else
    {
        mem[y][xByte] &= ~bitMask;
    }
}

void Glcd::drawLine(int x0, int y0, int x1, int y1, bool color)
{
    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = (dx>dy ? dx : -dy)/2, e2;

    for(;;)
    {
        drawPixel(x0,y0,color);
        if (x0==x1 && y0==y1) break;
        e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
    }
}

QPoint Glcd::translatePos(QPoint pos)
{
    QPoint noPixel(-1,-1);
    if (pos.x() < spaceWidth || pos.y() < spaceHeight)
    {
        return noPixel;
    }
    int x = (pos.x()-spaceWidth)/(pixelWidth+spaceWidth);
    int y = (pos.y()-spaceHeight)/(pixelHeight+spaceHeight);
    if (x >= width || y >= height)
    {
        return noPixel;
    }
    int rectX = x*(pixelWidth+spaceWidth)+spaceWidth;
    int rectY = y*(pixelHeight+spaceHeight)+spaceHeight;
    if (pos.x() >= (rectX+pixelWidth) || pos.y() >= (rectY+pixelHeight))
    {
        return noPixel;
    }
    return QPoint(x, y);
}

