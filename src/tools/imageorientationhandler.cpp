#include "imageorientationhandler.h"
#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_LINUX
    #include <libexif/exif-data.h>
#endif

ImageOrientationHandler::ImageOrientationHandler() {}

#ifdef Q_OS_LINUX
void ImageOrientationHandler::rotateImageBasedOnExif(const QString &image)
{
    //getExifRotation() will return 0 if it has no data, in other case it will return 1-8;
    short currentExifRotation = getExifRotation(image);

    if(currentExifRotation > 1){
        rotateImg(image, currentExifRotation, true); //if currentExifRotation==1, then the rotation is normal
    }
}

short ImageOrientationHandler::getExifRotation(const QString &filename){
    ExifData *data = exif_data_new_from_file(filename.toLocal8Bit().data());
    ExifByteOrder byte_order = exif_data_get_byte_order(data);
    ExifEntry *entry;

    short return_value=0;
    if(data){
        if ((entry = exif_content_get_entry( data->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION))){
            return_value = (short) exif_get_short(entry->data, byte_order);
        }
        exif_data_unref(data); //no need to exif_entry_unref(entry), there is no memory leak here.
    }
    return return_value;
}
#endif

void ImageOrientationHandler::rotateImg(const QString &filename, short rotation_type, bool show_messagebox){
    /*
     * A function for rotating the image 'filename' based on its 'rotation_type'
     * Rotation_type corresponds to the exif orientation values (1-8) as defined at:
     * https://exiftool.org/TagNames/EXIF.html
     */
    QString ext=filename.right(3);
    if(ext == "gif" || ext == "GIF"){
        if(show_messagebox){
            qWarning() << "Rotation is not supported for GIF files!";
        }
        else
        {
            QMessageBox::warning(0, QCoreApplication::translate("ImageOrientationHandler", "Error!"),
                                 QCoreApplication::translate("ImageOrientationHandler", "Rotation is not supported for GIF files!"));
        }
        return;
    }
    QString extension;
    if(ext=="jpg" || ext=="JPG" || ext=="peg" || ext=="PEG"){
        extension="JPG";
    }
    else if(ext=="png" || ext=="PNG"){
        extension="PNG";
    }
    else if(ext=="bmp" || ext=="BMP"){
        extension="BMP";
    }

    /*
     * ASCII art table representing the Exif orientation values
     *     1        2       3      4         5            6           7          8
     *
     *   888888  888888      88  88      8888888888  8888888888          88  88
     *   88          88      88  88      88  88          88  88      88  88  88  88
     *   8888      8888    8888  8888    88                  88  8888888888  8888888888
     *   88          88      88  88
     *   88          88  888888  888888
     *
     */

    QImage image(filename);
    enum MirrorType { None, Horizontal, Vertical };
    MirrorType mirror = None;
    short rotate=0;

    switch (rotation_type){
    default: // Original (no transformation)
        break;
    case 2: // Mirror Horizontal
        mirror=Horizontal;
        break;
    case 3: // Rotate 180
        rotate=180;
        break;
    case 4: // Mirror Vertical
        mirror=Vertical;
        break;
    case 5: // Mirror horizontal and rotate 270 CW
        mirror=Horizontal;
        rotate=270;
        break;
    case 6: // Rotate 90 CW
        rotate=90;
        break;
    case 7: // Mirror horizontal and rotate 90 CW
        mirror=Horizontal;
        rotate=90;
        break;
    case 8: // Rotate 270 CW
        rotate=270;
        break;
    }

    if(mirror==Horizontal){
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
        image = image.flipped(Qt::Horizontal);
#else
        image = image.mirrored(true, false);
#endif
    }
    else if(mirror==Vertical){
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
        image = image.flipped(Qt::Vertical);
#else
        image = image.mirrored(false, true);
#endif
    }

    if(rotate)
        image = image.transformed(QTransform().rotate(rotate), Qt::SmoothTransformation);

    image.save(filename, extension.toLocal8Bit().data(), 100);
}
