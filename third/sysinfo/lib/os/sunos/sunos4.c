/*
 * Copyright (c) 1992-2000 MagniComp 
 * This software may only be used in accordance with the license which is 
 * available as http://www.magnicomp.com/sysinfo/4.0/sysinfo-eu-license.shtml
 */

#ifndef lint
static char *RCSid = "$Revision: 1.1.1.1 $";
#endif

/*
 * SunOS 4.x specific routines
 */

#include "defs.h"
#include <sys/stat.h>
#include <sys/mtio.h>

/*
 * Characters for disk partitions
 */
char 			PartChars[] = "abcdefgh";

/*
 * CPU (model) symbol
 */
char 			CpuSYM[] = "_cpu";

/*
 * Name of generic magnetic tape device.
 */
#define MTNAME		"mt"

/*
 * Generally used variables
 */
static kvm_t 		       *kd = NULL;
static struct stat 		StatBuf;
static DevInfo_t 	       *DevInfo;
static char 			Buf[BUFSIZ];
extern char		        RomVecSYM[];
static int			OpenPROMTraverse();

#if	defined(HAVE_MAINBUS)
/*
 * Build a device tree by searching the MainBus
 */

#define DV_SIZE	(sizeof(struct mb_device))
#define DR_SIZE (sizeof(struct mb_driver))
extern char		 	MainBusSYM[];

/*
 * Build device tree by looking at mainbus (mb) devices
 */
extern int BuildMainBus(TreePtr, SearchNames)
    DevInfo_t 		       **TreePtr;
    char		       **SearchNames;
{
    struct nlist	       *nlptr;
    static struct mb_device 	Device;
    static struct mb_driver 	Driver;
    static char 		CtlrName[BUFSIZ], DevName[BUFSIZ];
    static DevData_t 		DevData;
    u_long 			Addr, DeviceAddr;
    DevInfo_t 		       *DevInfo;

    /*
     * Read table address from kernel
     */
    if (!(kd = KVMopen()))
	return(-1);

    if ((nlptr = KVMnlist(kd, MainBusSYM, (struct nlist *)NULL, 0)) == NULL)
	return(-1);

    if (CheckNlist(nlptr))
	return(-1);

    /*
     * Read each device table entry.  A NULL device.mb_driver
     * indicates that we're at the end of the table.
     */
    for (DeviceAddr = nlptr->n_value; DeviceAddr; 
	 DeviceAddr += DV_SIZE) {

	/*
	 * Read this device
	 */
	if (KVMget(kd, DeviceAddr, (char *) &Device, DV_SIZE, KDT_DATA)) {
	    SImsg(SIM_GERR, "Cannot read mainbus device from address 0x%x.", 
		      DeviceAddr);
	    KVMclose(kd);
	    return(-1);
	}

	/*
	 * See if we're done.
	 */
	if (!Device.md_driver)
	    break;

	/*
	 * Read the driver structure
	 */
	Addr = (u_long) Device.md_driver;
	if (KVMget(kd, Addr, (char *) &Driver, DR_SIZE, KDT_DATA)) {
	    SImsg(SIM_GERR, "Cannot read driver for mainbus address 0x%x.", Addr);
	    continue;
	}

	/*
	 * Get the device name
	 */
	if (Addr = (u_long) Driver.mdr_dname) {
	    if (KVMget(kd, Addr, (char *) DevName, 
		       sizeof(DevName), KDT_STRING)) {
		SImsg(SIM_GERR, "Cannot read device name from address 0x%x.", Addr);
		continue;
	    }
	} else
	    DevName[0] = CNULL;

	/*
	 * Get the controller name
	 * XXX - not if "Device.md_ctlr" is -1; work around botch
	 * in current Auspex releases, where some boards (File Processor,
	 * Primary Memory, etc.) have both a device and a controller name,
	 * despite the fact that there's not both a controller and a
	 * set of 1 or more devices.
	 */
	if ((Addr = (u_long) Driver.mdr_cname) && Device.md_ctlr != -1) {
	    if (KVMget(kd, Addr, (char *) CtlrName, 
		       sizeof(CtlrName), KDT_STRING)) {
		SImsg(SIM_GERR, "Cannot read controller name from address 0x%x.", 
			  Addr);
		continue;
	    }
	} else
	    CtlrName[0] = CNULL;

	/* Make sure devdata is clean */
	bzero(&DevData, sizeof(DevData_t));

	/* Set what we know */
	if (DevName[0]) {
	    DevData.DevName = strdup(DevName);
	    DevData.DevUnit = Device.md_unit;
	}
	if (CtlrName[0]) {
	    DevData.CtlrName = strdup(CtlrName);
	    DevData.CtlrUnit = Device.md_ctlr;
	}
	/* 
	 * Mainbus devices such, as SCSI targets, may not exist
	 * but the controller reports them as present
	 */
	if (Device.md_alive)
	    DevData.Flags |= DD_MAYBE_ALIVE;

	SImsg(SIM_DBG, "MainBus: Found <%s> (Unit %d) on <%s> (Unit %d) %s",
	      DevData.DevName, DevData.DevUnit,
	      DevData.CtlrName, DevData.CtlrUnit,
	      (DevData.Flags & DD_MAYBE_ALIVE) ? "[MAYBE-ALIVE]" : "");

	/* Probe and add device */
	if (DevInfo = ProbeDevice(&DevData, TreePtr, SearchNames, NULL))
	    AddDevice(DevInfo, TreePtr, (char **)NULL);
    }

    KVMclose(kd);
    return(0);
}
#endif	/* HAVE_MAINBUS */

