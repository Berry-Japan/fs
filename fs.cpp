//---------------------------------------------------------
//	Filesystem Detection and Configuration
//
//		(C)2003 NAKADA
//---------------------------------------------------------

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <errno.h>
#include "fs.h"


//---------------------------------------------------------
//	Global Variables
//---------------------------------------------------------

int flag;		// option flag
int devc;		// device count
//char dev[120];		// device name
//char part[120];		// partition name


//---------------------------------------------------------
//	Define llseek
//---------------------------------------------------------

_syscall5(int, _llseek, unsigned int, fd,
          unsigned long, offset_high, unsigned long, offset_low,
          long long *, result, unsigned int, origin)


//---------------------------------------------------------
//	Seek Sector
//---------------------------------------------------------

void seek_sector(int fd, uint sec)
{
	long long r;
	unsigned long long offset = (unsigned long long)sec * 512;

	_llseek(fd, offset>>32, offset&0xffffffff, &r, SEEK_SET);
	if (r==-1) {
		printf("\nError seeking drive\n");
		exit(1);
	}

	return;
}


//---------------------------------------------------------
//	Read Sector
//---------------------------------------------------------

int read_sector(char *device, uint sec, void *buff)
{
	int fd;
	int status;

	fd = open(device, O_RDONLY);
	if (fd==-1) {
		printf("\nError opening drive %s\n", device);
		exit(1);
	}
	seek_sector(fd, sec);
	status = read(fd, buff, 512);

	return status;
}


//---------------------------------------------------------
//	Read Partition Table
//---------------------------------------------------------

int read_partition_table(char *device, uint sec, partition_table *pt)
{
	int status;
	char buff[512];

	status=read_sector(device, sec, buff);
	memmove(pt, (buff + 0x1be), sizeof(partition_table));

	return status;
}


//---------------------------------------------------------
//	Read Partition Table
//---------------------------------------------------------

/*int read_partition_table(char *drive, partition_table *pt)
{
	byte buffer[512];
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


//---------------------------------------------------------
//	Write Partition Table
//---------------------------------------------------------

int write_partition_table(partition_table partn, char *drive)
{
	byte buffer[512];
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
}*/


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
//	Check /etc/fstab
//---------------------------------------------------------

int check_fstab(char *device)
{
	FILE *fp;
	int m, n;
	char line[256], dev[120], mnt[120], fs[20], opt[100];

	fp = fopen("/etc/fstab", "r");
	if (!fp) {
		fprintf(stderr, "Can't open %s\n", "/etc/fstab");
		return 1;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "%s %s %s %s %d %d", dev, mnt, fs, opt, &m, &n) != 6) continue;
		if (!strcmp(dev, device)) return 0;
	}
	fclose(fp);

	return 1;
}


//---------------------------------------------------------
//	Displays Partition Table
//---------------------------------------------------------

void show_partition_table(char *device, partition_table pt, int flag)
{
	int x;
	char buff[256];

	if (flag) {
		for (x = 0; x < 4; x++) {
			devc++;

			snprintf(buff, sizeof(buff), "%s%d", device, devc);
			if (!check_fstab(buff)) continue;

			switch (pt.entry[x].id) {
			case NTFS:		// 0x07
				printf("%s%d\t/mnt/hd%d\tntfs\tusers,ro\t0 0\n", device, devc, devc);
				break;

			case Win98_FAT32:	// 0x0b
			case Win98_FAT32_LBA:	// 0x0c
				printf("%s%d\t/mnt/hd%d\tvfat\tusers,codepage=932,iocharset=euc-jp\t0 0\n", device, devc, devc);
				break;

			case LINUX_PARTITION:	// 0x81
				printf("%s%d\t/mnt/hd%d\text2\tusers\t0 0\n", device, devc, devc);
				break;

			case LINUX_SWAP:	// 0x82
				printf("%s%d\tswap\tswap\tdefaults\t0 0\n", device, devc);
				break;

			case LINUX_NATIVE:	// 0x83
				printf("%s%d\t/mnt/hd%d\text3\tusers\t0 0\n", device, devc, devc);
				break;
			}
			//printf("%x\n",pt.entry[x].id);
		}
	} else {
		printf("\nActive\tSystem Type\t\t\t    Start  Size (KB)\n");
		for (x = 0; x < 4; x++) {
			printf("  %-3s", pt.entry[x].boot_flag ? "Yes" : "No");
			printf("\t%-30s", get_system_type(pt.entry[x].id));
			printf(" %10ld", pt.entry[x].lba_start);
			printf(" %10ld\n", pt.entry[x].lba_size / 2);
		}
	}

	return;
}


