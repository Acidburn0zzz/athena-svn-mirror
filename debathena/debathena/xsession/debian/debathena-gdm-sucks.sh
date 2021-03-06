if [ "$GDMSESSION" = "001debathena-ttymode" ]; then
    if [ "$DEBATHENA_HOME_TYPE" = "afs" ]; then
	QUOTAPCT=$(fs lq $HOME | tail -1 | awk '{print $4}' | sed 's/[^0-9]*//g')
	if [ $QUOTAPCT -ge 100 ]; then
	    # Otherwise it blocks the session until it times out
	    pkill pulseaudio
	fi
    fi
    if zenity --question --text="This will launch an xterm which will emulate a tty-mode login session.  You can end your session by typing 'logout'.\nContinue?"; then
	xsetroot -solid grey
	xterm -ls -geometry 80x24+280+220 -display :0.0
    fi
    exit 0
fi
