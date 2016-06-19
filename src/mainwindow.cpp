#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    isFontFile = false;

    glcd = NULL;
    glcdScene = NULL;
    createGlcdView();
    connect(ui->setGlcdSizeButton, SIGNAL(clicked(bool)), this, SLOT(createGlcdView()));

    bitcounts.append("8 bit");
    bitcounts.append("32 bit");
    ui->bitcount->addItems(bitcounts);

    bitorders.append("MSB first");
    bitorders.append("LSB first");
    ui->bitorder->addItems(bitorders);

    presets.append("ESP8266");
    presets.append("Generic (8 bit)");
    presets.append("Generic (32 bit)");
    ui->preset->addItems(presets);

    presetChanged(ESP8266);
    connect(ui->preset, SIGNAL(currentIndexChanged(int)), this, SLOT(presetChanged(int)));

    ui->generateButton->setEnabled(false);
    ui->fontInfoBox->setVisible(false);
    ui->charInfoBox->setVisible(false);
    ui->imgInfoBox->setVisible(false);
    ui->lThreshold->setVisible(false);
    ui->threshold->setVisible(false);

    ui->fill->setInputMask("\\0\\xHH");
    ui->fill->setText("0xFF");
}

MainWindow::~MainWindow()
{
    delete glcd;
    delete ui;
}

void MainWindow::createGlcdView()
{
    delete glcdScene;
    delete glcd;

    int pixelSize = ui->pixelSize->value();
    int spaceSize = ui->spaceSize->value();
    glcd = new Glcd(ui->glcdWidth->value(), ui->glcdHeight->value(),
                    pixelSize, pixelSize, spaceSize, spaceSize);

    glcdScene = new GlcdScene(glcd, this);
    ui->glcdView->setScene(glcdScene);

    updateGlcdView();
    if (isFontFile && converter.getChars().size() > 0)
    {
        setGlcdFont();
    }
}

void MainWindow::presetChanged(int preset)
{
    if (preset == ESP8266)
    {
        ui->arraySyntax1->setText("static const unsigned int %1[%2] ICACHE_RODATA_ATTR={");
        ui->arraySyntax2->setText("const unsigned int *%1[%2] ={");
        ui->includes->clear();
        ui->includes->appendPlainText("#include <ets_sys.h>\n");
        ui->bitcount->setCurrentIndex(bits32);
    }
    else if (preset == Generic_8bit)
    {
        ui->arraySyntax1->setText("static const unsigned char %1[%2] ={");
        ui->arraySyntax2->setText("const unsigned char *%1[%2] ={");
        ui->includes->clear();
        ui->bitcount->setCurrentIndex(bits8);
    }
    else if (preset == Generic_32bit)
    {
        ui->arraySyntax1->setText("static const unsigned int %1[%2] ={");
        ui->arraySyntax2->setText("const unsigned int *%1[%2] ={");
        ui->includes->clear();
        ui->bitcount->setCurrentIndex(bits32);
    }
}

const CharInfo *MainWindow::getCurrentCharInfo()
{
    int index = ui->listWidget->currentIndex().row();
    return converter.getCharInfo(index);
}

const ImageInfo *MainWindow::getCurrentImgInfo()
{
    int index = ui->listWidget->currentIndex().row();
    return converter.getImageInfo(index);
}


bool MainWindow::openFont(const QString &filename)
{
    if (!converter.openFont(filename,
                       ui->threshold->value(),
                       ui->firstChar->value(),
                       ui->lastChar->value()))
    {
        return false;
    }

    clearCharInfoLabels();
    updateFontInfoLabels(converter.getFontInfo());
    initPreview();
    setGlcdFont();
    return true;
}


void MainWindow::initPreview()
{
    ui->listWidget->setResizeMode( QListView::Adjust );
    ui->listWidget->clear();
    if (isFontFile)
    {
        foreach (CharInfo *ch, converter.getChars())
        {
            ui->listWidget->addItem( new QListWidgetItem( QIcon(ch->charPic), QString() ));
        }
    }
    else
    {
        foreach (ImageInfo *img, converter.getImages())
        {
            ui->listWidget->addItem( new QListWidgetItem( QIcon(img->pixmap), QString() ));
        }
    }
}

