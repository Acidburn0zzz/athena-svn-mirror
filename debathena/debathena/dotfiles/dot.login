# Prototype user .login file
#
# $Id: dot.login,v 1.16 1999-06-03 14:52:56 danw Exp $

# This file sources a system-wide .login file, which:
#      - presumes that the .cshrc file has been sourced
#      - performs standard setup appropriate for a tty session
#      - runs standard startup activities (e.g., checking mail)
#      - sources user file ~/.startup.tty, if it exists

set initdir=/usr/athena/lib/init

if (-r $initdir/login) then
        source $initdir/login
else
	echo "Warning: System-wide initialization files not found."
       	echo "Login initialization has not been performed."
endif


# If you want to ADJUST the login initialization sequence, create a
# .startup.tty file in your home directory, with commands to run activities
# once the environment has been set up (znol, emacs, etc.).

# To adjust the environment initialization sequence, see the instructions in
# the .cshrc file.

# In most cases, you will never need to edit this file.  All the 
# customizations you could want to make can be made by editing one
# of the user dotfiles, such as ~/.cshrc.mine, ~/.environment, 
# ~/.startup.tty or ~/.startup.X
#
# WARNING: If you revise this .login file, you will not automatically
# get any changes that Athena may make to the system-wide file at a
# later date. Be sure you know what you are doing.
