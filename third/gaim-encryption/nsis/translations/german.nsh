;;
;;  german.nsh
;;
;;  German language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1252
;;  Author: Karim Malhas <karim@malhas.de>
;;  Version 1, sept 2004 
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_GERMAN} "Gaim-Encryption ben�tigt eine installierte Gaim Version. Sie m�ssen Gaim installieren bevor sie Gaim-Encryption installieren."

LangString GAIM-ENCRYPTION_TITLE ${LANG_GERMAN} "Gaim-Encryption Plugin f�r Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_GERMAN} "Inkompatible Version.$\r$\n$\r$\nDiese Gaim-Encryption Plugin Version wurde f�r Gaim Version ${GAIM_VERSION} erstellt.  Sie scheinen Gaim Version"

LangString BAD_GAIM_VERSION_2 ${LANG_GERMAN} "installiert zu haben.$\r$\n$\r$\nBesuchen sie http://gaim-encryption.sourceforge.net f�r weitere Informationen."

LangString UNKNOWN_GAIM_VERSION ${LANG_GERMAN} "Ich kann nicht erkennen welche Gaim version installiert ist.  Vergewissern sie sich das es Version ${GAIM_VERSION} ist."

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_GERMAN} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} Installallationsprogramm"
LangString WELCOME_TEXT  ${LANG_GERMAN} "Anmerkung:Diese Version des Plugins wurde f�r Gaim Version ${GAIM_VERSION} erstellt, und wird mit anderen anderen Versionen von Gaim weder installierbar sein noch funktionieren .\r\n\r\nWenn sie Gaim upgraden, m�ssen sie dieses Plugin entweder deinstallieren oder auch upgraden.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_GERMAN} "Bitte w�hlen sie das Verzeichnis aus in dem Gaim installiert ist"
LangString DIR_INNERTEXT ${LANG_GERMAN} "Gaim in diesem Ordner installieren:"

LangString FINISH_TITLE ${LANG_GERMAN} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} Installations Abgeschlossen."
LangString FINISH_TEXT ${LANG_GERMAN} "Sie m�ssen Gaim neu starten damit das Plugin geladen wird. Dann k�nnen sie das Plugin in den Optionen aktivieren."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_GERMAN} "Das encrypt.dll Plugin wird jetzt aus ihrem Gaim/Plugin Verzeichnis gel�scht.  Fortfahren?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_GERMAN} "Gaim-Encryption Plugin (nur entfernen)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_GERMAN} "Das Deinstallationsprogram konnte keine Registry Eintr�ge f�r Gaim-Encryption finden.$\rWarscheinlich hat ein anderer Benutzer das Plugin installiert."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_GERMAN} "Sie haben nicht die n�tigen Rechte um das Plugin zu deinstallieren."