void MainWindow::updateFontInfoLabels(const FontInfo *fontInfo)
{
    if (!fontInfo)
        return;

    ui->lFontName->setText( "<b>" + fontInfo->name );
    ui->lFontSize->setText( "<b>" + QString().sprintf("%d * %d%", fontInfo->size, fontInfo->stretch) );
    ui->lFontFirst->setText( "<b>" + QString().sprintf("(0x%02x) '%c'", fontInfo->first, fontInfo->first) );
    ui->lFontLast->setText( "<b>" + QString().sprintf("(0x%02x) '%c'", fontInfo->last, fontInfo->last) );
    ui->lFontUsed->setText( "<b>" + QString().sprintf("%d / %d", fontInfo->used, fontInfo->count) );
    ui->lFontBytes->setText( "<b>" + QString().sprintf("%d B", fontInfo->overallSize) );
}

void MainWindow::updateCharInfoLabels(const CharInfo *charInfo)
{
    if (!charInfo)
        return;

    ui->lCharIndex->setText( "<b>" + QString::number(ui->listWidget->currentIndex().row()) );
    ui->lCharChar->setText( "<b>" + QString().sprintf("'%c' %d (0x%02x)", charInfo->id, charInfo->id, charInfo->id) );
    ui->lCharDim->setText( "<b>" + QString().sprintf("%d x %d", charInfo->width, charInfo->height) );
    ui->lCharScaled->setText( charInfo->scaled ? "<b>Yes" : "<b>No" );
    ui->lCharBytes->setText( "<b>" + QString().sprintf("%d B", charInfo->byteSize ) );
    ui->charIncluded->setChecked(!charInfo->skip);
    ui->charCustomWidth->setValue(charInfo->customWidth);
    ui->charCustomWidth->setEnabled(charInfo->useCustomWidth && !charInfo->skip);
    ui->charCustomWidthEnb->setChecked(charInfo->useCustomWidth);
    ui->charCustomWidthEnb->setEnabled(!charInfo->skip);
}

void MainWindow::updateImgInfoLabels(const ImageInfo *imgInfo)
{
    if (!imgInfo)
        return;

    ui->lImgName->setText("<b>" + imgInfo->name);
    ui->lImgDim->setText("<b>" + QString().sprintf("%d x %d", imgInfo->width, imgInfo->height));
    ui->lImgScaled->setText(imgInfo->scaled ? "<b>Yes" : "<b>No");
    ui->lImgBytes->setText("<b>" + QString().sprintf("%d B", imgInfo->byteSize ));
    ui->imgCustomWidth->setValue(imgInfo->customWidth);
    ui->imgCustomWidth->setEnabled(imgInfo->useCustomWidth);
    ui->imgCustomWidthEnb->setChecked(imgInfo->useCustomWidth);
}

void MainWindow::clearCharInfoLabels()
{
    ui->lCharIndex->setText("");
    ui->lCharChar->setText("");
    ui->lCharDim->setText("");
    ui->lCharScaled->setText("");
    ui->lCharBytes->setText("");
    ui->charIncluded->setChecked(false);
    ui->charCustomWidth->setValue(0);
    ui->charCustomWidthEnb->setChecked(false);
    ui->charCustomWidth->setEnabled(false);
}

void MainWindow::clearImgInfoLabels()
{
    ui->lImgName->setText("");
    ui->lImgDim->setText("");
    ui->lImgScaled->setText("");
    ui->lImgBytes->setText("");
    ui->imgCustomWidth->setValue(0);
    ui->imgCustomWidth->setEnabled(false);
    ui->imgCustomWidthEnb->setChecked(false);
}


void MainWindow::on_openFileButton_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
                this,
                "Select file to open",
                QFileInfo(fontFile).absolutePath(), "(*.fnt *.png *.bmp *.jpg)");

    if (filenames.isEmpty())
        return;

    QString suffix = QFileInfo(filenames.first()).suffix();
    foreach (QString filename, filenames)
    {
        if (QFileInfo(filename).suffix() != suffix)
        {
            QMessageBox msgBox;
            msgBox.setText("Open either font or image(s) files.");
            msgBox.exec();
            return;
        }
    }

    fontFile = filenames.first();
    isFontFile = suffix == "fnt";
    if (isFontFile)
    {
        if (!openFont(fontFile))
        {
            ui->generateButton->setEnabled(false);
            return;
        }
        setWindowTitle("FontConverter - " + QDir::toNativeSeparators(fontFile));
    }
    else
    {
        converter.clearImages();
        foreach (QString filename, filenames)
        {
            if (!converter.openImage(filename, ui->threshold->value()))
            {
                ui->generateButton->setEnabled(false);
                return;
            }
        }
        initPreview();
        clearImgInfoLabels();
        setWindowTitle("FontConverter - " +
                       QDir::toNativeSeparators(QFileInfo(filenames.first()).absolutePath()));
    }
    ui->generateButton->setEnabled(true);
    ui->fontInfoBox->setVisible(isFontFile);
    ui->charInfoBox->setVisible(isFontFile);
    ui->imgInfoBox->setVisible(!isFontFile);
    ui->lThreshold->setVisible(true);
    ui->threshold->setVisible(true);
}


