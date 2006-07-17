;;
;;  japanese.nsh
;;
;;  Default language strings for the Windows Gaim-encryption NSIS installer.
;;  Windows Code page: 932
;;
;;  Author: Takeshi AIHANA <aihana@gnome.gr.jp>
;;  Version 1, sept 2004
;;

; Startup Gaim check
LangString GAIM_NEEDED ${LANG_JAPANESE} "Gaim-Encryption �𗘗p����ɂ� Gaim ���C���X�g�[������Ă���K�v������܂��BGaim-Encryption ���C���X�g�[������O�� Gaim ���C���X�g�[�����ĉ������B"

LangString GAIM-ENCRYPTION_TITLE ${LANG_JAPANESE} "Gaim ������ Gaim-Encryption �v���O�C��"

LangString BAD_GAIM_VERSION_1 ${LANG_JAPANESE} "�o�[�W�����������Ă��܂���B$\r$\n$\r$\n���̃o�[�W������ Gaim-Encryption �v���O�C���� Gaim �o�[�W���� ${GAIM_VERSION} �����ɊJ�����ꂽ���̂ł��B���� Gaim �o�[�W�������C���X�g�[�����Ă���ƕ\������܂�:"

LangString BAD_GAIM_VERSION_2 ${LANG_JAPANESE} "$\r$\n$\r$\n����ɏڍׂȏ��ȏ��ɂ��Ă� http://gaim-encryption.sourceforge.net �������������B"

LangString UNKNOWN_GAIM_VERSION ${LANG_JAPANESE} "�C���X�g�[������Ă��� Gaim �̃o�[�W�������擾�ł��܂���ł����B�o�[�W���� ${GAIM_VERSION} �ł��邱�Ƃ��m�F���ĉ������B"

; Overrides for default text in windows:

LangString WELCOME_TITLE ${LANG_JAPANESE} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} �C���X�g�[��"
LangString WELCOME_TEXT  ${LANG_JAPANESE} "����: ���̃o�[�W�����̃v���O�C���́AGaim �o�[�W���� ${GAIM_VERSION} �����ɐ݌v���ꂽ���̂ŁA����ȊO�̃o�[�W�����ł̓C���X�g�[���A�܂��͓��삵�Ȃ���������܂���B\r\n\r\n���g���� Gaim ���A�b�v�O���[�h����ۂ͓��l�ɁA���̃v���O�C�����A���C���X�g�[���A�܂��̓A�b�v�O���[�h���ĉ������B\r\n\r\n"

LangString DIR_SUBTITLE ${LANG_JAPANESE} "Gaim ���C���X�g�[������Ă���t�H���_���w�肵�ĉ�����"
LangString DIR_INNERTEXT ${LANG_JAPANESE} "���� Gaim �t�H���_�̒��ɃC���X�g�[������:"

LangString FINISH_TITLE ${LANG_JAPANESE} "Gaim-Encryption v${GAIM-ENCRYPTION_VERSION} �̃C���X�g�[�����������܂���"
LangString FINISH_TEXT ${LANG_JAPANESE} "�v���O�C����ǂݍ��ނ��߂� Gaim ���ċN�����AGaim �̐ݒ�_�C�A���O���� Gaim-Encryption �v���O�C����L���ɂ��ĉ������B"

; during install uninstaller
LangString GAIM_ENCRYPTION_PROMPT_WIPEOUT ${LANG_JAPANESE} "���g���� Gaim/�v���O�C���E�t�H���_����t�@�C�� encrypt.dll ���폜���܂��B���s���Ă���낵���ł���?"

; for windows uninstall
LangString GAIM_ENCRYPTION_UNINSTALL_DESC ${LANG_JAPANESE} "Gaim-Encryption �v���O�C�� (�폜��p)"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_1 ${LANG_JAPANESE} "�A���C���X�g�[���� Gaim-Encryption �ɑ΂��郌�W�X�g���̃G���g���������邱�Ƃ��ł��܂���ł����B$\r�����炭�N�����̃��[�U���v���O�C�����C���X�g�[�������Ǝv���܂��B"
LangString un.GAIM_ENCRYPTION_UNINSTALL_ERROR_2 ${LANG_JAPANESE} "���̃v���O�C�����A���C���X�g�[������̂ɕK�v�Ȍ���������܂���B"



