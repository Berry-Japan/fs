//---------------------------------------------------------
//	Hardware Detection and Configuration
//		gcc -Wall -lstdc++ fs.cpp -o fs
//
//		(C)2003 NAKADA
//---------------------------------------------------------

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//#include "fs.h"

#define EXTENDED        0x05
#define WIN98_EXTENDED  0x0f
#define LINUX_PARTITION 0x81
#define LINUX_SWAP      0x82
#define LINUX_NATIVE    0x83
#define LINUX_EXTENDED  0x85
#define LINUX_LVM       0x8e
#define LINUX_RAID      0xfd

#define IS_EXTENDED(i) \
	((i) == EXTENDED || (i) == WIN98_EXTENDED || (i) == LINUX_EXTENDED)


typedef unsigned char byte;

struct partition_table_entry {
	byte boot_flag;			/* active partition ? */
	byte start_sector_head;
	byte start_sector;
	byte start_cylinder;
	byte id;			/* partition type */
	byte end_sector_head;
	byte end_sector;
	byte end_cylinder;
	long lba_start;			/* starting sector counting from 0 */
	long lba_size;			/* nr of sectors in partition */
};

struct partition_table {
	partition_table_entry entry[4];
	byte signature[2];		/* Should be 55 AA for valid disk */
};

struct systypes {
	byte type;
	char *name;
};


struct systypes i386_sys_types[] = {
	{0x00, "Empty"},
	{0x01, "FAT12"},
	{0x02, "XENIX root"},
	{0x03, "XENIX usr"},
	{0x04, "FAT16 <32M"},
	{0x05, "Extended"},			/* DOS 3.3+ extended partition */
	{0x06, "FAT16"},			/* DOS 16-bit >=32M */
	{0x07, "HPFS/NTFS"},			/* OS/2 IFS, eg, HPFS or NTFS or QNX */
	{0x08, "AIX"},				/* AIX boot (AIX -- PS/2 port) or SplitDrive */
	{0x09, "AIX bootable"},			/* AIX data or Coherent */
	{0x0a, "OS/2 Boot Manager"},		/* OS/2 Boot Manager */
	{0x0b, "Win95 FAT32"},
	{0x0c, "Win95 FAT32 (LBA)"},		/* LBA really is `Extended Int 13h' */
	{0x0e, "Win95 FAT16 (LBA)"},
	{0x0f, "Win95 Ext'd (LBA)"},
	{0x10, "OPUS"},
	{0x11, "Hidden FAT12"},
	{0x12, "Compaq diagnostics"},
	{0x14, "Hidden FAT16 <32M"},
	{0x16, "Hidden FAT16"},
	{0x17, "Hidden HPFS/NTFS"},
	{0x18, "AST SmartSleep"},
	{0x1b, "Hidden Win95 FAT32"},
	{0x1c, "Hidden Win95 FAT32 (LBA)"},
	{0x1e, "Hidden Win95 FAT16 (LBA)"},
	{0x24, "NEC DOS"},
	{0x39, "Plan 9"},
	{0x3c, "PartitionMagic recovery"},
	{0x40, "Venix 80286"},
	{0x41, "PPC PReP Boot"},
	{0x42, "SFS"},
	{0x4d, "QNX4.x"},
	{0x4e, "QNX4.x 2nd part"},
	{0x4f, "QNX4.x 3rd part"},
	{0x50, "OnTrack DM"},
	{0x51, "OnTrack DM6 Aux1"},		/* (or Novell) */
	{0x52, "CP/M"},				/* CP/M or Microport SysV/AT */
	{0x53, "OnTrack DM6 Aux3"},
	{0x54, "OnTrackDM6"},
	{0x55, "EZ-Drive"},
	{0x56, "Golden Bow"},
	{0x5c, "Priam Edisk"},
	{0x61, "SpeedStor"},
	{0x63, "GNU HURD or SysV"},		/* GNU HURD or Mach or Sys V/386 (such as ISC UNIX) */
	{0x64, "Novell Netware 286"},
	{0x65, "Novell Netware 386"},
	{0x70, "DiskSecure Multi-Boot"},
	{0x75, "PC/IX"},
	{0x80, "Old Minix"},			/* Minix 1.4a and earlier */
	{0x81, "Minix / old Linux"},		/* Minix 1.4b and later */
	{0x82, "Linux swap"},			/* also Solaris */
	{0x83, "Linux"},
	{0x84, "OS/2 hidden C: drive"},
	{0x85, "Linux extended"},
	{0x86, "NTFS volume set"},
	{0x87, "NTFS volume set"},
	{0x8e, "Linux LVM"},
	{0x93, "Amoeba"},
	{0x94, "Amoeba BBT"},			/* (bad block table) */
	{0x9f, "BSD/OS"},			/* BSDI */
	{0xa0, "IBM Thinkpad hibernation"},
	{0xa5, "FreeBSD"},			/* various BSD flavours */
	{0xa6, "OpenBSD"},
	{0xa7, "NeXTSTEP"},
	{0xa8, "Darwin UFS"},
	{0xa9, "NetBSD"},
	{0xab, "Darwin boot"},
	{0xb7, "BSDI fs"},
	{0xb8, "BSDI swap"},
	{0xbb, "Boot Wizard hidden"},
	{0xbe, "Solaris boot"},
	{0xc1, "DRDOS/sec (FAT-12)"},
	{0xc4, "DRDOS/sec (FAT-16 < 32M)"},
	{0xc6, "DRDOS/sec (FAT-16)"},
	{0xc7, "Syrinx"},
	{0xda, "Non-FS data"},
	{0xdb, "CP/M / CTOS / ..."},		/* CP/M or Concurrent CP/M or Concurrent DOS or CTOS */
	{0xde, "Dell Utility"},			/* Dell PowerEdge Server utilities */
	{0xdf, "BootIt"},			/* BootIt EMBRM */
	{0xe1, "DOS access"},			/* DOS access or SpeedStor 12-bit FAT extended partition */
	{0xe3, "DOS R/O"},			/* DOS R/O or SpeedStor */
	{0xe4, "SpeedStor"},			/* SpeedStor 16-bit FAT extended partition < 1024 cyl. */
	{0xeb, "BeOS fs"},
	{0xee, "EFI GPT"},			/* Intel EFI GUID Partition Table */
	{0xef, "EFI (FAT-12/16/32)"},		/* Intel EFI System Partition */
	{0xf0, "Linux/PA-RISC boot"},		/* Linux/PA-RISC boot loader */
	{0xf1, "SpeedStor"},
	{0xf4, "SpeedStor"},			/* SpeedStor large partition */
	{0xf2, "DOS secondary"},		/* DOS 3.3+ secondary */
	{0xfd, "Linux raid autodetect"},	/* New (2.2.x) raid partition with autodetect using persistent superblock */
	{0xfe, "LANstep"},			/* SpeedStor >1024 cyl. or LANstep */
	{0xff, "BBT"},				/* Xenix Bad Block Table */
	{ 0, 0 }
};


