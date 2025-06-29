/*
 Wallch - A Modern, Cross-Platform Wallpaper Changer

 Copyright © 2010-2025, The Wallch Team.
 Original Authors (2010-2015): Alexandros Solanos, Leon Vitanos
 Modernization & New Code (2025-): Leon Vitanos

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.
*/

#include "liveearthfeature.h"
#include "imagefetcher.h"

LiveEarthFeature::LiveEarthFeature(ImageFetcher *imageFetcher, QObject *parent)
    : QObject(parent)
    , m_imageFetcher(imageFetcher)
{
}

void LiveEarthFeature::start()
{
    m_imageFetcher->setFetchType(FetchType::LE);
    m_imageFetcher->fetch();
}
