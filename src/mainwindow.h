#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDomDocument>
#include <QModelIndex>
#include <QListWidgetItem>
#include <QStringList>
#include "converter.h"
#include "glcdscene.h"
#include "glcd.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void MainWindow::pixelHovered(QPoint pixel);
    
private slots:
    void createGlcdView();
    void presetChanged(int preset);
    void on_openFileButton_clicked();
    void on_generateButton_clicked();
    void on_listWidget_currentRowChanged(int currentRow);
    void on_listWidget_itemClicked(QListWidgetItem *item);
    void on_pixelSize_valueChanged(int arg1);
    void on_spaceSize_valueChanged(int arg1);
    void on_printButton_clicked();
    void on_clearButton_clicked();
    void on_fillButton_clicked();
    void on_threshold_valueChanged(int arg1);
    void on_firstChar_valueChanged(int arg1);
    void on_lastChar_valueChanged(int arg1);
    void on_charCustomWidthEnb_clicked(bool checked);
    void on_charCustomWidth_valueChanged(int arg1);
    void on_charIncluded_clicked(bool checked);
    void on_imgCustomWidthEnb_clicked(bool checked);
    void on_imgCustomWidth_valueChanged(int arg1);


private:
    Ui::MainWindow *ui;

    enum Preset{
        ESP8266 = 0, Generic_8bit, Generic_32bit
    };
    enum Bitcount{
        bits8 = 0, bits32
    };
    enum Bitorder{
        MSB_first = 0, LSB_first
    };
    QStringList presets, bitcounts, bitorders;

    bool openFont(const QString &filename);
    void initPreview();
    void updateFontInfoLabels(const FontInfo*);
    void updateCharInfoLabels(const CharInfo*);
    void updateImgInfoLabels(const ImageInfo*);
    void clearCharInfoLabels();
    void clearImgInfoLabels();
    const CharInfo* getCurrentCharInfo();
    const ImageInfo* getCurrentImgInfo();

    void drawItemOnGlcd(int index);
    void setGlcdFont();
    void drawGlcd();
    void updateGlcdView();

    Converter converter;

    bool isFontFile;
    QString fontFile;

    Glcd *glcd;
    GlcdScene *glcdScene;
};


#endif // MAINWINDOW_H
