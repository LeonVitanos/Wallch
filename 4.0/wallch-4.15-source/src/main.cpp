/*
Wallch - Wallpaper Changer
A tool for changing Desktop Wallpapers automatically
with lots of features
Copyright © 2010-2014 by Alex Solanos and Leon Vitanos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#define QT_NO_KEYWORDS

#include "nongui.h"
#include "glob.h"

struct GlobalVar gv;

nonGUI nongui;

int main(int argc, char *argv[])
{
#ifdef ON_LINUX
    //small workaround in order to make Wallch work under Mint 17
    if(gv.unacceptedDesktopValues.contains(QProcessEnvironment::systemEnvironment().value("XDG_CURRENT_DESKTOP"))){
        qputenv("XDG_CURRENT_DESKTOP", "GNOME");
    }
#endif

    return nongui.startProgram(argc, argv);
}
