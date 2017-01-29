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
int partc;		// partition count
char dev[120];		// device name
char part[120];		// partition name


//---------------------------------------------------------
//	Define llseek (for Linux)
//---------------------------------------------------------

_syscall5(int, _llseek, unsigned int, fd,
          unsigned long, offset_high, unsigned long, offset_low,
          long long *, result, unsigned int, origin)


//---------------------------------------------------------
//	Seek Sector (for Linux)
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
//	Read Sector (for Linux)
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


/*//---------------------------------------------------------
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
//	Check /etc/fstab (for Linux)
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

void show_partition_table(char *device, partition_table pt, int flag, int ex)
{
	int x;
	char buff[256];

	if (flag) {
		// extened ?
		if (ex) ex = 1;
		else ex = 4;

		for (x=0; x<ex; x++) {
			partc++;

			// check fstab
			snprintf(buff, sizeof(buff), "%s%d", device, partc);
			if (!(flag&2) && !check_fstab(buff)) continue;

			snprintf(buff, sizeof(buff), "%s%d\t/mnt/hd%c%d", device, partc, 'a'+devc, partc);

			// set
			switch (pt.entry[x].id) {
			case NTFS:			// 0x07
				printf("%s\tntfs\tusers,iocharset=euc-jp,uid=1000\t0 0\n", buff);
				break;

			case Win98_FAT32:		// 0x0b
			case Win98_FAT32_LBA:		// 0x0c
			case Win98_HIDDEN_FAT32:	// 0x1b
			case Win98_HIDDEN_FAT32_LBA:	// 0x1c
				printf("%s\tvfat\tusers,codepage=932,iocharset=euc-jp\t0 0\n", buff);
				break;

			case LINUX_PARTITION:		// 0x81
				printf("%s\text2\tusers\t0 0\n", buff);
				break;

			case LINUX_SWAP:		// 0x82
				printf("%s%d\tswap\t\tswap\tdefaults\t0 0\n", device, partc);
				break;

			case LINUX_NATIVE:		// 0x83
				printf("%s\text3\tusers\t0 0\n", buff);
			}
			//printf("%x\n",pt.entry[x].id);
		}
	} else {
		printf("\nDevice %s\n", dev);		// Bad Code !!
		printf("Active\tSystem Type\t\t\t    Start  Size (KB)\n");
		for (x=0; x<4; x++) {
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

void partition(uint sec, uint m)
{
	int x;
	partition_table pt;

	// infomation
	read_partition_table(dev, sec, &pt);
	show_partition_table(part, pt, flag, sec!=0);

	// for extended partition
	for (x=0; x<4; x++) {
		if (IS_EXTENDED(pt.entry[x].id)) {
			partition(pt.entry[x].lba_start + m, m==0 ? pt.entry[x].lba_start : m);
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
	char line[100], name[100], *s;
	int ma, mi, sz;

	int i;
	glob_t globres;
	struct stat statbuf;

	// Check options
	flag=0;
	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-c")) flag|=1;
		else if(!strcmp(argv[i], "-a")) flag|=2;
		else if(!strcmp(argv[i], "-v")) flag|=4;
		//else return syntax(argv[i]);
	}

	if (flag&4) printf("Scanning for new harddiscs/partitions...\n");

	// Check using devfs
	if (stat("/dev/.devfsd", &statbuf)) {
	//if (1) {
		// Use /proc/partitions
		fp = fopen(PROC_PARTITIONS, "r");
		if (!fp) {
			fprintf(stderr, "Can't open %s\n", PROC_PARTITIONS);
			return 1;
		}

		devc=0;
		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, " %d %d %d %[^\n ]", &ma, &mi, &sz, name) != 4) continue;
			for (s = name; *s; s++) ;
			if (isdigit(s[-1])) continue;

			partc=0;
			snprintf(dev, sizeof(dev), "/dev/%s", name);
			snprintf(part, sizeof(part), "/dev/%s", name);

			if (!is_ide_cdrom_or_tape(name)) {
				partition(0, 0);
			}
			devc++;
		}
		fclose(fp);
	} else {
		// Use devfs
		if (!glob("/dev/discs/disc?*", 0, NULL, &globres)) {
			for (devc=0; devc<(int)globres.gl_pathc; devc++) {
				//printf("%s\n", globres.gl_pathv[i]);
				partc=0;
				snprintf(dev, sizeof(dev), "%s/disc", globres.gl_pathv[devc]);
				snprintf(part, sizeof(part), "%s/part", globres.gl_pathv[devc]);
				partition(0, 0);
			}
		}
		globfree(&globres);
	}

	return 0;
}