/*
 * Get disk info structure.
 */
static DKinfo *GETdk_info(d, file)
    int 			d;
    char 		       *file;
{
    static DKinfo 		dk_info;

    if (ioctl(d, DKIOCINFO, &dk_info) < 0) {
	SImsg(SIM_DBG, "%s: DKIOCINFO: %s.", file, SYSERR);
	return(NULL);
    }

    return(&dk_info);
}

/*
 * Get disk configuration structure.
 */
static DKconf *GETdk_conf(d, file, disktype)
    int 			d;
    char 		       *file;
    int				disktype;
{
    static DKconf 		dk_conf;

    if (disktype == DKT_CDROM) {
	if (Debug) 
	    SImsg(SIM_GERR, "%s: Get CDROM disk configuration info is not supported.",
		  file);
	return((DKconf *) NULL);
    }

    if (ioctl(d, DKIOCGCONF, &dk_conf) < 0) {
	SImsg(SIM_DBG, "%s: DKIOCGCONF: %s.", file, SYSERR);
	return((DKconf *) NULL);
    }

    return(&dk_conf);
}

/*
 * Get disk geometry structure.
 */
static DKgeom *GETdk_geom(d, file, disktype)
    int 			d;
    char 		       *file;
    int				disktype;
{
    static DKgeom 		dk_geom;

    if (disktype == DKT_CDROM) {
	if (Debug) 
	    SImsg(SIM_GERR, "%s: Get CDROM disk geometry info is not supported.", file);
	return((DKgeom *) NULL);
    }

    if (ioctl(d, DKIOCGGEOM, &dk_geom) < 0) {
	SImsg(SIM_DBG, "%s: DKIOCGGEOM: %s.", file, SYSERR);
	return((DKgeom *) NULL);
    }

    return(&dk_geom);
}

/*
 * Get disk type structure.
 */
static DKtype *GETdk_type(d, file)
    int 			d;
    char 		       *file;
{
    static DKtype 		dk_type;

    if (ioctl(d, DKIOCGTYPE, &dk_type) < 0) {
	if (errno != ENOTTY)
	    SImsg(SIM_DBG, "%s: DKIOCGTYPE: %s.", file, SYSERR);
	return(NULL);
    }

    return(&dk_type);
}

/*
 * Check the checksum of a disklabel.
 */
static int DkLblCheckSum(DkLabel)
    DKlabel 		       *DkLabel;
{
    register short 	       *Ptr, Sum = 0;
    register short 		Count;

    Count = (sizeof (DKlabel)) / (sizeof (short));
    Ptr = (short *)DkLabel;

    /*
     * Take the xor of all the half-words in the label.
     */
    while (Count--)
	Sum ^= *Ptr++;

    /*
     * The total should be zero for a correct checksum
     */
    return(Sum);
}

/*
 * Get label information from label on disk.
 * The label is stored in the first sector of the disk.
 * We use the driver specific "read" flag with the DKIOCSCMD
 * ioctl to read the first sector.  There should be a special
 * ioctl to just read the label.
 */
static DKlabel *GETdk_label(d, file, dk_info, disktype)
    int 			d;
    char 		       *file;
    DKinfo	 	       *dk_info;
    int				disktype;
{
    struct dk_cmd 		dk_cmd;
    static DKlabel	 	dk_label;
    DevDefine_t		       *DevDefine;

    if (!file || !dk_info)
	return((DKlabel *) NULL);

    /*
     * CDROM's don't support DKIOCSCMD and doing a DKIOCSCMD on
     * a CDROM drive can sometimes crash a system.
     */
    if (disktype == DKT_CDROM) {
	SImsg(SIM_DBG, "%s: Reading CDROM labels is not supported.", file);
	return((DKlabel *) NULL);
    }

    DevDefine = DevDefGet(NULL, DT_DISKCTLR, dk_info->dki_ctype);
    if (!DevDefine) {
	SImsg(SIM_GERR, "Controller type %d is unknown.", dk_info->dki_ctype);
	return((DKlabel *) NULL);
    }

    if (DevDefine->DevFlags <= 0) {
	if (Debug)
	    SImsg(SIM_GERR, 
		  "Read block on controller type \"%s\" is unsupported.",
		  DevDefine->Model);
	return((DKlabel *) NULL);
    }

    bzero((char *) &dk_cmd, sizeof(dk_cmd));
    dk_cmd.dkc_cmd = DevDefine->DevFlags;
    dk_cmd.dkc_flags = DK_SILENT | DK_ISOLATE;
    dk_cmd.dkc_blkno = (daddr_t)0;
    dk_cmd.dkc_secnt = 1;
    dk_cmd.dkc_bufaddr = (char *) &dk_label;
    dk_cmd.dkc_buflen = SECSIZE;

    if (ioctl(d, DKIOCSCMD, &dk_cmd) < 0) {
	SImsg(SIM_DBG, "%s: DKIOCSCMD: %s.", file, SYSERR);
	return((DKlabel *) NULL);
    }

    if (dk_label.dkl_magic != DKL_MAGIC) {
	SImsg(SIM_GERR, "%s: Disk not labeled.", file);
	return((DKlabel *) NULL);
    }

    if (DkLblCheckSum(&dk_label)) {
	SImsg(SIM_GERR, "%s: Bad label checksum.", file);
	return((DKlabel *) NULL);
    }

    return(&dk_label);
}

