/*
 * commandOne.c
 *
 *  Created on: Mar 20, 2018
 *      Author: jburleso
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>

// main bus; scratch RAM
// used only for testing
#define FPGA_ONCHIP_BASE      0xC0000000
#define FPGA_ONCHIP_SPAN      262144

// main bus; FIFO write address
#define FIFO_BASE            0xC0040000
#define FIFO_SPAN            4
// the read and write ports for the FIFOs
// you need to query the status ports before these operations
// PUSH the write FIFO
// POP the read FIFO
#define FIFO_WRITE		     (*(FIFO_write_ptr))
#define FIFO_READ            (*(FIFO_read_ptr))

/// lw_bus; FIFO status address
#define FIFO_CSRREGS_BASE          0xff200000
#define FIFO_CSRREGS_SPAN          32

// WAIT looks nicer than just braces
#define WAIT {}
// FIFO status registers
// base address is current fifo fill-level
// base+1 address is status:
// --bit0 signals "full"
// --bit1 signals "empty"
#define WRITE_FIFO_FILL_LEVEL (*FIFO_write_status_ptr)
#define READ_FIFO_FILL_LEVEL  (*FIFO_read_status_ptr)
#define WRITE_FIFO_FULL		  ((*(FIFO_write_status_ptr+1))& 1 )
#define WRITE_FIFO_EMPTY	  ((*(FIFO_write_status_ptr+1))& 2 )
#define READ_FIFO_FULL		  ((*(FIFO_read_status_ptr+1)) & 1 )
#define READ_FIFO_EMPTY	      ((*(FIFO_read_status_ptr+1)) & 2 )
// arg a is data to be written
#define FIFO_WRITE_BLOCK(a)	  {while (WRITE_FIFO_FULL){WAIT};FIFO_WRITE=a;}
// arg a is data to be written, arg b is success/fail of write: b==1 means success
#define FIFO_WRITE_NOBLOCK(a,b) {b=!WRITE_FIFO_FULL; if(!WRITE_FIFO_FULL)FIFO_WRITE=a; }
// arg a is data read
#define FIFO_READ_BLOCK(a)	  {while (READ_FIFO_EMPTY){WAIT};a=FIFO_READ;}
// arg a is data read, arg b is success/fail of read: b==1 means success
#define FIFO_READ_NOBLOCK(a,b) {b=!READ_FIFO_EMPTY; if(!READ_FIFO_EMPTY)a=FIFO_READ;}


// the light weight buss base
void *h2p_lw_virtual_base;
// HPS_to_FPGA FIFO status address = 0
volatile unsigned int * FIFO_write_status_ptr = NULL ;
volatile unsigned int * FIFO_read_status_ptr = NULL ;

// RAM FPGA command buffer
// main bus address 0x0800_0000
//volatile unsigned int * sram_ptr = NULL ;
//void *sram_virtual_base;

// HPS_to_FPGA FIFO write address
// main bus address 0x0000_0000
void *h2p_virtual_base;
volatile unsigned int * FIFO_write_ptr = NULL ;
volatile unsigned int * FIFO_read_ptr = NULL ;

// /dev/mem file id
int fd;

// timer variables
struct timeval t1, t2;
double elapsedTime;

/* Flag set by `--verbose'. */
static int verbose_flag = 0;
int helpopt = 0;
int term_out = 0;  // default is not for terminal output

