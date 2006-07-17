;;
;;  czech.nsh
;;
;;  Czech language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1250
;;  Author: Lubo� Stan�k <lubek@users.sourceforge.net>
;;  Version 1, Nov 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_CZECH} "Dopln�k Gaim-Encryption vy�aduje nainstalovan� Gaim. Mus�te nainstalovat Gaim p�ed instalac� dopl�ku Gaim-Encryption."

LangString GAIM-ENCRYPTION_TITLE ${LANG_CZECH} "Dopln�k Gaim-Encryption pro Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_CZECH} "Nekompatibiln� verze.$\r$\n$\r$\nTato verze dopl�ku Gaim-Encryption byla vytvo�ena pro Gaim ve verzi ${GAIM_VERSION}. Zd� se, �e m�te nainstalov�nu verzi"

LangString BAD_GAIM_VERSION_2 ${LANG_CZECH} "programu Gaim.$\r$\n$\r$\nV�ce informac� z�sk�te n�v�t�vou http://gaim-encryption.sourceforge.net."

LangString UNKNOWN_GAIM_VERSION ${LANG_CZECH} "Nelze zjistit, jak� verze programu Gaim je nainstalov�na. Ujist�te se, �e je to verze ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_CZECH} "Instalace dopl�ku Gaim-Encryption v${GAIM-ENCRYPTION_VERSION}"
LangString WELCOME_TEXT  ${LANG_CZECH} "Pozn�mka: Tato verze dopl�ku je navr�ena pro Gaim ${GAIM_VERSION}, nenainstaluje se ani nebude fungovat s jin�mi verzemi programu Gaim.\r\n\r\nKdy� aktualizujete svou verzi programu Gaim, mus�te odinstalovat nebo aktualizovat tak� tento dopln�k.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_CZECH} "Lokalizujte pros�m slo�ku, kam je nainstalov�n program Gaim"
LangString DIR_INNERTEXT ${LANG_CZECH} "Instalovat do t�to slo�ky programu Gaim:"

LangString FINISH_TITLE ${LANG_CZECH} "Instalace dopl�ku Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} je dokon�ena"
LangString FINISH_TEXT ${LANG_CZECH} "Je t�eba restartovat Gaim, aby se dopln�k na�etl. Pak jd�te do nastaven� programu Gaim a povolte dopln�k Gaim-Encryption."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_CZECH} "Dopln�k encrypt.dll m� b�t vymaz�n z va�� slo�ky dopl�k� programu Gaim. Pokra�ovat?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_CZECH} "Gaim-Encryption dopln�k (pouze odebrat)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_CZECH} "Odinstal�tor nem��e naj�t polo�ky registru pro Gaim-Encryption.$\rNejsp�e instaloval dopln�k jin� u�ivatel."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_CZECH} "Nem�te dostate�n� opr�vn�n� pro odinstalaci dopl�ku."