/*
 * Get the name of a disk (i.e. sd0).
 */
static char *GetDiskName(name, dk_conf, dk_info)
    char 		       *name;
    DKconf 		       *dk_conf;
    DKinfo 		       *dk_info;
{
    if (!dk_conf || !dk_info) {
	if (name)
	    return(name);
	return((char *) NULL);
    }

#if	defined(DKI_HEXUNIT)
    if (FLAGS_ON(dk_info->dki_flags, DKI_HEXUNIT))
	(void) snprintf(Buf, sizeof(Buf), "%s%3.3x", 
			dk_conf->dkc_dname, dk_conf->dkc_unit);
    else
#endif 	/* DKI_HEXUNIT */
	(void) snprintf(Buf, sizeof(Buf), "%s%d", 
			dk_conf->dkc_dname, dk_conf->dkc_unit);

    return(strdup(Buf));
}

/*
 * Get the name of the controller for a disk.
 */
static char *GetDiskCtlrName(dk_conf)
    DKconf 		       *dk_conf;
{
    if (!dk_conf)
	return((char *) NULL);

    (void) snprintf(Buf, sizeof(Buf), "%s%d", 
		    dk_conf->dkc_cname, dk_conf->dkc_cnum);

    return(strdup(Buf));
}

/*
 * Get a disk controller device from disk info.
 */
static DevInfo_t *GetDiskCtlrDevice(DevData, dk_info, dk_conf)
    DevData_t 		       *DevData;
    DKinfo	 	       *dk_info;
    DKconf 		       *dk_conf;
{
    DevInfo_t 		       *MkMasterFromDevData();
    DevInfo_t 		       *DiskCtlr;

    /*
     * Get name of controller from devdata if available
     */
    if (DevData && DevData->CtlrName)
	DiskCtlr = MkMasterFromDevData(DevData);
    else {
	if ((DiskCtlr = NewDevInfo(NULL)) == NULL)
	    return((DevInfo_t *) NULL);
    }

    DiskCtlr->Type = DT_DISKCTLR;

    if (dk_conf) {
	if (!DiskCtlr->Name) {
	    DiskCtlr->Name = GetDiskCtlrName(dk_conf);
	    DiskCtlr->Unit = dk_conf->dkc_cnum;
	}
	DiskCtlr->Addr = dk_conf->dkc_addr;
	DiskCtlr->Prio = dk_conf->dkc_prio;
	DiskCtlr->Vec = dk_conf->dkc_vec;
    }

    if (dk_info)
	SetDiskCtlrModel(DiskCtlr, dk_info->dki_ctype);

    return(DiskCtlr);
}

/*
 * Get disk label info from the extracted dk_label info.
 */
static char *GetDiskLabel(dk_label)
    DKlabel 		       *dk_label;
{
    register char 	       *cp;
    char		       *label;

    if (!dk_label)
	return((char *) NULL);

    label = strdup(dk_label->dkl_asciilabel);

    /*
     * The label normally has geometry information in it we don't want
     * to see, so we trim out anything starting with " cyl".
     */
    for (cp = label; cp && *cp; ++cp)
	if (*cp == ' ' && strncasecmp(cp, " cyl", 4) == 0)
	    *cp = CNULL;

    return(strdup(label));
}

/*
 * Get MntEnt for PartInfo
 */
static struct mntent *GetMntEnt(PartInfo)
     PartInfo_t		       *PartInfo;
{
    static FILE 	       *mountedFP = NULL;
    static FILE 	       *mnttabFP = NULL;
    struct mntent 	       *mntent;

    if (!PartInfo)
      return NULL;

    /*
     * First try currently mounted filesystems (/etc/mtab)
     */
    if (!mountedFP) {
	if ((mountedFP = setmntent(MOUNTED, "r")) == NULL) {
	    SImsg(SIM_GERR, "%s: Cannot open for reading: %s.", 
		  MOUNTED, SYSERR);
	    return NULL;
	}
    } else
	rewind(mountedFP);

    while (mntent = getmntent(mountedFP))
	if (mntent->mnt_fsname && EQ(mntent->mnt_fsname, PartInfo->DevPath))
	    return mntent;

    /*
     * Now try static information (/etc/fstab)
     */
    if (!mnttabFP) {
	if ((mnttabFP = setmntent(MNTTAB, "r")) == NULL) {
	    SImsg(SIM_GERR, "%s: Cannot open for reading: %s.", 
		  MNTTAB, SYSERR);
	    return NULL;
	}
    } else
	rewind(mnttabFP);

    while (mntent = getmntent(mnttabFP))
	if (mntent->mnt_fsname && EQ(mntent->mnt_fsname, PartInfo->DevPath))
	    return mntent;

    return NULL;
}

