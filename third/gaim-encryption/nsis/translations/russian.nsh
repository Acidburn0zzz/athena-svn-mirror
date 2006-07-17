;;
;;  russian.nsh
;;
;;  Russian language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 1251
;;  Author: Roman Sosenko <coloc75@yahoo.com>
;;  Version 1, Dec 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_RUSSIAN} "����� ���������� Gaim encryption ���������� ���������� Gaim. ���������� Gaim."

LangString GAIM-ENCRYPTION_TITLE ${LANG_RUSSIAN} "������ Gaim-Encryption ��� Gaim"

LangString BAD_GAIM_VERSION_1 ${LANG_RUSSIAN} "������������� ������.$\r$\n$\r$\n��� ������ ������ Gaim-Encryption ���� ������� ��� ������ Gaim ${GAIM_VERSION}. �� ����� ���������� ����������� ������ Gaim"

LangString BAD_GAIM_VERSION_2 ${LANG_RUSSIAN} "�����������.$\r$\n$\r$\n�������� http://gaim-encryption.sourceforge.net ��� ����� ��������� ����������."

LangString UNKNOWN_GAIM_VERSION ${LANG_RUSSIAN} "��� ����������, ����� ������ Gaim ����������� �� ����� ����������. ���������, ��� ��� ������ ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_RUSSIAN} "����������� Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} "
LangString WELCOME_TEXT  ${LANG_RUSSIAN} "��������: ��������� ������ ������ ������� ��� Gaim ${GAIM_VERSION}, � �� ����� ���� ����������� � ��������������� � ������� �������� Gaim.\r\n\r\n� ������ ���������� ������ Gaim ��� ���������� ����������������� ��� �������� ����� � ������.\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_RUSSIAN} "����������, ������� �������, � ������� ���������� Gaim"
LangString DIR_INNERTEXT ${LANG_RUSSIAN} "���������� � ��� ����� Gaim:"

LangString FINISH_TITLE ${LANG_RUSSIAN} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} ��������� ���������"
LangString FINISH_TEXT ${LANG_RUSSIAN} "��� �������� ������ ��� ����� ���������� ��������� Gaim ������, ����� � ������ Gaim ������������ ������ Gaim-Encryption."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_RUSSIAN} "������ encrypt.dll ����� ����� � ������ �������� Gaim/������. ����������?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_RUSSIAN} "������ Gaim-Encryption  (������ ��������)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_RUSSIAN} "������������� �� ����� ����� �������� ������� Gaim-Encryption.$\r��������, ������ ��� ���������� ������ �������������."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_RUSSIAN} "� ��� ��� ���� ��� ������������� ������."



