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
apt-get install debhelper qt5-qmake qttools5-dev-tools libqt5core5 libqt5widgets5 libqt5network5 libqt5dbus5 libnotify-dev libunity-dev libkeybinder-dev libdee-dev libexif-dev libappindicator-dev libglib2.0-dev qtdeclarative5-dev libqt5opengl5-dev libgstreamer-plugins-base0.10-dev libsqlite3-dev libxslt1-dev libqt5webkit5-dev || (error_report "installing build dependencies failed")
apt-get install xdg-utils yelp unzip libstdc++6 libappindicator1 libc6 libdbusmenu-glib4 libexif12 libgcc1 libglib2.0-0 libgtk2.0-0 libnotify4 || (error_report "installing dependencies failed")

report_step "Building the source code"

if [ -f "wallch" ]; then
	echo "Weird, code seems compiled. Hard recompilation takes place."
	make clean
	rm -rf wallch Makefile
fi

qmake *.pro || (error_report "qmake failed")
make || (error_report "make failed")

report_step "Compiling translations"

install_tr data/translations/wallch_bs.ts data/to_usr_share/wallch/translations/wallch_bs.qm
install_tr data/translations/wallch_fr.ts data/to_usr_share/wallch/translations/wallch_fr.qm
install_tr data/translations/wallch_nb.ts data/to_usr_share/wallch/translations/wallch_nb.qm
install_tr data/translations/wallch_sk.ts data/to_usr_share/wallch/translations/wallch_sk.qm
install_tr data/translations/wallch_da.ts data/to_usr_share/wallch/translations/wallch_da.qm
install_tr data/translations/wallch_he.ts data/to_usr_share/wallch/translations/wallch_he.qm
install_tr data/translations/wallch_pl.ts data/to_usr_share/wallch/translations/wallch_pl.qm
install_tr data/translations/wallch_su.ts data/to_usr_share/wallch/translations/wallch_su.qm
install_tr data/translations/wallch_el.ts data/to_usr_share/wallch/translations/wallch_el.qm
install_tr data/translations/wallch_hr.ts data/to_usr_share/wallch/translations/wallch_hr.qm
install_tr data/translations/wallch_pt.ts data/to_usr_share/wallch/translations/wallch_pt.qm
install_tr data/translations/wallch_sv.ts data/to_usr_share/wallch/translations/wallch_sv.qm
install_tr data/translations/wallch_es.ts data/to_usr_share/wallch/translations/wallch_es.qm
install_tr data/translations/wallch_it.ts data/to_usr_share/wallch/translations/wallch_it.qm
install_tr data/translations/wallch_ru.ts data/to_usr_share/wallch/translations/wallch_ru.qm
install_tr data/translations/wallch_tr.ts data/to_usr_share/wallch/translations/wallch_tr.qm
install_tr data/translations/wallch_de.ts data/to_usr_share/wallch/translations/wallch_de.qm
install_tr data/translations/wallch_nl.ts data/to_usr_share/wallch/translations/wallch_nl.qm

report_step "Installing Wallch to your system"

install_now "wallch binary" "wallch" "/usr/bin/"
install_now "/usr/share files" "data/to_usr_share/wallch/" "/usr/share/"
install_now "bash autocompletion" "data/bash_autocompletion/wallch" "/etc/bash_completion.d/"
install_now "manpage" "data/man/wallch.1.gz" "/usr/share/man/man1/"
install_now "program pixmap" "data/pixmap/wallch.png" "/usr/share/pixmaps/"
install_now "desktop file" "data/wallch-nautilus.desktop" "/usr/share/applications/"

report_step "Installation finished. Thank you for using Wallch."
