# This file is sourced by Xsession(5), not executed.

if [ afs = "$DEBATHENA_HOME_TYPE" ] && \
    [ -f "$HOME/.config/tracker/tracker.cfg" ] && \
    [ "$(awk -F= '/^EnableIndexing/ { print $2}' $HOME/.config/tracker/tracker.cfg)" != "false" ]; then
    
    message "You currently have the Tracker applet configured to index" \
	"your home directory, probably because you logged into an earlier" \
	"version of Debathena.  This is undesirable, because it can interact" \
	"poorly with the AFS cache and render AFS unusable.  To fix this," \
	"run the following command:  athrun consult fix-tracker"
fi
