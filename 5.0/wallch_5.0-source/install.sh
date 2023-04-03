#!/bin/bash
#Wallch Installation file
#Original file made by Devyn Collier Johnson <DevynCJohnson@Gmail.com> for Wallch v3.xx
#Edited by Alex Solanos <alexsol.developer@gmail.com> for Wallch v4.0

set -e

error_report()
{
	echo "ERROR: $1";
	echo "Please report this at wallch.developers@gmail.com. Somebody will ACTUALLY have a look :)"
	exit 1
}

report_step(){
	echo -ne "\n\n"
	echo "---- $1 ----"
	echo -ne "\n\n"
}

install_now()
{
	echo -ne "Installing $1..."
	if [ -d "$2" ]; then
		#installing a directory
		cp -r "$2" "$3"
	elif [ -f "$2" ]; then
		#installing a file
		cp "$2" "$3"
	else
		error_report "\"$2\" - No such file or directory."
	fi
	echo -ne "Done.\n"
}

install_tr()
{
	echo -ne "Installing $1..."
	lrelease -silent "$1" -qm "$2"
	echo -ne "Done.\n"
}

if [[ $EUID != 0 ]]; then
	echo "Please run this install script as root: sudo $0"
	exit 1
fi

report_step "Installing the needed packages"
apt-get install debhelper qt5-qmake qttools5-dev-tools libqt5core5a libqt5widgets5 qt5-default libqt5network5 libqt5dbus5 libnotify-dev libunity-dev libkeybinder-dev libdee-dev libexif-dev libappindicator-dev libglib2.0-dev qtdeclarative5-dev libqt5opengl5-dev libgstreamer-plugins-base0.10-dev libsqlite3-dev libxslt1-dev libqt5webkit5-dev || (error_report "installing build dependencies failed")
apt-get install xdg-utils unzip libstdc++6 libappindicator1 libc6 libdbusmenu-glib4 libexif12 libgcc1 libglib2.0-0 libgtk2.0-0 libnotify4 || (error_report "installing dependencies failed")

report_step "Building the source code"

if [ -f "wallch" ]; then
	echo "Weird, code seems compiled. Hard recompilation takes place."
	make clean
	rm -rf wallch Makefile
fi

qmake *.pro || (error_report "qmake failed")
make || (error_report "make failed")

report_step "Compiling translations"

install_tr data/translations/wallch_el.ts data/to_usr_share/wallch/translations/wallch_el.qm

report_step "Installing Wallch to your system"

install_now "wallch binary" "wallch" "/usr/bin/"
install_now "/usr/share files" "data/to_usr_share/wallch/" "/usr/share/"
install_now "bash autocompletion" "data/bash_autocompletion/wallch" "/etc/bash_completion.d/"
install_now "manpage" "data/man/wallch.1.gz" "/usr/share/man/man1/"
install_now "program pixmap" "data/pixmap/wallch.png" "/usr/share/pixmaps/"
install_now "desktop file" "data/wallch-nautilus.desktop" "/usr/share/applications/"

report_step "Installation finished. Thank you for using Wallch."