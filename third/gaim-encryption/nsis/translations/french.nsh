;;
;;  french.nsh
;;
;;  French language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1252
;;  Author: Davy Defaud <davy.defaud@free.fr>
;;  Version 1, sept 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_FRENCH} "Gaim-Encryption est un greffon (plugin) pour Gaim. Vous devez d'abord installer Gaim avant d'installer Gaim-Encryption."

LangString GAIM-ENCRYPTION_TITLE ${LANG_FRENCH} "Gaim-Encryption, greffon de chiffrement pour Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_FRENCH} "Version incompatible.$\r$\n$\r$\nCette version du greffon Gaim-Encryption a �t� compil�e pour la version ${GAIM_VERSION} de Gaim. Vous semblez poss�der la version"

LangString BAD_GAIM_VERSION_2 ${LANG_FRENCH} "de Gaim.$\r$\n$\r$\nPour plus d'information, veuillez consulter le site internet http://gaim-encryption.sourceforge.net."

LangString UNKNOWN_GAIM_VERSION ${LANG_FRENCH} "Impossible de d�tecter la version de Gaim install�e.  Veuillez vous assurer qu'il s'agit de la version ${GAIM_VERSION}."

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_FRENCH} "Installateur de Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} "
LangString WELCOME_TEXT  ${LANG_FRENCH} "Note: Cette version de Gaim-Encryption est con�ue pour Gaim ${GAIM_VERSION}, elle ne s'installera et ne fonctionnera pas avec d'autres versions de Gaim.\r\n\r\nQuand vous mettez � jour votre version de Gaim, vous devez d�sinstaller ou mettre �galement � jour Gaim-Encryption.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_FRENCH} "Veuillez indiquer le r�pertoire d'installation de Gaim."
LangString DIR_INNERTEXT ${LANG_FRENCH} "Installer dans ce dossier Gaim:"

LangString FINISH_TITLE ${LANG_FRENCH} "Installation de Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} termin�e."
LangString FINISH_TEXT ${LANG_FRENCH} "Vous devez red�marrer Gaim pour charger le greffon, ensuite vous rendre dans les pr�f�rences de Gaim et activer le greffon (plugin) Gaim-Encryption."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_FRENCH} "Le fichier encrypt.dll est sur le point d'�tre effac� de votre sous-r�pertoire Gaim/plugins. Voulez-vous continuer?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_FRENCH} "D�sinstallation de Gaim-Encryption"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_FRENCH} "Le d�sinstallateur ne peut trouver les entr�es de la base de registres concernant Gaim-Encryption.$\rIl semble qu'un autre utilisateur a install� le greffon."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_FRENCH} "Vous ne poss�dez pas les privil�ges n�cessaires pour d�sinstaller Gaim-Encryption."