#define PROC_PARTITIONS		"/proc/partitions"
char drive_name[100][100];



int read_partition_table(partition_table *pt, char *drive)
{
	byte buffer[512];                       /* Store Master Boot Sector here */
	int status;
	FILE *dr;

	dr = fopen(drive, "rb");
	if (dr == NULL) {
		printf("\nError opening drive %s\n", drive);
		exit(1);
	}
	status = fread(buffer, sizeof(byte), 512, dr);
	memmove(pt, (buffer + 0x1BE), sizeof(partition_table));

	return status;
}


int write_partition_table(partition_table partn, char *drive)
{
	byte buffer[512];                       /* Store Master Boot Sector here */
	int status;
	FILE *dr;

	memmove((buffer + 0x1BE), &partn, sizeof(partition_table));

	dr = fopen(drive, "wb");
	if (dr == NULL) {
		printf("\nError opening drive %s for writing\n", drive);
		exit(1);
	}
	status = fwrite(buffer, sizeof(byte), 512, dr);

	return status;
}



//---------------------------------------------------------
//	Get File System Type
//---------------------------------------------------------

char *get_system_type(byte type)
{
	int x;
//	systypes *types = get_sys_types();
	systypes *types = i386_sys_types;

	for (x=0; types[x].name; x++)
		if (types[x].type == type)
			return types[x].name;

	return "Unkown !!";
}


//---------------------------------------------------------
//	Displays Partition Table
//---------------------------------------------------------

void show_partition_table(partition_table pt, int drive)
{
	int x;

	printf("\nActive\tSystem Type\t\t\t    Start  Size (KB)\n");
	for (x = 0; x < 4; x++) {
		printf("  %-3s", pt.entry[x].boot_flag ? "Yes" : "No");
		printf("\t%-30s", get_system_type(pt.entry[x].id));
		printf(" %10ld", pt.entry[x].lba_start);
		printf(" %10ld\n", pt.entry[x].lba_size / 2);
	}

	return;
}

#include <sys/stat.h>


//---------------------------------------------------------
//	Get Partition Name
//---------------------------------------------------------

int determine_number_of_drives()
{
	FILE *fp, *fp2;
	char line[100], ptname[100], device[120], *s;
	char buff[100];
	int ma, mi, sz;
	int is_ide, n;
	struct stat statbuf;

	fp = fopen(PROC_PARTITIONS, "r");
	if (!fp) {
		fprintf(stderr, "Can't open %s\n", PROC_PARTITIONS);
		return 0;
	}

	n=0;
	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, " %d %d %d %[^\n ]", &ma, &mi, &sz, ptname) != 4) continue;
		for (s = ptname; *s; s++) ;
		if (isdigit(s[-1])) continue;
		snprintf(device, sizeof(device), "/dev/%s", ptname);

		/* is it CDROM ? */
		snprintf(buff, sizeof(buff), "/proc/ide/%s/media", ptname);
		printf("%s\n", buff);
		fp2 = fopen(buff, "r");
		if (fp2 != NULL && fgets(buff, sizeof(buff), fp2)) {
			is_ide=(!strncmp(buff, "cdrom", 5) || !strncmp(buff, "tape", 4));
		} else {
			/* Now when this proc file does not exist, skip the device when it is read-only. */
			if (stat(device, &statbuf) == 0) is_ide = ((statbuf.st_mode & 0222) == 0);
		}
		if (fp2) fclose(fp2);
		if (!is_ide) strcpy(drive_name[n++], device);
	}
	fclose(fp);

	return n;
}


//---------------------------------------------------------
//	main
//---------------------------------------------------------

int main(int argc, char *argv[])
{
	int x,drive_count;
	partition_table pt;

	drive_count = determine_number_of_drives();

	for (x = 0; x < drive_count; x++) {
		read_partition_table(&pt, drive_name[x]);
		show_partition_table(pt, x);
	}

	return 0;
}
