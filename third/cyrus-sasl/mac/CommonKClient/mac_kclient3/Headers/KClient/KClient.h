/* * KClient 3.0 API declarations * See KClient30-API.html * * $Header: /afs/dev.mit.edu/source/repository/third/cyrus-sasl/mac/CommonKClient/mac_kclient3/Headers/KClient/KClient.h,v 1.1.1.1 2002-10-13 17:59:26 ghudson Exp $ */#ifndef	__KCLIENT__#define	__KCLIENT__/* Constants */enum {	/* No error */	kcNoError					= 0,		/* General runtime errors */	kcErrNoMemory				= 23000,	kcErrBadParam,	/* Various invalid structures */	kcErrInvalidSession			= 23010,	kcErrInvalidPrincipal,	kcErrInvalidAddress,	kcErrInvalidFile,	/* Missing required settings in the session */	kcErrNoClientPrincipal		= 23020,	kcErrNoServerPrincipal,	kcErrNoLocalAddress,	kcErrNoRemoteAddress,	kcErrNoSessionKey,	kcErrNoServiceKey,	kcErrNoChecksum,		kcErrNotLoggedIn			= 23030,	kcErrUserCancelled,	kcErrIncorrectPassword,		kcErrBufferTooSmall			= 23040,	kcErrKeyFileAccess,	kcErrFileNotFound,	kcErrInvalidPreferences,	kcErrChecksumMismatch,		kcFirstKerberosError				= 20000,	kcLastKerberosError					= kcFirstKerberosError + 256};#ifndef rez#include <KerberosSupport/KerberosSupport.h>#include <CredentialsCache/CredentialsCache.h>#include <KerberosProfile/KerberosProfile.h>#include <KerberosDES/KerberosDES.h>#include <KClient/KClientTypes.h>#if TARGET_API_MAC_OSX && TARGET_API_MAC_CARBON    #include <CoreServices/CoreServices.h>#elif TARGET_API_MAC_OS8 || TARGET_API_MAC_CARBON    #include <Files.h>#else    #error "Unknown OS"#endif#ifdef __cplusplusextern "C" {#endif/* Functions */OSStatus KClientGetVersion (	UInt16*					outMajorVersion,	UInt16*					outMinorVersion,	const char**			outVersionString);/* Initialization / destruction */OSStatus KClientNewClientSession (	KClientSession*			outSession);	OSStatus KClientNewServerSession (	KClientSession*			inSession,	KClientPrincipal		inService);	OSStatus KClientDisposeSession (	KClientSession			inSession);	/* Accessing session properties */OSStatus KClientGetClientPrincipal (	KClientSession			inSession,	KClientPrincipal*		outPrincipal);	OSStatus KClientSetClientPrincipal (	KClientSession			inSession,	KClientPrincipal		inPrincipal);	OSStatus KClientGetServerPrincipal (	KClientSession			inSession,	KClientPrincipal*		outPrincipal);	OSStatus KClientSetServerPrincipal (	KClientSession			inSession,	KClientPrincipal		inPrincipal);	OSStatus KClientGetLocalAddress (	KClientSession			inSession,	KClientAddress*			outLocalAddress);	OSStatus KClientSetLocalAddress (	KClientSession			inSession,	const KClientAddress*	inLocalAddress);	OSStatus KClientGetRemoteAddress (	KClientSession			inSession,	KClientAddress*			outRemoteAddress);	OSStatus KClientSetRemoteAddress (	KClientSession			inSession,	const KClientAddress*	inRemoteAddress);	OSStatus KClientGetSessionKey (	KClientSession			inSession,	KClientKey*				outKey);OSStatus KClientGetExpirationTime (	KClientSession			inSession,	UInt32*					outExpiration);	OSStatus KClientSetKeyFile (	KClientSession			inSession,	const KClientFile*		inKeyFile);	/* Logging in and out (client) */OSStatus KClientLogin (	KClientSession			inSession);OSStatus KClientPasswordLogin (	KClientSession			inSession,	const char* 			inPassword);	OSStatus KClientKeyFileLogin (	KClientSession			inSession);/*OSStatus KClientKeyLogin (	KClientSession			inSession,	const KClientKey*		inKey);*/	OSStatus KClientLogout (	KClientSession			inSession);	/* Accessing service keys (server) */OSStatus KClientGetServiceKey (	KClientSession			inSession,	UInt32					inVersion,	KClientKey*				outKey);	OSStatus KClientAddServiceKey (	KClientSession			inSession,	UInt32					inVersion,	const KClientKey*		inKey);	/* Authenticating to a service (client) */OSStatus KClientGetTicketForService (	KClientSession			inSession,	UInt32					inChecksum,	void*					outBuffer,	UInt32*					ioBufferLength);	OSStatus KClientGetAuthenticatorForService (	KClientSession			inSession,	UInt32					inChecksum,	const char*				inApplicationVersion,	void*					outBuffer,	UInt32*					ioBufferLength);OSStatus KClientVerifyEncryptedServiceReply (	KClientSession			inSession,	const void*				inBuffer,	UInt32					inBufferLength);	OSStatus KClientVerifyProtectedServiceReply (	KClientSession			inSession,	const void*				inBuffer,	UInt32					inBufferLength);	/* Authenticating a client (server) */OSStatus KClientVerifyAuthenticator (	KClientSession			inSession,	const void*				inBuffer,	UInt32					inBufferLength);	OSStatus KClientGetEncryptedServiceReply (	KClientSession			inSession,	void*					outBuffer,	UInt32*					ioBufferSize);	OSStatus KClientGetProtectedServiceReply (	KClientSession			inSession,	void*					outBuffer,	UInt32*					ioBufferSize);	/* Communicating between a server and a client */OSStatus KClientEncrypt (	KClientSession			inSession,	const void*				inPlainBuffer,	UInt32					inPlainBufferLength,	void*					outEncryptedBuffer,	UInt32*					ioEncryptedBufferLength);OSStatus KClientDecrypt (	KClientSession			inSession,	void*					inEncryptedBuffer,	UInt32					inDecryptedBufferLength,	UInt32*					outPlainOffset,	UInt32*					outPlainLength);	OSStatus KClientProtectIntegrity (	KClientSession			inSession,	const void*				inPlainBuffer,	UInt32					inPlainBufferLength,	void*					outProtectedBuffer,	UInt32*					ioProtectedBufferLength);OSStatus KClientVerifyIntegrity (	KClientSession			inSession,	void*					inProtectedBuffer,	UInt32					inProtectedBufferLength,	UInt32*					outPlainOffset,	UInt32*					outPlainLength);	/* Miscellaneous */OSStatus KClientPasswordToKey (	KClientSession			inSession,	const char*				inPassword,	KClientKey*				outKey);	/* Getting to other APIs */OSStatus KClientGetCCacheReference (	KClientSession			inSession,	cc_ccache_t*			outCCacheReference);OSStatus KClientGetProfileHandle (	KClientSession			inSession,	profile_t*				outProfileHandle);/* Principal manipulation */OSStatus KClientV4StringToPrincipal (	const char*				inPrincipalString,	KClientPrincipal*		outPrincipal);	OSStatus KClientPrincipalToV4String (	KClientPrincipal		inPrincipal,	char*					outPrincipalString);	OSStatus KClientPrincipalToV4Triplet (	KClientPrincipal		inPrincipal,	char*					outName,	char*					outInstance,	char*					outRealm);	OSStatus KClientDisposePrincipal (	KClientPrincipal		inPrincipal);		#ifdef __cplusplus}#endif#endif /* !rez */#endif /* __KCLIENT__ */