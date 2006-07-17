;;
;;  danish.nsh
;;
;;  Danish language strings for the Windows Gaim-Encryption NSIS installer.
;;  Windows Code page: 1252
;;  Author: Morten Brix Pedersen <morten@wtf.dk>
;;  Version 1, sept 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_DANISH} "Gaim-Encryption kr�ver at Gaim er installeret. Du skal installere Gaim f�r du installerer Gaim-Encryption."

LangString GAIM-ENCRYPTION_TITLE ${LANG_DANISH} "Gaim-Encryption (krypterings) modul til Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_DANISh} "Inkompatibel version.$\r$\n$\r$\nDenne version af Gaim-Encryption var bygget til Gaim version ${GAIM_VERSION}. Det ser ud til at du har Gaim version"

LangString BAD_GAIM_VERSION_2 ${LANG_DANISH} "installeret.$\r$\n$\r$\nSe http://gaim-encryption.sourceforge.net for flere oplysninger."

LangString UNKNOWN_GAIM_VERSION ${LANG_DANISH} "Jeg kan ikke se hvilken Gaim version der er installeret. S�rg for at det er version ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_DANISH} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} installationsprogram"
LangString WELCOME_TEXT  ${LANG_DANISH} "Bem�rk: Denne version af modulet er lavet til Gaim ${GAIM_VERSION}, og vil ikke installere eller virke med andre versioner af Gaim.\r\n\r\nN�r du opgraderer din version af Gaim, skal du s�rge for at fjerne eller opgradere dette modul ogs�.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_DANISH} "Vis mappen hvor Gaim er installeret"
LangString DIR_INNERTEXT ${LANG_DANISH} "Install�r i denne Gaim mappe:"

LangString FINISH_TITLE ${LANG_DANISH} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} installation f�rdig"
LangString FINISH_TEXT ${LANG_DANISH} "Du skal genstarte Gaim f�r modulet indl�ses, derefter g� til indstillinger i Gaim og sl� Gaim-Encryption modulet til."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_DANISH} "encrypt.dll modulet er ved at blive fjernet fra din Gaim/plugins mappe. Forts�t?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_DANISH} "Gaim-Encryption modul (fjern kun)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_DANISH} "Af-installeringsprogrammet kunne ikke finde Gaim-Encryption i registreringsdatabasen.$\rDet kan muligt at en anden bruger har installeret modulet."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_DANISH} "Du har ikke de n�dvendige rettigheder til at installere modulet."



