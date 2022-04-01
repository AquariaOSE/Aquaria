#!/bin/sh

hascmd() {
	# because `which` is non-POSIX
	type "$1" >/dev/null 2>&1
	return $?
}

# title, text, detail
msgbox()
{

echo "
#-------[ $1 ]--------
# $2
#---------------------------------
$3

"
if [ -n "$DISPLAY" ]; then
	if hascmd kdialog; then
		kdialog --error "$2\n\n$3" --title "$1" 2>/dev/null # kdialog auto-expands \n
	elif hascmd zenity; then
		zenity --error --title="$1" --text="$2\n\n$3" # zenity auto-expands \n
	elif hascmd wish; then
		echo "tk_messageBox -type ok -title \"$1\" -message \"$2\" -icon error -detail \"$3\"; exit" | wish
	elif hascmd osascript; then # apple specific
/usr/bin/osascript <<EOT
tell application "Finder"
	activate
	set myReply to button returned of (display dialog "$1\n\n$2\n\n$3" default button 1 buttons {"Close"})
end tell
EOT
	elif hascmd xmessage; then
		echo -e "$1\n\n$2\n$3" | xmessage -file -
	fi
fi
}

MACHINE=`uname -m`

# just in case. PPC not supported.
if [ $MACHINE != "i386" ] && [ $MACHINE != "x86_64" ] && [ $MACHINE != "i686" ]; then
	msgbox "Error!" "Unsupported machine architecture." "Your machine reports \"$MACHINE\", but only i386 and x86_64 are supported."
	exit 1
fi

# get our very own dir
DIR=`dirname "$0"`
DIR=`cd "$DIR" && pwd`

#cd "../../updater" # this fails if started via Finder (working dir == app dir in that case)
# so here's the safe way
OLDDIR="$DIR"
DIR=`dirname "$DIR"`
DIR=`dirname "$DIR"`
cd "$DIR/updater"

# just in case
RUNCMD=""
if hascmd wish; then
	RUNCMD=wish
elif hascmd tclsh; then
	RUNCMD=tclsh
fi

if [ -n "$RUNCMD" ]; then
	
	if [ -f main.tcl ]; then
		"$RUNCMD" main.tcl &
	else
		msgbox "Oops!" "Installer script not found!" "This indicates a script error. Please let me know about it -- fg"
	fi
else
	msgbox "Oops!" "Tcl/Wish not installed!" "I thought this was the case on Mac OSX. Please let me know about it -- fg"
fi

cd "$OLDDIR"