/*
 * Get partition usage (what it's used for)
 */
static int GetPartUsage(PartInfo)
     PartInfo_t		       *PartInfo;
{
    struct mntent 	       *MntEnt;
    char		      **Argv;

    if (!PartInfo)
      return -1;

    /*
     * XXX Hard coded check for swap device.  Yuck!
     */
    if (EQ(PartInfo->Name, "sd0b")) {
	PartInfo->Usage = PIU_SWAP;
    } else if (MntEnt = GetMntEnt(PartInfo)) {
	PartInfo->MntName = strdup(MntEnt->mnt_dir);
	PartInfo->Type = strdup(MntEnt->mnt_type);	    
	if (EQ(PartInfo->Type, MNTTYPE_42)) 
	    PartInfo->Usage = PIU_FILESYS;
	else if (EQ(PartInfo->Type, MNTTYPE_SWAP)) 
	    PartInfo->Usage = PIU_SWAP;
	if (MntEnt->mnt_opts && !EQ(MntEnt->mnt_opts, "-"))
	    if (StrToArgv(MntEnt->mnt_opts, ",", &Argv, NULL, 0) > 0)
		PartInfo->MntOpts = Argv;
	return 0;
    }

    return -1;
}

/*
 * Get information about partition size
 */
static int GetPartSize(PartInfo, dk_geom)
    PartInfo_t		       *PartInfo;
    DKgeom 		       *dk_geom;
{
    struct dk_map 		dk_map;
    char 		       *p;
    int 			d;

    if (!PartInfo)
        return -1;

    if (stat(PartInfo->DevPathRaw, &StatBuf) != 0) {
	SImsg(SIM_DBG, "%s: No such partition.", PartInfo->DevPathRaw);
        return -1;
    }

    if ((d = open(PartInfo->DevPathRaw, O_RDONLY)) < 0) {
	SImsg(SIM_GERR, "%s: Cannot open for read: %s.", 
	      PartInfo->DevPathRaw, SYSERR);
        return -1;
    }

    if (ioctl(d, DKIOCGPART, &dk_map) != 0) {
	SImsg(SIM_GERR, "%s: Cannot extract partition info: %s.", 
		PartInfo->DevPathRaw, SYSERR);
        return -1;
    }
 
    (void) close(d);

    /*
     * Skip empty partitions
     */
    if (!dk_map.dkl_nblk) {
	SImsg(SIM_DBG, "%s: partition has no size.", PartInfo->DevPathRaw);
        return -1;
    }

    PartInfo->StartSect = (Large_t) (dk_map.dkl_cylno *
	(dk_geom->dkg_nhead * dk_geom->dkg_nsect));
    PartInfo->NumSect = (Large_t) dk_map.dkl_nblk;

    return 0;
}

/*
 * Get information about partition usage
 */
#include <sys/vfs.h>
static int GetPartAmtUsed(PartInfo)
    PartInfo_t		       *PartInfo;
{
    struct statfs		StatFs;

    if (!PartInfo || !PartInfo->MntName)
        return -1;

    if (statfs(PartInfo->MntName, &StatFs) != 0) {
	SImsg(SIM_DBG, "%s: statfs failed: %s.", 
	      PartInfo->MntName, SYSERR);
        return -1;
    }

    PartInfo->AmtUsed = (Large_t) ((StatFs.f_blocks - StatFs.f_bfree) * 
				   StatFs.f_bsize);

    return 0;
}

/*
 * Translate disk partition information from basic
 * extracted disk info.
 */
static DiskPart_t *GetDiskPart(name, dk_conf, dk_geom)
    char 		       *name;
    DKconf 		       *dk_conf;
    DKgeom 		       *dk_geom;
{
    extern char 		PartChars[];
    DiskPart_t		       *New;
    DiskPart_t 		       *Base = NULL;
    DiskPart_t 		       *Last = NULL;
    PartInfo_t		       *PartInfo = NULL;
    register int 		i;
    static char 		Path[MAXPATHLEN];

    if (!name || !dk_conf || !dk_geom)
	return((DiskPart_t *) NULL);

    for (i = 0; PartChars[i]; ++i) {
        PartInfo = PartInfoCreate(NULL);
	(void) snprintf(Path, sizeof(Path), "%s%c", name, PartChars[i]);
	PartInfo->Name = strdup(Path);
	(void) snprintf(Path, sizeof(Path), "/dev/%s", PartInfo->Name);
	PartInfo->DevPath = strdup(Path);
	(void) snprintf(Path, sizeof(Path), "/dev/r%s", PartInfo->Name);
	PartInfo->DevPathRaw = strdup(Path);
	PartInfo->Num = i;
	(void) GetPartSize(PartInfo, dk_geom);
	(void) GetPartUsage(PartInfo);
	/* GetPartAmtUsed must be called after GetPartUsage */
	(void) GetPartAmtUsed(PartInfo);

	New = NewDiskPart(NULL);
	New->PartInfo = PartInfo;
	if (Last) {
	  Last->Next = New;
	  Last = New;
	} else {
	  Base = Last = New;
	}
    }

    return(Base);
}

