;;
;;  polish.nsh
;;
;;  Polish language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1250
;;  Author: Marek Habersack <grendel@caudium.net>
;;  Version 1, sept 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_POLISH} "Gaim-Encryption wymaga by Gaim by� zainstalowany. Nale�y zainstalowa� Gaim przed instalacj� Gaim-Encryption."

LangString GAIM-ENCRYPTION_TITLE ${LANG_POLISH} "Wtyczka Gaim-Encryption dla Gaim" 

LangString BAD_GAIM_VERSION_1 ${LANG_POLISH} "Nieodpowiednia wersja.$\r$\n$\r$\nTa wersja wtyczki Gaim-Encryption zosta�a skompilowana dla wersji ${GAIM_VERSION} Gaim. Wydaje si�, �e zainstalowana wersja Gaim to" 

LangString BAD_GAIM_VERSION_2 ${LANG_POLISH} "$\r$\n$\r$\nOdwied� http://gaim-encryption.sourceforge.net by uzyska� wi�cej informacji."

LangString UNKNOWN_GAIM_VERSION ${LANG_POLISH} "Nie potrafi� okre�li� wersji zainstalowanego Gaim'a. Upewnij si�, �e jest to wersja ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_POLISH} "Instalator Gaim-Encryption v${GAIM-ENCRYPTION_VERSION}"

LangString WELCOME_TEXT  ${LANG_POLISH} "Uwaga: ta wersja wtyczki zosta�a zaprojektowana dla Gaim ${GAIM_VERSION} i nie b�dzie dzia�a�a z innymi wersjami Gaim.\r\n\r\nPrzy ka�dej aktualizacji Gaim nale�y r�wnie� zaktualizowa� t� wtyczk�.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_POLISH} "Prosz� wskaza� katalog w kt�rym zainstalowano Gaim"

LangString DIR_INNERTEXT ${LANG_POLISH} "Instaluj w poni�szym katalogu Gaim:"

LangString FINISH_TITLE ${LANG_POLISH} "Instalacja Gaim-Encryption v{GAIM_ENCRYPTION_VERSION} zako�czona"

LangString FINISH_TEXT ${LANG_POLISH} "Aby Gaim m�g� u�ywa� nowej wtyczki nale�y go zrestartowa� a nast�pnie uaktywni� wtyczk� w okienku konfiguracyjnym Gaim."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_POLISH} "Wtyczka encrypt.dll zostanie usuni�ta z katalogu wtyczek Gaim. Kontynuowa�?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_POLISH} "Wtyczka Gaim-Encryption (tylko usuwanie)"

LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_POLISH} "Skrypt deinstalacyjny nie m�g� usun�� wpis�w w rejestrze dotycz�cych wtyczki Gaim-Encryption.$\rJest prawdopodobne, �e inny u�ytkownik r�wnie� zainstalowa� wtyczk�."

LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_POLISH} "Nie posiadasz wystarczaj�cych uprawnien aby zainstalowa� wtyczk�."