void MainWindow::on_generateButton_clicked()
{
    QString basename = isFontFile ? QFileInfo(fontFile).baseName() : "images";
    QString abspath;
    if (isFontFile)
    {
        abspath = QFileInfo(fontFile).absolutePath();
    }
    else
    {
        const ImageInfo *imgInfo = converter.getImageInfo(0);
        if (!imgInfo)
            return;
        abspath = QFileInfo(imgInfo->srcFile).absolutePath();
    }
    QString filename = QFileDialog::getSaveFileName(this, "Save File",
                               abspath+"/"+basename+".c", ("(*.c)"));
    if (filename.isNull())
        return;

    QString includes = ui->includes->toPlainText();
    bool bitcount32 = ui->bitcount->currentIndex() == bits32;
    QImage::Format format = ui->bitorder->currentIndex() == MSB_first ?
                            QImage::Format_Mono : QImage::Format_MonoLSB;
    if (isFontFile)
    {
        converter.generateFont(filename, basename,
                               includes, bitcount32, format,
                               ui->arraySyntax1->text(),
                               ui->arraySyntax2->text());
    }
    else
    {
        converter.generateImages(filename,
                                 includes, bitcount32, format,
                                 ui->arraySyntax1->text());
    }
}

void MainWindow::setGlcdFont()
{
    glcd->setFont(converter.getFontData(QImage::Format_Mono));
}


void MainWindow::on_listWidget_currentRowChanged(int currentRow)
{
    drawItemOnGlcd(currentRow);
}

void MainWindow::on_listWidget_itemClicked(QListWidgetItem *item)
{
    drawItemOnGlcd(ui->listWidget->row(item));
}

void MainWindow::drawItemOnGlcd(int index)
{
    if (isFontFile)
    {
        const CharInfo *charInfo = converter.getCharInfo(index);
        if (!charInfo)
            return;

        updateCharInfoLabels(charInfo);

        glcd->fillMem(0);
        glcd->drawChar(ui->cursorX->value(), ui->cursorY->value(), (char)charInfo->id);
        //glcd->printMem();
        drawGlcd();
    }
    else
    {
        const ImageInfo *imgInfo = converter.getImageInfo(index);
        if (!imgInfo)
            return;

        updateImgInfoLabels(imgInfo);

        uchar *image = converter.getImageData(index, QImage::Format_Mono);
        glcd->fillMem(0);
        glcd->drawImage(ui->cursorX->value(), ui->cursorY->value(), image);
        drawGlcd();
        delete image;
    }
}

void MainWindow::on_threshold_valueChanged(int arg1)
{
    arg1;
    QModelIndex index = ui->listWidget->currentIndex();
    if (isFontFile)
    {
        openFont(fontFile);
    }
    else
    {
        QStringList filenames = converter.getImgFiles();
        converter.clearImages();
        foreach (QString filename, filenames)
        {
            converter.openImage(filename, ui->threshold->value());
        }
        initPreview();
    }
    ui->listWidget->setCurrentIndex(index);
}

void MainWindow::on_firstChar_valueChanged(int arg1)
{
    ui->lastChar->setMinimum(arg1);
    if (isFontFile)
    {
        openFont(fontFile);
    }
}

void MainWindow::on_lastChar_valueChanged(int arg1)
{
    ui->firstChar->setMaximum(arg1);
    if (isFontFile)
    {
        openFont(fontFile);
    }
}