/*
 * Create a base DiskDrive device.
 */
static DevInfo_t *CreateBaseDiskDrive(ProbeData, DiskName)
     ProbeData_t	       *ProbeData;
     char		       *DiskName;
{
    DevInfo_t		       *DevInfo;
    DevData_t 		       *DevData;
    DevDefine_t	     	       *DevDefine;
    char		       *DevName;
    char		       *AltName = NULL;

    DevName = ProbeData->DevName;
    DevData = ProbeData->DevData;
    DevDefine = ProbeData->DevDefine;

    if (DiskName)
	ProbeData->DevName = DiskName;

    DevInfo = DeviceCreate(ProbeData);
    if (!DevInfo)
	return((DevInfo_t *) NULL);

    /*
     * See if there's a good alternative name we can set.
     */
    if (DevData)
	AltName = MkDevName(DevData->DevName, DevData->DevUnit,
			    DevDefine->Type, DevDefine->Flags);
    else if (!EQ(DiskName, DevName))
	AltName = DiskName;
    if (AltName && !EQ(DevInfo->Name, AltName))
	DevInfo->AltName = strdup(AltName);

    return(ProbeData->RetDevInfo = DevInfo);
}

/*
 * Convert all we've learned about a disk to a DevInfo_t.
 */
static DevInfo_t *CreateDiskDrive(ProbeData, DevName, DiskType, DevData,
				  dk_info, dk_label, dk_conf, dk_geom, dk_type)
    ProbeData_t		       *ProbeData;
    char 		       *DevName;
    int				DiskType;
    DevData_t 		       *DevData;
    DKinfo	 	       *dk_info;
    DKlabel	 	       *dk_label;
    DKconf 		       *dk_conf;
    DKgeom 		       *dk_geom;
    DKtype 		       *dk_type;
{
    DevInfo_t 		       *DevInfo;
    DevInfo_t 		       *DiskCtlr;
    DiskDriveData_t	       *DiskDriveData = NULL;
    DiskDrive_t		       *DiskDrive;
    int				GotScsi = FALSE;

    if (!ProbeData)
	return((DevInfo_t *) NULL);

    if (!(DevInfo = DeviceCreate(ProbeData))) {
	SImsg(SIM_GERR, "Cannot create new device entry.");
	return((DevInfo_t *) NULL);
    }

    if (ScsiQuery(DevInfo, ProbeData->DevFile, ProbeData->FileDesc, TRUE) == 0)
	GotScsi = TRUE;

    if (dk_label == NULL && DiskType != DKT_CDROM) {
	SImsg(SIM_GERR, "%s: No disk label found on disk.", DevName);
	if (!GotScsi)
	    return((DevInfo_t *) NULL);
    }

    if (DevInfo->DevSpec)
	DiskDriveData = (DiskDriveData_t *) DevInfo->DevSpec;
    else {
	if ((DiskDriveData = NewDiskDriveData(NULL)) == NULL) {
	    SImsg(SIM_GERR, "Cannot create new DiskDriveData entry.");
	    return((DevInfo_t *) NULL);
	}
	DevInfo->DevSpec = (void *) DiskDriveData;
    }

    if ((DiskDrive = NewDiskDrive(NULL)) == NULL) {
	SImsg(SIM_GERR, "Cannot create new DiskDrive entry.");
	return((DevInfo_t *) NULL);
    }
    /*
     * DiskDrive is the OS DiskDrive data
     */
    DiskDriveData->OSdata = DiskDrive;

    DevInfo->Name 		= GetDiskName(DevName, dk_conf, dk_info);
    DevInfo->Type 		= DT_DISKDRIVE;
    DiskDrive->DiskPart 	= GetDiskPart(DevName, dk_conf, dk_geom);
    DiskDrive->Label 		= GetDiskLabel(dk_label);
    if (!DevInfo->Model)
      DevInfo->Model 		= DiskDrive->Label;

    if (dk_conf) {
	DiskDrive->Unit 	= dk_conf->dkc_unit;
	DiskDrive->Slave 	= dk_conf->dkc_slave;;
    }
    if (dk_geom) {
	DiskDrive->DataCyl 	= dk_geom->dkg_ncyl;
	DiskDrive->PhyCyl 	= dk_geom->dkg_pcyl;
	DiskDrive->AltCyl 	= dk_geom->dkg_acyl;
	DiskDrive->Tracks 	= dk_geom->dkg_nhead;
	DiskDrive->Sect 	= dk_geom->dkg_nsect;
	DiskDrive->APC 		= dk_geom->dkg_apc;
	DiskDrive->RPM 		= dk_geom->dkg_rpm;
	DiskDrive->IntrLv 	= dk_geom->dkg_intrlv;
    }
    if (dk_type) {
	DiskDrive->PhySect 	= dk_type->dkt_hsect;
	DiskDrive->PROMRev 	= dk_type->dkt_promrev;
    }
    if (dk_info) {
#if	defined(DKI_HEXUNIT)
	if (FLAGS_ON(dk_info->dki_flags, DKI_HEXUNIT))
	    DiskDrive->Flags |= DF_HEXUNIT;
#endif 	/* DKI_HEXUNIT */
    }
    DiskDrive->SecSize 	= SECSIZE;

    if (DiskCtlr = GetDiskCtlrDevice(DevData, dk_info, dk_conf))
      DevInfo->Master 		= DiskCtlr;

    return(DevInfo);
}

