#include "converter.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDomDocument>
#include <QPainter>
#include <QDebug>


Converter::Converter()
{

}

Converter::~Converter()
{
    clearChars();
    clearImages();
}

static QPixmap createPixmap(QPixmap &srcImage, int x, int y, int width, int height, int xadvance, int threshold, bool &scaled)
{
    QPixmap charPic = srcImage.copy(x, y, width, height);

    int mod = xadvance%8;
    if (mod)
    {
        if (mod <= threshold && xadvance > 8)   // cut width
        {
            xadvance -= mod;
            if (xadvance < width)   // scale pic
            {
                width = xadvance;
                scaled = true;
                return charPic.scaled(width, height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
            }
        }
        else    // increase width
        {
            xadvance += (8-mod);
        }
    }

    int xoffset = ((double)(xadvance-width))/2 + 0.5;
    QImage newImg(xadvance, height, QImage::Format_RGB32);
    newImg.fill(Qt::white);

    QRectF source(0.0, 0.0, width, height);
    QRectF target(xoffset, 0.0, width, height);
    QPainter painter;
    painter.begin(&newImg);
    painter.drawPixmap(target, charPic, source);
    painter.end();

    scaled = false;
    return QPixmap::fromImage(newImg);
}

static QPixmap createPixmap(QPixmap &srcImage, int x, int y, int width, int height, int targetWidth, bool &scaled)
{
    QPixmap charPic = srcImage.copy(x, y, width, height);

    if (targetWidth < width)
    {
        qDebug() << "getCharPic width" << width << "targetWidth" << targetWidth;
        scaled = true;
        return charPic.scaled(targetWidth, height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    int xoffset = ((double)(targetWidth-width))/2 + 0.5;
    QImage newImg(targetWidth, height, QImage::Format_RGB32);
    newImg.fill(Qt::white);

    QRectF source(0.0, 0.0, width, height);
    QRectF target(xoffset, 0.0, width, height);
    QPainter painter;
    painter.begin(&newImg);
    painter.drawPixmap(target, charPic, source);
    painter.end();

    scaled = false;
    return QPixmap::fromImage(newImg);
}

bool Converter::openFont(const QString &filename, int threshold, int firstChar, int lastChar)
{
    QFile file(filename);
    QDomDocument doc;
    if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file))
    {
        file.close();
        return false;
    }
    file.close();

    QDomElement root = doc.documentElement();
    if( root.tagName() == "font" )
    {
        QDomElement element = root.firstChildElement("info");
        if (!element.isNull())
        {
            fontInfo.name = element.attribute("face");
            fontInfo.size = element.attribute("size").toInt();
            fontInfo.stretch = element.attribute("stretchH").toInt();
        }

        element = root.firstChildElement("pages").firstChildElement("page");
        if (!element.isNull())
        {
            QString imageFilename = QFileInfo(file).absolutePath()+"/"+element.attribute("file");
            qDebug() << imageFilename;
            fontImage = QPixmap(imageFilename);
            if (fontImage.isNull())
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    QMap<int, CharInfo*> charsTemp;
    QDomNodeList chs = doc.elementsByTagName("char");
    for (int i=0; i < chs.count(); i++)
    {
        QDomElement ch = chs.item(i).toElement();

        CharInfo *charInfo = new CharInfo;
        charInfo->id = ch.attribute("id").toInt();
        charInfo->attributes.x = ch.attribute("x").toInt();
        charInfo->attributes.y = ch.attribute("y").toInt();
        charInfo->attributes.width = ch.attribute("width").toInt();
        charInfo->attributes.height = ch.attribute("height").toInt();
        charInfo->attributes.xadvance = ch.attribute("xadvance").toInt();
        charInfo->attributes.yoffset = ch.attribute("yoffset").toInt();
        charInfo->charPic = createPixmap(fontImage,
                                       charInfo->attributes.x,
                                       charInfo->attributes.y,
                                       charInfo->attributes.width,
                                       charInfo->attributes.height,
                                       charInfo->attributes.xadvance,
                                       threshold,
                                       charInfo->scaled);
        charInfo->width = charInfo->charPic.width();
        charInfo->height = charInfo->charPic.height();
        charInfo->byteSize = charInfo->width*charInfo->height/8;
        charInfo->skip = false;
        charInfo->customWidth = charInfo->width;
        charsTemp.insert(charInfo->id, charInfo);
    }

    clearChars();
    fontInfo.overallSize = 0;
    fontInfo.used = 0;
    for (int id = firstChar; id <= lastChar; id++)
    {
        CharInfo *ch = charsTemp.value(id, NULL);
        if (ch)
        {
            fontInfo.used++;
        }
        else
        {
            ch = new CharInfo;
            ch->id = id;
            ch->skip = true;
            ch->byteSize = 0;
        }
        chars.append(ch);
        fontInfo.overallSize += ch->byteSize;
    }
    fontInfo.first = chars.first()->id;
    fontInfo.last = chars.last()->id;
    fontInfo.count = chars.size();
    return true;
}

bool Converter::openImage(const QString &filename, int threshold)
{
    QPixmap origImg = QPixmap(filename);
    if (origImg.isNull())
    {
        return false;
    }
    ImageInfo *img = new ImageInfo;
    img->pixmap = createPixmap(origImg,
                             0, 0,
                             origImg.width(),
                             origImg.height(),
                             origImg.width(),
                             threshold,
                             img->scaled);
    img->width = img->pixmap.width();
    img->height = img->pixmap.height();
    img->byteSize = img->width*img->height/8;
    img->customWidth = img->width;
    img->srcFile = filename;
    img->name = QFileInfo(filename).baseName();
    images.append(img);
    imgFiles.append(filename);
    return true;
}


void Converter::clearChars()
{
    foreach (CharInfo *ch, chars)
    {
        delete ch;
    }
    chars.clear();
}

void Converter::clearImages()
{
    foreach (ImageInfo *img, images)
    {
        delete img;
    }
    images.clear();
    imgFiles.clear();
}


bool Converter::generateFont(const QString &filename,
                             const QString &fontname,
                             const QString &includes,
                             bool bitcount32,
                             QImage::Format format,
                             const QString &arraySyntax1,
                             const QString &arraySyntax2
                             )
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        return false;
    }
    QTextStream out(&file);

    int minYoffset = INT_MAX;
    foreach (CharInfo *ch, chars)
    {
        if (!ch->skip && ch->attributes.yoffset < minYoffset)
        {
            minYoffset = ch->attributes.yoffset;
        }
    }

    out << includes << "\n";

    QString lastChar = "";
    foreach (CharInfo *ch, chars)
    {
        if (!ch->skip)
        {
            out << lastChar;

            // bitmap data
            QImage img = ch->charPic.toImage();
            img = img.convertToFormat(format, Qt::MonoOnly);

            int byteWidth = (ch->width/8);
            QString lastChar2 = "";
            if (bitcount32)
            {
                int dwordSize = ch->byteSize/4;
                if (ch->byteSize%4)
                {
                    dwordSize++;
                }
                uint *temp = new uint[dwordSize]();
                uchar *pTemp = (uchar*)temp;
                for (int y = 0, i = 0; y < ch->height; y++, i+=byteWidth)
                {
                    uchar *line = img.scanLine(y);
                    memcpy(pTemp+i, line, byteWidth);
                }

                out << "/* '" << (char)ch->id << "' */\n";
                out << QString(arraySyntax1).arg(QString("char%1").arg(ch->id)).arg(dwordSize+4) << "\n";
                // header bytes
                out << QString("%1,%2,%3,%4,").arg(ch->width).arg(ch->height).arg(dwordSize*4).arg(ch->attributes.yoffset-minYoffset) << "\n";

                for (int i = 0; i < (dwordSize-1); i++)
                {
                    out << QString().sprintf("0x%08X,", temp[i]);
                }
                out << QString().sprintf("0x%08X", temp[dwordSize-1]);
                delete [] temp;
            }
            else    // 8bit
            {
                out << "/* '" << (char)ch->id << "' */\n";
                out << QString(arraySyntax1).arg(QString("char%1").arg(ch->id)).arg(ch->byteSize+4) << "\n";
                // header bytes
                out << QString("%1,%2,%3,%4,").arg(ch->width).arg(ch->height).arg(ch->byteSize).arg(ch->attributes.yoffset-minYoffset);

                for (int y = 0; y < ch->height; y++)
                {
                    out << lastChar2 << "\n";
                    uchar *line = img.scanLine(y);
                    for (int x = 0; x < (byteWidth-1); x++)
                    {
                        out << QString().sprintf("0x%02X,", line[x]);
                    }
                    out << QString().sprintf("0x%02X", line[byteWidth-1]);
                    lastChar2 = ",";
                }
            }

            out << "};";
            lastChar = "\n\n";
        }
    }
    out << "\n\n\n";

    out << QString(arraySyntax2).arg(fontname).arg(fontInfo.count+2) << "\n";
    if (bitcount32)
    {
        out << QString("(unsigned int*)%1,(unsigned int*)%2,\n").arg((int)fontInfo.first).arg((int)fontInfo.last);
    }
    else
    {
        out << QString("(unsigned char*)%1,(unsigned char*)%2,\n").arg((int)fontInfo.first).arg((int)fontInfo.last);
    }

    lastChar = "";
    foreach (CharInfo *ch, chars)
    {
        out << lastChar;
        if (ch->skip)
        {
            out << "0";
        }
        else
        {
            out << QString("char%1").arg(ch->id);
        }
        lastChar = ",\n";
    }
    out << "};\n";
    file.close();
    return true;
}

bool Converter::generateImages(const QString &filename,
                               const QString &includes,
                               bool bitcount32,
                               QImage::Format format,
                               const QString &arraySyntax1
                               )
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        return false;
    }
    QTextStream out(&file);

    out << includes << "\n";

    QString lastChar = "";
    foreach (ImageInfo *ii, images)
    {
        out << lastChar;

        // bitmap data
        QImage img = ii->pixmap.toImage();
        img = img.convertToFormat(format, Qt::MonoOnly);

        int byteWidth = (ii->width/8);
        QString lastChar2 = "";
        if (bitcount32)
        {
            int dwordSize = ii->byteSize/4;
            if (ii->byteSize%4)
            {
                dwordSize++;
            }
            uint *temp = new uint[dwordSize]();
            uchar *pTemp = (uchar*)temp;
            for (int y = 0, i = 0; y < ii->height; y++, i+=byteWidth)
            {
                uchar *line = img.scanLine(y);
                memcpy(pTemp+i, line, byteWidth);
            }

            out << QString(arraySyntax1).arg(ii->name).arg(dwordSize+4) << "\n";
            // header bytes
            out << QString("%1,%2,%3,0,").arg(ii->width).arg(ii->height).arg(dwordSize*4) << "\n";

            for (int i = 0; i < (dwordSize-1); i++)
            {
                out << QString().sprintf("0x%08X,", temp[i]);
            }
            out << QString().sprintf("0x%08X", temp[dwordSize-1]);

            delete [] temp;
        }
        else    // 8bit
        {
            out << QString(arraySyntax1).arg(ii->name).arg(ii->byteSize+4) << "\n";
            // header bytes
            out << QString("%1,%2,%3,0,").arg(ii->width).arg(ii->height).arg(ii->byteSize) << "\n";

            for (int y = 0; y < ii->height; y++)
            {
                out << lastChar2 << "\n";
                uchar *line = img.scanLine(y);
                for (int x = 0; x < (byteWidth-1); x++)
                {
                    out << QString().sprintf("0x%02X,", line[x]);
                }
                out << QString().sprintf("0x%02X", line[byteWidth-1]);
                lastChar2 = ",";
            }
        }

        out << "};";
        lastChar = "\n\n";
    }
    out << "\n\n\n";
    file.close();
    return true;
}


