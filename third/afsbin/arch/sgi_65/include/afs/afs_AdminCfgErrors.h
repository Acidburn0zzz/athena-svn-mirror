/*
 * afs_AdminCfgErrors.h:
 * This file is automatically generated; please do not edit it.
 */
#define ADMCFGNOTSUPPORTED                       (17920L)
#define ADMCFGHOSTNAMENULL                       (17921L)
#define ADMCFGHOSTNAMETOOLONG                    (17922L)
#define ADMCFGHOSTHANDLEPNULL                    (17923L)
#define ADMCFGHOSTHANDLENULL                     (17924L)
#define ADMCFGHOSTHANDLEBADMAGIC                 (17925L)
#define ADMCFGHOSTHANDLEINVALID                  (17926L)
#define ADMCFGHOSTHANDLEHOSTNAMENULL             (17927L)
#define ADMCFGHOSTHANDLECELLHANDLENULL           (17928L)
#define ADMCFGHOSTHANDLECELLNAMENULL             (17929L)
#define ADMCFGADMINPRINCIPALNULL                 (17930L)
#define ADMCFGADMINPRINCIPALTOOLONG              (17931L)
#define ADMCFGPASSWDNULL                         (17932L)
#define ADMCFGCONFIGSTATUSPNULL                  (17933L)
#define ADMCFGCELLNAMEPNULL                      (17934L)
#define ADMCFGSERVERBASICINFOINVALID             (17935L)
#define ADMCFGSERVERNOTINCELL                    (17936L)
#define ADMCFGSERVERNOKEYS                       (17937L)
#define ADMCFGSERVERCELLNOTINDB                  (17938L)
#define ADMCFGSERVERCELLHASNODBENTRIES           (17939L)
#define ADMCFGCELLNAMENULL                       (17940L)
#define ADMCFGCELLNAMETOOLONG                    (17941L)
#define ADMCFGCELLNAMECONFLICT                   (17942L)
#define ADMCFGCELLDBHOSTSNULL                    (17943L)
#define ADMCFGCELLDBHOSTCOUNTTOOLARGE            (17944L)
#define ADMCFGSERVERSETCELLFAILED                (17945L)
#define ADMCFGBOSSERVERACTIVE                    (17946L)
#define ADMCFGVPTABLEPNULL                       (17947L)
#define ADMCFGVPTABLECOUNTPNULL                  (17948L)
#define ADMCFGVPTABLEREADFAILED                  (17949L)
#define ADMCFGPARTITIONNAMENULL                  (17950L)
#define ADMCFGPARTITIONNAMEBAD                   (17951L)
#define ADMCFGDEVICENAMENULL                     (17952L)
#define ADMCFGDEVICENAMEBAD                      (17953L)
#define ADMCFGVPTABLEENTRYBAD                    (17954L)
#define ADMCFGVPTABLEWRITEFAILED                 (17955L)
#define ADMCFGVALIDFLAGPNULL                     (17956L)
#define ADMCFGINSTALLEDFLAGPNULL                 (17957L)
#define ADMCFGVERSIONPNULL                       (17958L)
#define ADMCFGSTARTEDFLAGPNULL                   (17959L)
#define ADMCFGCLIENTBASICINFOINVALID             (17960L)
#define ADMCFGCLIENTNOTINCELL                    (17961L)
#define ADMCFGCLIENTCELLNOTINDB                  (17962L)
#define ADMCFGCLIENTCELLHASNODBENTRIES           (17963L)
#define ADMCFGCLIENTVERSIONNOTREAD               (17964L)
#define ADMCFGCLIENTCELLSERVDBNOTREAD            (17965L)
#define ADMCFGCLIENTCELLSERVDBNOTWRITTEN         (17966L)
#define ADMCFGCLIENTCELLSERVDBNOSPACE            (17967L)
#define ADMCFGCLIENTCELLSERVDBEDITFAILED         (17968L)
#define ADMCFGCLIENTTHISCELLNOTWRITTEN           (17969L)
#define ADMCFGBOSSERVERCTLSERVICEBAD             (17970L)
#define ADMCFGBOSSERVERCTLSERVICENOTREADY        (17971L)
#define ADMCFGBOSSERVERCTLSERVICETIMEOUT         (17972L)
#define ADMCFGBOSSERVERCTLSERVICESTATUSUNK       (17973L)
#define ADMCFGHOSTSETNOAUTHFAILED                (17974L)
#define ADMCFGBOSSERVERPROCSFLAGPNULL            (17975L)
#define ADMCFGDBSERVERCONFIGFLAGPNULL            (17976L)
#define ADMCFGFILESERVERCONFIGFLAGPNULL          (17977L)
#define ADMCFGUPSERVERCONFIGFLAGPNULL            (17978L)
#define ADMCFGUPCLIENTSUFFIXNULL                 (17979L)
#define ADMCFGUPCLIENTSUFFIXTOOLONG              (17980L)
#define ADMCFGUPCLIENTTARGETSERVERNULL           (17981L)
#define ADMCFGUPCLIENTIMPORTDIRNULL              (17982L)
#define ADMCFGUPCLIENTCONFIGFLAGPNULL            (17983L)
#define ADMCFGUBIKVOTENOCONNECTION               (17984L)
#define ADMCFGQUORUMWAITTIMEOUT                  (17985L)
#define ADMCFGCELLSERVDBTOOMANYENTRIES           (17986L)
#define ADMCFGCALLBACKNULL                       (17987L)
#define ADMCFGUPDATECOUNTNULL                    (17988L)
#define ADMCFGAFSKEYNOTAVAILABLE                 (17989L)
#define ADMCFGAFSPASSWDINVALID                   (17990L)
#define ADMCFGCANTRESOLVEHOSTNAME                (17991L)
#define ADMCFGRESOLVEDHOSTNAMETOOLONG            (17992L)
#define ADMCFGCACHEMGRSERVICEBAD                 (17993L)
#define ADMCFGCACHEMGRSERVICENOTREADY            (17994L)
#define ADMCFGCACHEMGRSERVICETIMEOUT             (17995L)
#define ADMCFGCACHEMGRSERVICESTATUSUNK           (17996L)
extern void initialize_af_error_table ();
#define ERROR_TABLE_BASE_af (17920L)

/* for compatibility with older versions... */
#define init_af_err_tbl initialize_af_error_table
#define af_err_base ERROR_TABLE_BASE_af