/*
 * Query and learn about a disk.
 */
extern DevInfo_t *ProbeDiskDriveGeneric(ProbeData, DiskType)
     ProbeData_t	       *ProbeData;
     int			DiskType;
{
    DevInfo_t 		       *DevInfo;
    DKinfo 		       *dk_info = NULL;
    DKconf 		       *dk_conf = NULL;
    DKtype 		       *dk_type = NULL;
    DKlabel 		       *dk_label = NULL;
    DKgeom 		       *dk_geom = NULL;
    char 		       *rfile;
    int 			d;
    char 		       *DevName;
    DevData_t 		       *DevData;
    DevDefine_t	     	       *DevDefine;

    if (!ProbeData || !ProbeData->DevName) {
	SImsg(SIM_GERR, "ProbeDiskDriveGeneric: Missing parameters.");
	return((DevInfo_t *) NULL);
    }

    DevName = ProbeData->DevName;
    DevData = ProbeData->DevData;
    DevDefine = ProbeData->DevDefine;

#if	defined(HAVE_IPI)
    /*
     * XXX - Kludge for IPI "id" disks.
     */
    if (EQ(DevData->DevName, "id")) {
	static char		Buf[128];

	(void) snprintf(Buf, sizeof(Buf), "%s%3.3x", 
			DevData->DevName, DevData->DevUnit);
	DevName = Buf;
    }
#endif	/* HAVE_IPI */

    if (DiskType == DKT_CDROM)
	rfile = GetRawFile(DevName, NULL);
    else {
	if (stat(rfile = GetRawFile(DevName, NULL), &StatBuf) != 0)
	    /*
	     * Get the name of the whole disk raw device.
	     */
	    rfile = GetRawFile(DevName, "c");
    }
    if (!ProbeData->DevFile)
      ProbeData->DevFile = rfile;

    if ((d = open(rfile, O_RDONLY)) < 0) {
	SImsg(SIM_GERR, "%s: Cannot open for reading: %s.", rfile, SYSERR);
	/*
	 * If we know for sure this drive is present and we
	 * know something about it, then create a minimal device.
	 */
	if (errno == EBUSY || errno == EIO ||
	    ((DevDefine->Model || DevDefine->Desc) &&
	     FLAGS_ON(DevData->Flags, DD_IS_ALIVE))) {
	    DevInfo = CreateBaseDiskDrive(ProbeData, DevName);
	    return(DevInfo);
	} else {
	    SImsg(SIM_GERR, "%s: Not enough data to create disk device.",
		  DevName);
	    return((DevInfo_t *) NULL);
	}
    }

    if ((dk_conf = GETdk_conf(d, rfile, DiskType)) == NULL)
	SImsg(SIM_GERR, "%s: get dk_conf failed.", rfile);

    if ((dk_info = GETdk_info(d, rfile)) == NULL)
	SImsg(SIM_GERR, "%s: get dk_info failed.", rfile);

    if ((dk_geom = GETdk_geom(d, rfile, DiskType)) == NULL)
	SImsg(SIM_GERR, "%s: get dk_geom failed.", rfile);

    if ((dk_label = GETdk_label(d, rfile, dk_info, DiskType)) == NULL)
	SImsg(SIM_GERR, "%s: get dk_label failed.", rfile);

    /*
     * Not all controllers support dk_type
     */
    dk_type = GETdk_type(d, rfile);

    close(d);

    if (!(DevInfo = CreateDiskDrive(ProbeData,
				    DevName, DiskType, DevData,
				    dk_info, dk_label, 
				    dk_conf, dk_geom, dk_type))) {
	SImsg(SIM_GERR, "%s: Cannot convert DiskDrive information.", DevName);
	return((DevInfo_t *) NULL);
    }

    return(DevInfo);
}