int main (int argc, char *argv[])
{
	int local_index;
	int data[5];  // 8];

  while (1)
    {
      static struct option long_options[] =
    {
      /* These options don't set a flag.
         We distinguish them by their indices. */
      {"verbose",    no_argument,       0, 'v'},
      {"terminal",    no_argument,       0, 't'},
      {"help",    no_argument,       0, '?'},
      {"help",    no_argument,       0, 'h'},
      {0,         0,                 0,  0}
    };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      int c = getopt_long (argc, argv, "vt?h",
               long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
    break;

      switch (c)
    {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
        printf (" with arg %s", optarg);
      printf ("\n");
      break;
    case 'v':
      puts ("option -v\n");
      verbose_flag = 1;
      term_out = 1;
      break;
    case 't':
      if(verbose_flag) puts ("option -t\n");
      term_out = 1;
      break;
    case '?':
        if(verbose_flag) puts ("option -?\n");
        helpopt = 1;
    case 'h':
      if(helpopt == 0)
      {
    	if(verbose_flag)  puts ("option -h\n");
      }
      puts("commandOne - this turns a single LED On or Off depending on arguments");
      puts("  passed in on the command line.");
      puts("  This has two input parameters which must be entered in this order.");
      puts("  #1 -> LED number to be modified (0 to 3)");
      puts("  #2 -> condition value ( 0 or 1 , for OFF and ON");
      puts("  Option -v prints verbose status output (also sets -t option).");
      puts("  Option -t outputs as a command line terminal, otherwise output is");
      puts("     as though the return strings are being passed over a serial interface");
      puts("    first line - status, second thru fourth line are returned data - (which is");
      puts("    meaningless in this case).");
      puts("  Options -? or -h print this help text and does not execute the command.");
      puts("  Errors are always written to screen.\n");
      puts("  Example;   ");
      puts("  > commandOne 3 0 -t ");
      puts("  > Ok");
      puts("  This runs the command in terminal mode (interactive use) and turns the");
      puts("  #3 LED off.  The returned status here shows that the command");
      puts("  executed correctly.");
      puts("  > commandOne 2 1");
      puts("  > 1");
      puts("  > 0");
      puts("  > 0");
      puts("  > 0");
      puts("  This runs the command in serial mode (RS232 user, so the return values are");
      puts("  sent as four numeric strings).  The command sets the condition of LED #2 to ON.");
      puts("  The returned data shows that the command executed correctly.");
      puts("  The three remaining zeros do not represent anything, just empty arguments.");
      exit(0);
      break;

    default:
      abort ();
    }
    }

  if (verbose_flag)
    puts ("verbose flag is set");

  /* Print any remaining command line arguments (not options). */
  local_index = 0;
  data[1] = 4;  // param1
  data[2] = 4;  // param2, default is invalid argument so if no parameter passed we will not do anything
  data[3] = 4;  // param3
  if (optind < argc)
    {
	  if (verbose_flag)
		  printf ("non-option ARGV-elements: \n");
      while (optind < argc)
      {  // this line is a warning only so ignore if serial
    	  local_index++;
// jmb    	  if(term_out == 1)   printf ("argument %s doesn't make sense for this command\n", argv[optind++]);
    	  if(local_index == 1) data[1] = atoi(argv[optind]);
    	  if(local_index == 2) data[2] = atoi(argv[optind]);
    	  if(local_index == 3) data[3] = atoi(argv[optind]);
    	  optind++;
      }
      if (verbose_flag)
    	  putchar ('\n');
    }


	// === get FPGA addresses ==================
    // Open /dev/mem
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 )
	{
		if(term_out == 1)
			printf( "ERROR: could not open \"/dev/mem\"...\n" );
		else
			printf("0\n0\n0\n0\n");
		return( 1 );
	}

	//============================================
    // get virtual addr that maps to physical
	// for light weight bus
	// FIFO status registers
	h2p_lw_virtual_base = mmap( NULL, FIFO_CSRREGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FIFO_CSRREGS_BASE );
	if( h2p_lw_virtual_base == MAP_FAILED ) {
		if(term_out == 1)
			printf( "ERROR: mmap1() failed...\n" );
		else
			printf("0\n0\n0\n0\n");
		close( fd );
		return(1);
	}
	// the two status registers
	FIFO_write_status_ptr = (unsigned int *)(h2p_lw_virtual_base);
	// From Qsys, second FIFO is 0x20
	FIFO_read_status_ptr = (unsigned int *)(h2p_lw_virtual_base + FIFO_CSRREGS_SPAN);

	//============================================
/*
	// scratch RAM FPGA parameter addr
	sram_virtual_base = mmap( NULL, FPGA_ONCHIP_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_ONCHIP_BASE);

	if( sram_virtual_base == MAP_FAILED )
	{
		if(term_out == 1)
			printf( "ERROR: mmap2() failed...\n" );
		else
			printf("0\n0\n0\n0\n");
		close( fd );
		return(1);
	}
    // Get the address that maps to the RAM buffer
	sram_ptr =(unsigned int *)(sram_virtual_base);

	// ===========================================
*/
	// FIFO write addr
	h2p_virtual_base = mmap( NULL, FIFO_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FIFO_BASE);

	if( h2p_virtual_base == MAP_FAILED ) {
		if(term_out == 1)
			printf( "ERROR: mmap3() failed...\n" );
		else
			printf("0\n0\n0\n0\n");
		close( fd );
		return(1);
	}
    // Get the address that maps to the FIFO read/write ports
	FIFO_write_ptr =(unsigned int *)(h2p_virtual_base);
	FIFO_read_ptr = (unsigned int *)(h2p_virtual_base + FIFO_SPAN);

	//============================================
	int N ;
//	int data[5];  // 8];
	int retdata[5];  // 8];
	long int retparams[3];
	int i ;

		N=4;
		// generate a sequence
		data[0] = 1;  // command number for commandOne
		// these are four data ints we pass over to the FPGA
//		data[1] = 0;  // param1 hiword  // we get these values from the command line
//		data[2] = 0;  // param1 loword
//		data[3] = 0;  // param2 hiword

		// ======================================
		// send array to FIFO and read entire block
		// ======================================
		// print fill levels
 	    if (verbose_flag)
 	    {
 	    	printf("=====================\n\r");
 	    	printf("fill levels before block write\n\r");
 	    	printf("write=%d read=%d\n\r", WRITE_FIFO_FILL_LEVEL, READ_FIFO_FILL_LEVEL);
 	    }
		// send array to FIFO and read block
		for (i=0; i<N; i++){
			// wait for a slot
			// do the FIFO write
			FIFO_WRITE_BLOCK(data[i]);
		}

 	    if (verbose_flag)
 	    {
 	    	printf("fill levels before block read\n\r");
 	 	    printf("write=%d read=%d\n\r", WRITE_FIFO_FILL_LEVEL, READ_FIFO_FILL_LEVEL);
 	    }
 	    usleep(40000);  // give the FPGA time to finish working

		// get array from FIFO until there is data in the FIFO
 	    i=0;
		while (!READ_FIFO_EMPTY) {
			retdata[i] = FIFO_READ;
			if (i>N) i=N;
			// print array from FIFO read port
	 	    if (verbose_flag)	printf("return=%d %d %d\n\r", retdata[i], WRITE_FIFO_FILL_LEVEL, READ_FIFO_FILL_LEVEL) ;
	 	    i++;
		}

		// FIFO fill levels
 	    if (verbose_flag)
 	    {
 	    	printf("fill levels after block read\n\r");
 	    	printf("write=%d read=%d\n\r", WRITE_FIFO_FILL_LEVEL, READ_FIFO_FILL_LEVEL);
 	    	printf("=====================\n\r");
 	    }
 	    // if the function performed successfully the first returned word will be the same as
 	    //  the first word sent (which is the command number)
 	    if(retdata[0] != data[0])
 	    {
 			if(term_out == 1)
 			{
 				printf("ERROR - command did not execute correctly\n\r");
 			}
 			else
 			{
 				printf("0\n");
 				retparams[0] = retdata[1];
 				printf("%ld \n",retparams[0]);
 				retparams[1] = retdata[2];
 				printf("%ld \n",retparams[1]);
 				retparams[2] = retdata[3];
 				printf("%ld \n",retparams[2]);
 			}
 	    }
 	    else
 	    {
 			if(term_out == 0)
 			{  // its a serial interface so print strings to return via serial interface
 				printf("%d\n",retdata[0]);
 				retparams[0] = retdata[1];
 				printf("%ld \n",retparams[0]);
 				retparams[1] = retdata[2];
 				printf("%ld \n",retparams[1]);
 				retparams[2] = retdata[3];
 				printf("%ld \n",retparams[2]);
 			}
 	    }
	    // we will do something with the returned data later
	exit(0);
} // end main

//////////////////////////////////////////////////////////////////


