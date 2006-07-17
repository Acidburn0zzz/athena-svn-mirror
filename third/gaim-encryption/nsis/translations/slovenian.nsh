;;
;;  slovenian.nsh
;;
;;  Slovenian language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1250
;;  Author: Martin Srebotnjak <miles@filmsi.net>
;;  Version 1, oct 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_SLOVENIAN} "�ifriranje Gaim zahteva name��eni Gaim. Pred namestitvijo �ifriranja Gaim morate namestiti Gaim."

LangString GAIM-ENCRYPTION_TITLE ${LANG_SLOVENIAN} "Vstavek �ifriranje Gaim za Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_SLOVENIAN} "Incompatible version.$\r$\n$\r$\nTa razli�ica vstavka �ifriranja Gaim je prirejena za Gaim razli�ice ${GAIM_VERSION}.  Videti je, da imate name��eno Gaim razli�ice "

LangString BAD_GAIM_VERSION_2 ${LANG_SLOVENIAN} ".$\r$\n$\r$\nZa ve� informacij si poglejte stran http://gaim-encryption.sourceforge.net."

LangString UNKNOWN_GAIM_VERSION ${LANG_SLOVENIAN} "Ni mogo�e ugotoviti, katera razli�ica Gaima je name��ena.  Prepri�ajte se, da je to razli�ica ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_SLOVENIAN} "Namestitev �ifriranja Gaim v${GAIM-ENCRYPTION_VERSION}"
LangString WELCOME_TEXT  ${LANG_SLOVENIAN} "Opomba: Ta razli�ica vstavka je prirejena za Gaim ${GAIM_VERSION} in ne bo name��ena ali delovala z drugimi razli�icami Gaima.\r\n\r\nKo nadgradite razli�ico Gaima, ga morate odstraniti ali prav tako nadgraditi ta vstavek.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_SLOVENIAN} "Prosimo poi��ite imenik, kjer je name��en Gaim"
LangString DIR_INNERTEXT ${LANG_SLOVENIAN} "Namesti v ta imenik Gaim:"

LangString FINISH_TITLE ${LANG_SLOVENIAN} "Namestitev �ifriranja Gaim v${GAIM-ENCRYPTION_VERSION} dokon�ana"
LangString FINISH_TEXT ${LANG_SLOVENIAN} "Za nalaganje vstavka morate ponovno zagnati Gaim ter v Mo�nostih Gaima omogo�iti vstavek �ifriranje Gaim."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_SLOVENIAN} "Datoteka encrypt.dll bo zbrisana iz imenika Gaim/plugins.  �elite nadaljevati?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_SLOVENIAN} "Vstavek �ifriranje Gaim (samo odstrani)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_SLOVENIAN} "Program za odstranitev programa v registru ne najde vnosov za �ifriranje Gaim.$\rVerjetno je, da je vstavek namestil drug uporabnik."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_SLOVENIAN} "Nimate potrebnih pravic za odstranitev vstavka."