/*
 * Probe normal disk drive by calling Generic probe routine.
 */
extern DevInfo_t *ProbeDiskDrive(ProbeData)
     ProbeData_t	       *ProbeData;
{
    return(ProbeDiskDriveGeneric(ProbeData, DKT_GENERIC));
}

/*
 * Probe CDROM disk drive by calling Generic probe routine.
 */
extern DevInfo_t *ProbeCDROMDrive(ProbeData)
     ProbeData_t	       *ProbeData;
{
    return(ProbeDiskDriveGeneric(ProbeData, DKT_CDROM));
}

/*
 * Create a tape device
 */
static DevInfo_t *CreateTapeDrive(ProbeData, DevName, TapeName)
     ProbeData_t	       *ProbeData;
     char		       *DevName;
     char		       *TapeName;
{
    DevInfo_t		       *DevInfo;

    ProbeData->DevName = DevName;
    DevInfo = DeviceCreate(ProbeData);
    if (TapeName && !EQ(DevName, TapeName))
	DevInfo->AltName = strdup(TapeName);
    DevInfo->Type 		= DT_TAPEDRIVE;

    return(DevInfo);
}

/*
 * Probe a tape device
 * XXX - this loses if somebody's using the tape, as tapes are exclusive-open
 * devices, and our open will therefore fail.
 * This also loses if there's no tape in the drive, as the open will fail.
 * The above probably applies to most other flavors of UNIX.
 */
extern DevInfo_t *ProbeTapeDrive(ProbeData)
     ProbeData_t	       *ProbeData;
{
    DevInfo_t 		       *DevInfo;
    char 		       *file;
    char			rfile[MAXPATHLEN];
    static char 		Buf[128];
    struct mtget 		mtget;
    register char	       *cp;
    int 			d;
    char 		       *DevName;
    DevData_t 		       *DevData;
    DevDefine_t	 	       *DevDefine;

    if (!ProbeData) {
	SImsg(SIM_DBG, "ProbeTapeDrive: Missing parameters.");
	return((DevInfo_t *) NULL);
    }

    DevName = ProbeData->DevName;
    DevData = ProbeData->DevData;
    DevDefine = ProbeData->DevDefine;

    SImsg(SIM_DBG, "ProbeTapeDrive(%s)", DevName);

    /*
     * Don't use GetRawFile; that'll just stick an "r" in front of the
     * device name, meaning it'll return the rewind-on-close device.
     * Somebody may have left the tape positioned somewhere other than
     * at the BOT to, for example, write a dump there later in the
     * evening; it'd be rather rude to reposition it out from under them.
     *
     * The above probably applies to most other flavors of UNIX.
     */
    if (!DevName)
	file = NULL;
    else {
	(void) snprintf(rfile, sizeof(rfile), "/dev/nr%s", DevName);
	file = rfile;
    }

    if ((d = open(file, O_RDONLY)) < 0) {
	SImsg(SIM_GERR, "%s Cannot open for read: %s.", file, SYSERR);

	/*
	 * --RECURSE--
	 * If we haven't tried the "mt" name yet, try it now
	 */
	if (strncmp(DevName, MTNAME, strlen(MTNAME)) != 0) {
	    (void) snprintf(Buf, sizeof(Buf), "%s%d", 
			    MTNAME, DevData->DevUnit);
	    ProbeData->DevName = Buf;
	    DevInfo = ProbeTapeDrive(ProbeData);
	    if (DevInfo)
		return(DevInfo);
	}

	/*
	 * If we know for sure this drive is present and we
	 * know something about it, then create a minimal device.
	 */
	if (FLAGS_ON(DevData->Flags, DD_IS_ALIVE)) {
	    DevInfo = CreateTapeDrive(ProbeData, DevName, NULL);
	    /* 
	     * Recreate name from devdata since we might have had to
	     * call ourself with name "rmt?"
	     */
	    (void) snprintf(Buf, sizeof(Buf), "%s%d", DevData->DevName, 
			   DevData->DevUnit);
	    DevInfo->Name = strdup(Buf);
	    return(DevInfo);
	} else {
	    SImsg(SIM_GERR, "%s: No data available to create tape device.",
		  DevName);
	    return((DevInfo_t *) NULL);
	}
    }

    if (ioctl(d, MTIOCGET, &mtget) != 0) {
	SImsg(SIM_GERR, "%s: Cannot extract tape status: %s.", file, SYSERR);
	return((DevInfo_t *) NULL);
    }

    (void) close(d);

    DevInfo = CreateTapeDrive(ProbeData, DevName, NULL);

    cp = GetTapeModel(mtget.mt_type);
    if (cp)
	DevInfo->Model = strdup(cp);
    else if (!DevInfo->Model)
	DevInfo->Model = "unknown";

    return(DevInfo);
}