void MainWindow::on_charCustomWidthEnb_clicked(bool checked)
{
    CharInfo *charInfo = (CharInfo*)getCurrentCharInfo();
    if (!charInfo)
        return;

    charInfo->useCustomWidth = checked;
    charInfo->customWidth = ui->charCustomWidth->value();
    converter.recreateCharPic(charInfo, ui->threshold->value());

    updateCharInfoLabels(charInfo);
    ui->lFontBytes->setText( "<b>" + QString().sprintf("%d B", converter.getFontInfo()->overallSize) );
    ui->listWidget->currentItem()->setIcon(charInfo->charPic);
    setGlcdFont();
}

void MainWindow::on_charCustomWidth_valueChanged(int arg1)
{
    CharInfo *charInfo = (CharInfo*)getCurrentCharInfo();
    if (!charInfo || !charInfo->useCustomWidth)
        return;

    charInfo->customWidth = arg1;
    converter.recreateCharPic(charInfo, ui->threshold->value());

    updateCharInfoLabels(charInfo);
    ui->lFontBytes->setText( "<b>" + QString().sprintf("%d B", converter.getFontInfo()->overallSize) );
    ui->listWidget->currentItem()->setIcon(charInfo->charPic);
    setGlcdFont();
}


void MainWindow::on_charIncluded_clicked(bool checked)
{
    const CharInfo *charInfo = getCurrentCharInfo();
    QListWidgetItem *item = ui->listWidget->currentItem();
    if (!charInfo || !item)
        return;

    converter.charIncluded(ui->listWidget->currentIndex().row(), checked);

    if (!checked)
    {
        QPixmap pixmap = item->icon().pixmap(ui->listWidget->iconSize(), QIcon::Disabled);
        item->setIcon( pixmap );

        ui->charCustomWidthEnb->setEnabled(false);
        ui->charCustomWidth->setEnabled(false);
    }
    else
    {
        item->setIcon( charInfo->charPic );

        ui->charCustomWidthEnb->setEnabled(true);
        ui->charCustomWidth->setEnabled(charInfo->useCustomWidth);
    }

    updateFontInfoLabels(converter.getFontInfo());
    setGlcdFont();
}

void MainWindow::on_imgCustomWidthEnb_clicked(bool checked)
{
    ImageInfo *imgInfo = (ImageInfo*)getCurrentImgInfo();
    if (!imgInfo)
        return;

    imgInfo->useCustomWidth = checked;
    imgInfo->customWidth = ui->imgCustomWidth->value();
    converter.recreateImgPic(imgInfo, ui->threshold->value());

    updateImgInfoLabels(imgInfo);
    ui->listWidget->currentItem()->setIcon(imgInfo->pixmap);
}

void MainWindow::on_imgCustomWidth_valueChanged(int arg1)
{
    ImageInfo *imgInfo = (ImageInfo*)getCurrentImgInfo();
    if (!imgInfo || !imgInfo->useCustomWidth)
        return;

    imgInfo->customWidth = arg1;
    converter.recreateImgPic(imgInfo, ui->threshold->value());

    updateImgInfoLabels(imgInfo);
    ui->listWidget->currentItem()->setIcon(imgInfo->pixmap);
}

//------------------------------------------------------------------------------------
void MainWindow::drawGlcd()
{
    glcd->renderMem();
    updateGlcdView();
}

void MainWindow::updateGlcdView()
{
    QPixmap pixmap = glcd->getPixmap();
    glcdScene->clear();
    glcdScene->addPixmap(pixmap);
    glcdScene->setSceneRect(pixmap.rect());
}

void MainWindow::on_pixelSize_valueChanged(int arg1)
{
    glcd->setPixelSize(arg1, arg1);
    updateGlcdView();
}

void MainWindow::on_spaceSize_valueChanged(int arg1)
{
    glcd->setSpaceSize(arg1, arg1);
    updateGlcdView();
}

void MainWindow::on_printButton_clicked()
{
    glcd->drawStr(ui->cursorX->value(), ui->cursorY->value(),
                  ui->text->text().toLatin1().toStdString().c_str());
    drawGlcd();
}

void MainWindow::on_clearButton_clicked()
{
    glcd->fillMem(0);
    drawGlcd();
}

void MainWindow::on_fillButton_clicked()
{
    bool ok;
    glcd->fillMem(ui->fill->text().toUInt(&ok, 16));
    drawGlcd();
}

void MainWindow::pixelHovered(QPoint pixel)
{
    QString text;
    if (pixel.x() >= 0 && pixel.y() >= 0)
    {
        text = QString("%1, %2").arg(pixel.x()).arg(pixel.y());
    }
    ui->glcdInfo->setText(text);
}