void Converter::recreateCharPic(CharInfo *charInfo, int threshold)
{
    if (charInfo->useCustomWidth)
    {
        charInfo->charPic = createPixmap(fontImage,
                                       charInfo->attributes.x,
                                       charInfo->attributes.y,
                                       charInfo->attributes.width,
                                       charInfo->attributes.height,
                                       charInfo->customWidth,
                                       charInfo->scaled);
    }
    else
    {
        charInfo->charPic = createPixmap(fontImage,
                                       charInfo->attributes.x,
                                       charInfo->attributes.y,
                                       charInfo->attributes.width,
                                       charInfo->attributes.height,
                                       charInfo->attributes.xadvance,
                                       threshold,
                                       charInfo->scaled);
    }

    charInfo->width = charInfo->charPic.width();
    charInfo->height = charInfo->charPic.height();

    int oldSize = charInfo->byteSize;
    charInfo->byteSize = charInfo->width*charInfo->height/8;

    if (!charInfo->skip)
    {
        fontInfo.overallSize -= oldSize;
        fontInfo.overallSize += charInfo->byteSize;
    }
}

void Converter::recreateImgPic(ImageInfo *imgInfo, int threshold)
{
    QPixmap origImg = QPixmap(imgInfo->srcFile);
    if (origImg.isNull())
    {
        return;
    }

    if (imgInfo->useCustomWidth)
    {
        imgInfo->pixmap = createPixmap(origImg,
                                     0, 0,
                                     origImg.width(),
                                     origImg.height(),
                                     imgInfo->customWidth,
                                     imgInfo->scaled);
    }
    else
    {
        imgInfo->pixmap = createPixmap(origImg,
                                     0, 0,
                                     origImg.width(),
                                     origImg.height(),
                                     origImg.width(),
                                     threshold,
                                     imgInfo->scaled);
    }

    imgInfo->width = imgInfo->pixmap.width();
    imgInfo->height = imgInfo->pixmap.height();
    imgInfo->byteSize = imgInfo->width*imgInfo->height/8;
}


