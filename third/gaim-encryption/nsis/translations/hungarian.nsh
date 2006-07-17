;;
;;  hungarian.nsh
;;
;;  Hungarian language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1250
;;  Author: Peter Tutervai <mrbay@csevego.net>
;;  Version 1, nov 2004 
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_HUNGARIAN} "A Gaim-Encryption telep�t�s�hez sz�ks�g van a Gaimra. Fel kell telep�tened a Gaim-ot a Gaim-Encryption telep�t�se el�tt."

LangString GAIM-ENCRYPTION_TITLE ${LANG_HUNGARIAN} "Gaim-Encryption plugin a Gaimhoz"

LangString BAD_GAIM_VERSION_1 ${LANG_HUNGARIAN} "Nem kompatibilis verzi�.$\r$\n$\r$\nA Gaim-Encryption plugin ezen verzi�ja a Gaim ${GAIM_VERSION} verzi�j�hoz lett leford�tva. Neked a "

LangString BAD_GAIM_VERSION_2 ${LANG_HUNGARIAN} "verzi�j� Gaim van feltelep�tve.$\r$\n$\r$\nN�zd meg a http://gaim-encryption.sourceforge.net webhelyet tov�bbi inform�ci�k�rt."

LangString UNKNOWN_GAIM_VERSION ${LANG_HUNGARIAN} "A feltelep�tett Gaim verzi�ja ismeretlen. Bizonyosodjon meg r�la, hogy a verzi�ja ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_HUNGARIAN} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} Telep�t�"
LangString WELCOME_TEXT  ${LANG_HUNGARIAN} "Fontos: A plugin ezen verzi�ja a Gaim ${GAIM_VERSION} verzi�j�hoz lett leford�tva, �s nem lesz telep�tve vagy nem fog futni a Gaim m�s verzi�ival.\r\n\r\nHa friss�ti a Gaimot, t�r�lje vagy friss�tse a plugint is.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_HUNGARIAN} "K�rlek, add meg a Gaim hely�t"
LangString DIR_INNERTEXT ${LANG_HUNGARIAN} "Telep�t�s ebbe a Gaim k�nyvt�rba:"

LangString FINISH_TITLE ${LANG_HUNGARIAN} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} Telep�t�se befejez�d�tt"
LangString FINISH_TEXT ${LANG_HUNGARIAN} "�jra kell ind�tanod a Gaimot, hogy bet�lts�n a plugin, majd a Gaim be�ll�t�sokban be kell kapcsolnod Gaim-Encryption Plugint."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_HUNGARIAN} "Az encrypt.dll plugin t�r�lve lesz a Gaim/plugins k�nyvt�rb�l. Folytassam?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_HUNGARIAN} "Gaim-Encryption Plugin (csak t�r�lhet�)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_HUNGARIAN} "Az uninstaller nem tal�lt bejegyz�seket a registryben a Gaim-Encryptionh�z.$\rVal�sz�n�leg m�sik felhaszn�l� telep�tette a Gaim-Encryptiont."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_HUNGARIAN} "Nincs jogod a Gaim-Encryption t�rl�s�hez."



