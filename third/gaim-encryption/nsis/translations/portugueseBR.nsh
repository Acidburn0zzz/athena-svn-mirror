;;
;;  portugueseBR.nsh
;;
;;  Portuguese language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1252
;;  Author: Aury Fink Filho <nuny@aury.com.br>
;;  Version 1, oct 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_PORTUGUESEBR} "Gaim-Encryption requer que o Gaim esteja instalado. Voc� deve instalar o Gaim antes de instalar o Gaim-Encryption."

LangString GAIM-ENCRYPTION_TITLE ${LANG_PORTUGUESEBR} "Gaim-Encryption plugin para Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_PORTUGUESEBR} "Vers�o incompat�vel.$\r$\n$\r$\nEsta vers�o do plugin do Gaim-Encryption foi gerada para o Gaim vers�o ${GAIM_VERSION}.  Aparentemente, voc� tem o Gaim vers�o"

LangString BAD_GAIM_VERSION_2 ${LANG_PORTUGUESEBR} "instalado.$\r$\n$\r$\nVeja http://gaim-encryption.sourceforge.net para mais informa��es."

LangString UNKNOWN_GAIM_VERSION ${LANG_PORTUGUESEBR} "Eu n�o posso dizer qual vers�o do Gaim est� instalada. Verifique se � a vers�o ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_PORTUGUESEBR} "Instalador do Gaim-Encryption v${GAIM-ENCRYPTION_VERSION}"
LangString WELCOME_TEXT  ${LANG_PORTUGUESEBR} "Nota: Essa vers�o do plugin foi feita para o Gaim ${GAIM_VERSION}, e n�o ir� instalar ou funcionar com outras vers�es do Gaim.\r\n\r\nQuando voc� atualizar sua vers�o do Gaim, voc� deve desinstalar ou atualizar esse plugin tamb�m.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_PORTUGUESEBR} "Por favor, localize o diret�rio aonde o Gaim est� instalado"
LangString DIR_INNERTEXT ${LANG_PORTUGUESEBR} "Instale nessa pasta do Gaim:"

LangString FINISH_TITLE ${LANG_PORTUGUESEBR} "Instala��o do Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} Finalizada"
LangString FINISH_TEXT ${LANG_PORTUGUESEBR} "Voc� necessita reiniciar o Gaim para o plugin ser carregado, ent�o v� para as prefer�ncias do Gaim e habilite o plugin do Gaim-Encryption."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_PORTUGUESEBR} "O plugin encrypt.dll est� para ser deletado de seu diret�rio Gaim/plugins.  Continuar?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_PORTUGUESEBR} "Gaim-Encryption Plugin (apenas remover)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_PORTUGUESEBR} "O desintalador n�o pode encontrar as entradas no registro para o Gaim-Encryption.$\rAparentemente, outro usu�rio instalou o plugin."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_PORTUGUESEBR} "Voc� n�o tem as permiss�es necess�rias para desinstalar o plugin."
