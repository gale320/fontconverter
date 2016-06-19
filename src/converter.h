#ifndef CONVERTER_H
#define CONVERTER_H

#include <QList>
#include <QPixmap>
#include <QTextStream>

struct FontInfo{
    FontInfo(){
        size = 0;
        stretch = 0;
        first = 0;
        last = 0;
        count = 0;
        used = 0;
        overallSize = 0;
    }

    QString name;
    int size, stretch;
    int first, last;
    int count, used;
    int overallSize;
};

struct CharInfo{
    CharInfo(){
        id = 0;
        width = 0;
        height = 0;
        scaled = false;
        byteSize = 0;
        skip = true;
        customWidth = 0;
        useCustomWidth = false;
    }
    struct Attributes{
        Attributes(){
            x = 0;
            y = 0;
            width = 0;
            height = 0;
            xadvance = 0;
            yoffset = 0;
        }
        int x, y, width, height, xadvance, yoffset;
    };
    Attributes attributes;

    int id;
    int width, height;
    bool scaled;
    int byteSize;
    bool skip;
    int customWidth;
    bool useCustomWidth;
    QPixmap charPic;
};

struct ImageInfo{
    ImageInfo(){
        width = 0;
        height = 0;
        scaled = false;
        byteSize = 0;
        customWidth = 0;
        useCustomWidth = false;
    }

    int width, height;
    bool scaled;
    int byteSize;
    int customWidth;
    bool useCustomWidth;
    QPixmap pixmap;
    QString srcFile;
    QString name;
};


class Converter
{
public:
    Converter();
    ~Converter();

    const FontInfo *getFontInfo() { return &fontInfo; }
    const CharInfo *getCharInfo(int index) { return chars.value(index); }
    const ImageInfo *getImageInfo(int index) { return images.value(index); }
    const QList<CharInfo*> &getChars() { return chars; }
    const QList<ImageInfo*> &getImages() { return images; }
    const QStringList &getImgFiles() { return imgFiles; }

    bool openFont(const QString &filename, int threshold, int firstChar, int lastChar);
    bool openImage(const QString &filename, int threshold);

    bool generateFont(const QString &filename,
                      const QString &fontname,
                      const QString &includes,
                      bool bitcount32,
                      QImage::Format format,
                      const QString &arraySyntax1,
                      const QString &arraySyntax2);

    bool generateImages(const QString &filename,
                        const QString &includes,
                        bool bitcount32,
                        QImage::Format format,
                        const QString &arraySyntax1);

    void recreateCharPic(CharInfo *charInfo, int threshold);
    void recreateImgPic(ImageInfo *imgInfo, int threshold);

    void charIncluded(int index, bool included);

    void clearChars();
    void clearImages();

    uchar **getFontData(QImage::Format format);
    uchar *getImageData(int index, QImage::Format format);

private:
    QPixmap fontImage;
    QStringList imgFiles;

    FontInfo fontInfo;
    QList<CharInfo*> chars;
    QList<ImageInfo*> images;
};


#endif // CONVERTER_H