//---------------------------------------------------------
//	Patition
//---------------------------------------------------------

void partition(char *device, char *name, uint sec, uint m, int flag)
{
	int x;
	partition_table pt;

	// infomation
	read_partition_table(device, sec, &pt);
	show_partition_table(name, pt, flag);

	// for extended partition
	for (x=0; x<4; x++) {
		if (IS_EXTENDED(pt.entry[x].id)) {
			partition(device, name, pt.entry[x].lba_start + m, m==0 ? pt.entry[x].lba_start : m, flag);
		}
	}

	return;
}


//---------------------------------------------------------
//	Is it cdrom ? (for no devfs)
//---------------------------------------------------------

int is_ide_cdrom_or_tape(char *device)
{
	FILE *fp;
	int is_ide;
	char buff[100];
	struct stat statbuf;

	/* is it CDROM ? */
	is_ide=0;
	snprintf(buff, sizeof(buff), "/proc/ide/%s/media", device);

	fp = fopen(buff, "r");
	if (fp != NULL && fgets(buff, sizeof(buff), fp)) {
		is_ide=(!strncmp(buff, "cdrom", 5) || !strncmp(buff, "tape", 4));
	} else {
		/* Now when this proc file does not exist, skip the device when it is read-only. */
		if (stat(device, &statbuf) == 0) is_ide = ((statbuf.st_mode & 0222) == 0);
	}
	if (fp) fclose(fp);

	return is_ide;
}


//---------------------------------------------------------
//	main
//---------------------------------------------------------

int main(int argc, char *argv[])
{
	FILE *fp;
	char line[100], name[100], device[120], *s;
	int ma, mi, sz;

	int i;
	glob_t globres;
	struct stat statbuf;

	// Check options
	//flag=0;
	//if (argc>1 && !strcmp(argv[1],"-c")) flag=1;
	flag=(argc>1 && !strcmp(argv[1],"-c"));

	if (stat("/dev/.devfsd", &statbuf)) {
	//if (1) {
		// Use /proc/partitions
		fp = fopen(PROC_PARTITIONS, "r");
		if (!fp) {
			fprintf(stderr, "Can't open %s\n", PROC_PARTITIONS);
			return 1;
		}

		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, " %d %d %d %[^\n ]", &ma, &mi, &sz, name) != 4) continue;
			for (s = name; *s; s++) ;
			if (isdigit(s[-1])) continue;
			snprintf(device, sizeof(device), "/dev/%s", name);

			if (!is_ide_cdrom_or_tape(name)) {
				devc=0;
				partition(device, device, 0, 0, flag);
			}
		}
		fclose(fp);
	} else {
		// Use devfs
		if (!glob("/dev/discs/disc?*", 0, NULL, &globres)) {
			for (i=0; i<(int)globres.gl_pathc; i++) {
				//printf("%s\n", globres.gl_pathv[i]);
				devc=0;
				snprintf(device, sizeof(device), "%s/disc", globres.gl_pathv[i]);
				snprintf(name, sizeof(name), "%s/part", globres.gl_pathv[i]);
				partition(device, name, 0, 0, flag);
			}
		}
		globfree(&globres);
	}

	return 0;
}
