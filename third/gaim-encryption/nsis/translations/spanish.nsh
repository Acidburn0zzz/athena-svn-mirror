;;
;;  spanish.nsh
;;
;;  Spanish language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1252
;;  Author: Javier Fern�ndez-Sanguino Pe�a <jfs@computer.org>
;;  Version 1, sept 2004 
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_SPANISH} "Gaim-Encryption necesita que Gaim est� instalado. Debe instalar Gaim antes de instalar Gaim-Encryption."

LangString GAIM-ENCRYPTION_TITLE ${LANG_SPANISH} "Complemento de Cifrado de Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_SPANISH} "Versi�n incompatible.$\r$\n$\r$\nEsta versi�n del complemento de Cifrado de Gaim se prepar� para la versi�n ${GAIM_VERSION} de Gaim.  Parece que vd. tiene la versi�n de Gaim"

LangString BAD_GAIM_VERSION_2 ${LANG_SPANISH} "instalada.$\r$\n$\r$\nPara m�s informaci�n consulte http://gaim-encryption.sourceforge.net"

LangString UNKNOWN_GAIM_VERSION ${LANG_SPANISH} "No puedo determinar la versi�n de Gaim que tiene instalada. Aseg�rese de que es la versi�n ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_SPANISH} "Instalador de Gaim-Encryption v${GAIM-ENCRYPTION_VERSION}"
LangString WELCOME_TEXT  ${LANG_SPANISH} "Aviso: Esta versi�n del complemento fue dise�ada para la versi�n ${GAIM_VERSION} de Gaim y no se podr� instalar ni funcionar� con otras versiones.\r\n\r\nCuando actualice su versi�n de Gaim debe desinstalar o actualizar este complemento.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_SPANISH} "Por favor, localice el directorio donde est� instalado Gaim"
LangString DIR_INNERTEXT ${LANG_SPANISH} "Instalar en este directorio de Gaim:"

LangString FINISH_TITLE ${LANG_SPANISH} "Se ha completado la instalaci�n de Gaim-Encryption v${GAIM-ENCRYPTION_VERSION}"
LangString FINISH_TEXT ${LANG_SPANISH} "Deber� reiniciar Gaim para que se cargue el complemento, despu�s vaya a las preferencias de Gaim y active el complemento de Cifrado de Gaim."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_SPANISH} "Se va a borrar el complemento encrypt.dll de su directorio de complementos de Gaim. �Desea continuar?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_SPANISH} "Complemento Gaim-Encryption Plugin (s�lo desinstalaci�n)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_SPANISH} "El desinstalador no pudo encontrar las entradas de registro de Gaim-Encryption.$\rEs posible que otro usuario haya instalado el complemento."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_SPANISH} "No tiene los permisos necesarios para desinstalar el complemento."



