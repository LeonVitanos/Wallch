#!/bin/bash
#Wallch Unonstallation file
#Created by Alex Solanos <alexsol.developer@gmail.com> for Wallch v4.0

error_report()
{
	echo -ne "ERROR: $1";
}

report_step(){
	echo -ne "\n\n"
	echo "---- $1 ----"
	echo -ne "\n\n"
}

uninstall_now()
{
	echo -ne "Uninstalling $1..."
	if [ -d "$2" ]; then
		#installing a directory
		rm -rf "$2"
	elif [ -f "$2" ]; then
		#installing a file
		rm -f "$2"
	else
		error_report "\"$2\" - No such file or directory. Skipping..."
	fi
	echo -ne "Done.\n"
}

if [[ $EUID != 0 ]]; then
	echo "Please run this uninstall script as root: sudo $0"
	exit 1
fi

report_step "Uninstalling Wallch from your system"

uninstall_now "wallch binary" "/usr/bin/wallch"
uninstall_now "/usr/share files" "/usr/share/wallch"
uninstall_now "bash autocompletion" "/etc/bash_completion.d/wallch"
uninstall_now "manpage" "/usr/share/man/man1/wallch.1.gz"
uninstall_now "program pixmap" "/usr/share/pixmaps/wallch.png"
uninstall_now "desktop file" "/usr/share/applications/wallch-nautilus.desktop"

report_step "Uninstallation finished. Thank you for using Wallch."
