//---------------------------------------------------------
//	Hardware Detection and Configuration
//
//		(C)2003 NAKADA
//---------------------------------------------------------


#include "fs.h"

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



/*
 *  Reads in the first 512 bytes of the given device into the
 *  buffer (global variable) and extracts partition table info from the
 *  sector (partn tbl starts at 0x1BE, 4 entries of 0x10 bytes plus a 2
 *  byte signature at end. Returns 0 if read OK, non-zero otherwise.
 *
 */

int read_partition_table(partition_table *pt, char *drive)
{
	int status;
	FILE *dr;

	dr = fopen(drive, "rb");
	if (dr == NULL) {
		printf("\nerror opening drive %s", drive);
		exit(1);
	}
	status = fread(buffer, sizeof(byte), 512, dr);
	memmove(pt, (buffer + 0x1BE), sizeof(partition_table));

	return status;
}


/*
 *  Does the reverse of read_partition_table() - writes a new partition
 *  table into the relevent bit of buffer (the last 74 bytes) and writes
 *  the whole 512 bytes to the 1st sector of the specified Hard Drive
 *
 */

int write_partition_table(partition_table partn, char *drive)
{
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


/*
 * Determines how many Hard drives the BIOS is aware of by trying to read
 * the Master Boot sector of each Hard drive starting from 0x80 (1st disk)
 * contines to read drives (0x81, 0x82, etc) until it receives an error,
 * after which it assumes that all physical drives have been read.
 *
 * Returns number of drives which it has detected.
 *
 */

int determine_number_of_drives()
{
  /* I don't know how to implement the proper calls (yet!) (Turbo C
     makes things so easy! <g>), so the quick and dirty way of doing
     this is just to return a predefined constant */

  return(NUM_DRIVES);
}



int display = 0, activate = 0, deactivate = 0;
byte buffer[512];                       /* Store Master Boot Sector here */


/*
 * Calculates the Start and End boundaries for the given partn table entry.
 * The 2 high-order bits of the Cylinder are stored in the 2 high order bits
 * of start_sector / end_sector, this these sector values are only 6 bits
 *
 */

void get_partition_boundaries(partition_table_entry entry, location *Start, location *End)
{
  Start->Side = entry.start_sector_head;
  Start->Sector = entry.start_sector & 0x3F;   /* 2 high bits are for cyl */
  Start->Cylinder = ((entry.start_sector & 0xC0) << 2)+(entry.start_cylinder);

  End->Side = entry.end_sector_head;
  End->Sector = entry.end_sector & 0x3F;
  End->Cylinder = ((entry.end_sector & 0xC0) << 2) + (entry.end_cylinder);
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
/*	location Start, End;

	printf("\nPartition Table for Hard Drive %s:", drive_name[drive]);

	printf("\n\t\t\t\t\t\t    Starting       Ending");
	printf("\nActive\t\tSystem Type\t    Size (KB)    Side Cyl Sect  Side Cyl Sect\n");

	for (x = 0; x < 4; x++) {
		get_partition_boundaries(pt.entry[x], &Start, &End);
		printf("  %-3s", pt.entry[x].boot_flag ? "Yes" : "No");
		printf("\t%-30s", get_system_type(pt.entry[x].id));
		printf("%6ld", pt.entry[x].lba_size / 2);
		printf("%8d %4d %3d", Start.Side, Start.Cylinder, Start.Sector);
		printf("%6d %4d %3d\n", End.Side, End.Cylinder, End.Sector);
	}*/

	printf("\nActive\tSystem Type\t\t\t    Start  Size (KB)\n");
	for (x = 0; x < 4; x++) {
		printf("  %-3s", pt.entry[x].boot_flag ? "Yes" : "No");
		printf("\t%-30s", get_system_type(pt.entry[x].id));
		printf(" %10ld", pt.entry[x].lba_start);
		printf(" %10ld\n", pt.entry[x].lba_size / 2);
	}

	return;
}


/*
 * Makes the given partition the booting partition - done simply by setting
 * the boot_flag = 0x80.
 *
 */

void activate_partition(partition_table_entry *partition)
{
   partition->boot_flag = 0x80;
}


/*
 * Resets the given partition's boot_flag, thus making it inactive
 *
 */

void deactivate_partition(partition_table_entry *partition)
{
  partition->boot_flag = 0x00;
}


/*
 *  Displays an error message and gives Usage information
 *
 */

void Usage(char *message, char *path, int errorlevel)
{
  printf("\nActivate v1.01 Hard Disk Partition selector (Ronan Mullally, 1992)\n");
  if(message != "") printf("\n%s", message);

  printf("\n\n%cUsage:  %s [-d] [-l] [partition #]\n", 7, path);
  printf("\t-d\t   : Deactivate given partition\n");
  printf("\t-l\t   : Display Partition Table(s)\n");
  printf("\tpartition #: Number of Partition to activate\n\n");
  printf(" Eg: %s 1     Activates 1st partition on Drive 0\n", path);
  printf("     %s -d 6  Deactivates 2nd partition on Drive 1\n\n", path);

  exit(errorlevel);
}


/*
 * Checks the command line for invalid parameters, processes valid options
 * and extracts a partition number (if given).
 *
 * Returns partition number given on command line, or aborts if invalid
 * options were given
 *
 */

int check_command_line(int argc, char *argv[])
{
  int x, y = 0, partition_number = 0, error = 0;
  char *buffer[40];

  for(x = 1; x < argc && !error; x++)
  {
    for(y = 0; !error && argv[x][y] != '\0'; y++)
      if(!isdigit(argv[x][y]))
        error = 1;

    if(!error)
    {
      partition_number = atoi(argv[x]);
      activate = !deactivate;
    }
    else if(!strcmp(argv[x], "-l"))
    {
      display = 1;
      error = 0;
      continue;
    }
    else if(!strcmp(argv[x], "-d"))
    {
      deactivate = 1;
      activate = 0;
      error = 0;
      continue;
    }
    else
    {
      error = 1;
      break;
    }
  }

  if(error)
  {
    sprintf((char *) buffer, "Invalid command line argument: %s", argv[x]);
    Usage((char*) buffer, argv[0], BAD_COMMAND_LINE);
  }
  return(partition_number);
}


//---------------------------------------------------------
//	main
//---------------------------------------------------------

int main(int argc, char *argv[])
{
	char *drive;	/* for *nix they are filenames */

	int max_partition, partition_to_activate = 0, x, y, drive_count;
	partition_table partn;

	if (argc > 4) Usage("Too many parameters on command line", argv[0], BAD_COMMAND_LINE);
	else if (argc == 1) display = 1;	/* no options given, just list */
	else partition_to_activate = check_command_line(argc, argv);

//	drive_count = determine_number_of_drives();
	drive_count = 1;
	max_partition = drive_count * 4;	/* 4 partns per drive */

	if (display) printf("\n\n%d Hard Drive%s found", drive_count, drive_count > 1 ? "s" : "");

	if ((!partition_to_activate && !display) || (partition_to_activate > max_partition)) {
		Usage("Invalid partition number", argv[0], BAD_PARTITION_NUMBER);
	}

	for (x = 0; x < drive_count; x++) {
		drive = drive_name[x];

		read_partition_table(&partn, drive);

		if ((partition_to_activate-1) / 4 == x)  {
			/* partition on this drive ? */
			if (activate) {
				/* Activate given partition */
				for (y = 0; y < 4; y++) deactivate_partition(&partn.entry[y]);   /* Deactive all first */
				activate_partition(&partn.entry[partition_to_activate - (4*x)-1 % 4]);
				write_partition_table(partn, drive);
			} else if (deactivate) {
				/* Deactivate given partition */
				deactivate_partition(&partn.entry[partition_to_activate-(4*x)-1 % 4]);
				write_partition_table(partn, drive);
			}
		}
		if (display) show_partition_table(partn, x);
	}

	return 0;
}