void Converter::charIncluded(int index, bool included)
{
    CharInfo *charInfo = chars.value(index);
    if (!charInfo)
        return;

    charInfo->skip = !included;
    if (charInfo->skip)
    {
        fontInfo.overallSize -= charInfo->byteSize;
        fontInfo.used--;
    }
    else
    {
        fontInfo.overallSize += charInfo->byteSize;
        fontInfo.used++;
    }
}


uchar **Converter::getFontData(QImage::Format format)
{
    int minYoffset = INT_MAX;
    foreach (CharInfo *ch, chars)
    {
        if (!ch->skip && ch->attributes.yoffset < minYoffset)
        {
            minYoffset = ch->attributes.yoffset;
        }
    }

    uchar **fontdata = new uchar*[fontInfo.count+2];
    fontdata[0] = (uchar*)fontInfo.first;
    fontdata[1] = (uchar*)fontInfo.last;
    int chIdx = 2;
    foreach (CharInfo *ch, chars)
    {
        if (!ch->skip)
        {
            uchar *chdata = new uchar[ch->byteSize+4];
            chdata[0] = ch->width;
            chdata[1] = ch->height;
            chdata[2] = 0;
            chdata[3] = (ch->attributes.yoffset-minYoffset);

            // bitmap data
            QImage img = ch->charPic.toImage();
            img = img.convertToFormat(format, Qt::MonoOnly);

            int byteWidth = (ch->width/8);
            for (int y = 0, i = 4; y < ch->height; y++, i+=byteWidth)
            {
                uchar *line = img.scanLine(y);
                memcpy(chdata+i, line, byteWidth);
            }
            fontdata[chIdx] = chdata;
        }
        else
        {
            fontdata[chIdx] = 0;
        }
        chIdx++;
    }
    return fontdata;
}

uchar *Converter::getImageData(int index, QImage::Format format)
{
    ImageInfo *imgInfo = images.value(index);
    if (!imgInfo)
        return NULL;

    uchar *image = new uchar[imgInfo->byteSize+2];
    image[0] = imgInfo->width;
    image[1] = imgInfo->height;

    // bitmap data
    QImage img = imgInfo->pixmap.toImage();
    img = img.convertToFormat(format, Qt::MonoOnly);

    int byteWidth = (imgInfo->width/8);
    for (int y = 0, i = 2; y < imgInfo->height; y++, i+=byteWidth)
    {
        uchar *line = img.scanLine(y);
        memcpy(image+i, line, byteWidth);
    }
    return image;
}

