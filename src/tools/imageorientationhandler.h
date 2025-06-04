#ifndef IMAGEORIENTATIONHANDLER_H
#define IMAGEORIENTATIONHANDLER_H

#include <QString>

class ImageOrientationHandler
{
public:
    ImageOrientationHandler();
    static void rotateImg(const QString &filename, short rotation_type, bool show_messagebox);
#ifdef Q_OS_LINUX
    static void rotateImageBasedOnExif(const QString &image);
    static short getExifRotation(const QString &filename);
#endif
};

#endif // IMAGEORIENTATIONHANDLER_H
