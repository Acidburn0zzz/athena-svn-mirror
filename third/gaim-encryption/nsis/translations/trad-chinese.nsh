;;
;;  trad-chinese.nsh
;;
;;  Traditional Chinese language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 950
;;  Author: Tim Hsu <timhsu@info.sayya.org>
;;  Version 1, Dec 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_TRADCHINESE} "Gaim-Encryption �ݭn Gaim. �Цb�w�� Gaim-Encryption �e���w�� Gaim."

LangString GAIM-ENCRYPTION_TITLE ${LANG_TRADCHINESE} "Gaim-Encryption �[�K�Ҳ�"

LangString BAD_GAIM_VERSION_1 ${LANG_TRADCHINESE} "���ۮe������.$\r$\n$\r$\n�������� Gaim-Encryption �ҲթM Gaim ���� ${GAIM_VERSION} �L�k�ۮe."

LangString BAD_GAIM_VERSION_2 ${LANG_TRADCHINESE} "�w�w��.$\r$\n$\r$\n�Q�F�ѧ�h����T�гs�� http://gaim-encryption.sourceforge.net"

LangString UNKNOWN_GAIM_VERSION ${LANG_TRADCHINESE} "�L�k���� Gaim ������. �нT�w Gaim ������ ${GAIM_VERSION}"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_TRADCHINESE} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} �w�˵{��"
LangString WELCOME_TEXT  ${LANG_TRADCHINESE} "�`�N: �����ҲլO�w�� Gaim ${GAIM_VERSION} �ҳ]�p, �䥦������ Gaim �N�L�k���`�ϥ�.\r\n\r\n��A�n�ɯŷs�� Gaim ��, �A�����������Τ���A���s�ɯŦ��Ҳ�\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_TRADCHINESE} "�Ы��X Gaim �Ҧw�˪��ؿ����|"
LangString DIR_INNERTEXT ${LANG_TRADCHINESE} "�w�˦ܦ� Gaim �ؿ�:"

LangString FINISH_TITLE ${LANG_TRADCHINESE} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} �w�˧���"
LangString FINISH_TEXT ${LANG_TRADCHINESE} "�Э��s�Ұ� Gaim �H���J���Ҳ�, �O�o�b���n�]�w�̱Ұ� Gaim-Encryption �Ҳ�."

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_TRADCHINESE} "���ʧ@�N�q Gaim/plugins �ؿ��̲��� encrypt.dll �Ҳ�.  �O�_�T�w�n�~��?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_TRADCHINESE} "Gaim-Encryption �Ҳ� (����)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_TRADCHINESE} "�����{���䤣�� Gaim-Encryption.$\r�]�\�O�䥦�ϥΪ̦w�ˤF���Ҳ�."
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_TRADCHINESE} "�v������, �L�k�������Ҳ�"