/*
 * Query and learn about a device attached to an Auspex Storage Processor.
 * They'll show up as "ad" in the Mainbus info structure, but that
 * merely reflects the way the slots are set up in the config file.
 * We need to find out what type of device it is at this particular
 * instant (it's subject to change - perhaps even while we're running,
 * but there's not a heck of a lot we can do about that).
 *
 * We do that by trying it as a CD-ROM first, then as a disk, then as
 * a tape; that loses if it's a tape and somebody's using it, but
 * tapes on most if not all UNIX systems can't be probed by us if
 * somebody's using it.
 * The reason why we try it first as a CD-ROM is that if the CD has a
 * partition table, the Auspex driver lets you open the partitions as
 * if it were a disk.
 */
extern DevInfo_t *ProbeSPDrive(ProbeData)
     ProbeData_t	       *ProbeData;
{
    DevInfo_t 		       *thedevice;
    char			DevName[32];
    char 		       *name;
    DevData_t 		       *DevData;
    DevDefine_t	 	       *DevDefine;

    if (!ProbeData)
	return((DevInfo_t *) NULL);

    name = ProbeData->DevName;
    DevData = ProbeData->DevData;
    DevDefine = ProbeData->DevDefine;

    /*
     * Try it first as a CD-ROM.
     */
    (void) snprintf(DevName, sizeof(DevName), "acd%d", DevData->DevUnit);
    DevData->DevName = "acd";
    DevDefine->Model = "CD-ROM";
    ProbeData->DevName = DevName;
    if (thedevice = ProbeCDROMDrive(ProbeData))
	return(thedevice);

    /*
     * Not a CD-ROM.  Try a disk.
     */
    (void) snprintf(DevName, sizeof(DevName), "ad%d", DevData->DevUnit);
    DevData->DevName = "ad";
    DevDefine->Model = NULL;
    ProbeData->DevName = DevName;
    if (thedevice = ProbeDiskDrive(ProbeData))
	return(thedevice);

    /*
     * Not a disk.  Try a tape.
     */
    (void) snprintf(DevName, sizeof(DevName), "ast%d", DevData->DevUnit);
    DevData->DevName = "ast";
    ProbeData->DevName = DevName;
    if (thedevice = ProbeTapeDrive(ProbeData))
	return(thedevice);

    /*
     * None of the above.  Who knows?
     */
    return((DevInfo_t *) NULL);
}

#if	defined(HAVE_SUNROMVEC)
/*
 * Be backwards compatible with pre-4.1.2 code
 */
#include <mon/sunromvec.h>
#if	defined(OPENPROMS) && !(defined(ROMVEC_VERSION) && \
				(ROMVEC_VERSION == 0 || ROMVEC_VERSION == 1))
#define v_mon_id op_mon_id
#endif
#endif	/* HAVE_SUNROMVEC */

/*
 * Get ROM Version number
 *
 * If "romp" is "defined" (in <mon/sunromvec.h>), then take that
 * as the address of the kernel pointer to "rom" (struct sunromvec).
 * Otherwise, nlist "romp" from the kernel.
 */
extern char *GetRomVerSun()
{
    static char			RomRev[16];
    struct nlist	       *nlptr;
#if	defined(HAVE_SUNROMVEC)
    static struct sunromvec	Rom;
    kvm_t		       *kd;
#if	!defined(romp)
    struct sunromvec	       *romp;

    if (!(kd = KVMopen()))
	return((char *) NULL);

    if ((nlptr = KVMnlist(kd, RomVecSYM, (struct nlist *)NULL, 0)) == NULL)
	return((char *) NULL);

    if (CheckNlist(nlptr))
	return((char *) NULL);

    /*
     * Read the kernel pointer to the sunromvec structure.
     */
    if (KVMget(kd, (u_long) nlptr->n_value, (char *) &romp, 
	       sizeof(romp), KDT_DATA)) {
	SImsg(SIM_GERR, "Cannot read sunromvec pointer from kernel.");
	return((char *) NULL);
    }

#else	/* romp */

    if (!(kd = KVMopen()))
	return((char *) NULL);

#endif	/* romp */

    /*
     * Read the sunromvec structure from the kernel
     */
    /*SUPPRESS 25*/
    if (KVMget(kd, (u_long) romp, (char *) &Rom, 
	       sizeof(struct sunromvec), KDT_DATA)) {
	SImsg(SIM_GERR, "Cannot read sunromvec from kernel.");
	return((char *) NULL);
    }

#if	!defined(romp)

    /*
     * XXX Hardcoded values
     */
    (void) snprintf(RomRev, sizeof(RomRev),  "%d.%d", Rom.v_mon_id >> 16, Rom.v_mon_id & 0xFFFF);

#else	/* romp */

    /*
     * Read the version string from the address indicated by Rom.v_mon_id.
     */
    if (KVMget(kd, (u_long) Rom.v_mon_id, RomRev, 
	       sizeof(RomRev), KDT_STRING)) {
	SImsg(SIM_GERR, "Cannot read rom revision from kernel.");
	return((char *) NULL);
    }
#endif	/* romp */

    KVMclose(kd);

#endif	/* HAVE_SUNROMVEC */

    return((RomRev[0]) ? RomRev : (char *) NULL);
}
