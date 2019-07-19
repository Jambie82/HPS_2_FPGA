# HPS_2_FPGA
Example of command &amp; data passing between HPS and FPGA on Altera Arria 10 SoC Devkit Board
  This is a pretty thinned down project to show how to pass data back and forth between the HPS to the FPGA using FIFOs.  The code
is not long so it can be read and understood without too much distraction and hunting.  Since our FPGA needs to do more than just
shuffle bits back and forth with the HPS the code also includes an example of turning on some other FPGA activity based on the
HPS command (see the commandThree example).

  In this we send commands to the FPGA as four 32 bit integers.  The first integer is the command number (i.e. as in 1 for 
command number one, 2 for command number 2, etc) and the other three are optional arguments or data to pass.  In my setup I 
always pass four items even if they are not needed.  The FPGA receives the four items then acts on them and passes back four 
32 bit data items.  If there arent four useful data items to send we send them anyway, so the interface operates in a steady 
lock step fashion.

  In the HPS code (see the .c routines in the 'LinuxProject' folders) the HPS maps to the base address of the lightweight bus 
then uses an offset from that base to access data and status variables that show how the two FIFOs are with respect to holding
useful data going in or out.  The HPS code takes in command line arguments like normal Linux 'C' code and acts on it to perform
the send/receive to cause the FPGA to turn the LEDs on or off.

  In the FPGA code (see 'a10_flat_top.sv' - my code uses System Verilog, though on looking at it I dont see anything here that 
couldn't be done in standrd verilog) there is a section near the start that counts up a counter (to 100,000,000) then rolls it over.
  If a flag variable has been set by the HPS command processor it will also sequence the LEDs, but if not it just counts and rolls over.
Below this is a state machine to operate two FIFOs (actually three small state machines).  There is one FIFO to send data from the 
HPS to the FPGA and a separate FIFO to send data from the FPGA to the HPS.
  Transactions begin at the HPS when it puts data into the HPS_to_FPGA FIFO.  The FPGA is always monitoring this FIFO and when data 
shows up it collects it and when it has four items it acts on them to do something, then it places the return data into the 
FPGA_to_HPS FIFO.  The HPS only monitors the FPGA_to_HPS FIFO when it is expecting a return, so it starts after it sends data to 
the FPGA.  When the HPS sees data in its FIFO it removes it and reports back to the Linux user.
  The state machine to send data from HPS to FPGA has four states (0 -> 2 -> 4 -> 3 -> then back to zero)  
  The state machine to send data from FPGA to HPS has three states (0 -> 4 -> 8 -> then back to zero)
  And there is a separate state machine to process the commands to do useful work, and it has four states (0 -> 5 -> 6 -> then 
back and forth with state 7 -> then back to zero).  State 6 is where the useful work is done (for this example the LEDs on/off), 
all the rest just check FIFO status and move data.  (See the state transition diagram 'state.jpg' for a graphical view of the sequence.)

To Run the Demo;
1. Start your A10 SoC Devkit board and boot up Linux in your terminal window.

2. Copy the three executeable files from the 'HPSTest\LinuxExecuteables' folder over to your current Linux directory 
on your Devkit board.

3. Start Quartus on your PC and start the project in the 'HPSTest\QuartusProject\a10_devkit_test' folder.

4. Using the 'Programmer' item on the 'Tools' menu in Quartus download the 
'ghrd_10as066n2.sof' file found in the project's 'output_files' folder into the FPGA.

5. Before executing the programs (commandOne, commandTwo, and comandThree) in your Linux window be sure you make them 
executeable as shown below;

> chmod 777 commandOne
  
> chmod 777 commandTwo
 
> chmod 777 commandThree

6. You can get help on any of the commnandXXX files by running them with the -h switch;
> ./commandOne -h

commandOne - this turns a single LED On or Off depending on arguments

  passed in on the command line.
  
    This has two input parameters which must be entered in this order.
    
  #1 -> LED number to be modified (0 to 3)
  
  #2 -> condition value ( 0 or 1 , for OFF and ON
  
  Option -v prints verbose status output (also sets -t option).
  
  Option -t outputs as a command line terminal, otherwise output is
  
     as though the return strings are being passed over a serial interface
     
    first line - status, second thru fourth line are returned data - (which is
    
    meaningless in this case).
    
  Options -? or -h print this help text and does not execute the command.
  
  Errors are always written to screen.
  

  Example;
  > commandOne 3 0 -t
  
  > Ok
  
  This runs the command in terminal mode (interactive use) and turns the
  #3 LED off.  The returned status here shows that the command
  executed correctly.
  
  > commandOne 2 1
  
  > 1
  
  > 0
  
  > 0
  
  > 0
  
  This runs the command in serial mode (RS232 user, so the return values are
  sent as four numeric strings).  The command sets the condition of LED #2 to ON.
  The returned data shows that the command executed correctly.
  The three remaining zeros do not represent anything, just empty arguments.

- commandOne passes command line args to turn lights on/off.  
- commandTwo has no arguments and just toggles the lights.  
- commandThree turns and auto-sequencer on and off so the FPGA runs the lights on its own.  (If the FPGA leaves lights on when it 
is turned off you can get it to turn them off by starting it again (using commandThree) and waiting until it has the lights all off, 
then run commandThree again to turn it off (but be quick because you only have about a second).

- I built the FPGA project using Quartus v17.0.2.  You can run the QSYS tool to see the setup of the two FIFOs.
- I compiled the Linux programs on my PC using Eclipse for DS-5 v5.26.0, though I have done the same using v5.23.0 in the past.
- There are lots more details on exactly where the FIFO documentation is located and where the address connections are defined
but this is all I have time for today.

You can download the Quartus project in a zip file in the github location below (it is named HPStest.zip)

The Linux code and other files can be found at my github here >> https://github.com/Jambie82/HPS_2_FPGA

Yes, there are always more details. We need the address references to the hardware elements used by the HPS to write Linux programs (in C) to interface the HPS to the hardware.

These addresses come from a header file (in our case here it is arria10_hps_0_arm_9_0.h, but there are others). This header file is generated by a Quartus (Linux) command line tool called ‘sopc-create-header-files’, which is in the Windows directory for Quartus (on my machine it is C:\intelFPGA_pro\17.0\Quartus\sopc_builder, but this is installation dependent). You should put this folder in your path so you dont need to navigate to it when you want to use this.

This tool uses a file created by Quartus when the FPGA build was completed as its input, and its output is one or more ‘C’ header files. The input file has a suffix that ends in ‘.sopcinfo’ and has a prefix that matches your project name. During compilation Quartus creates severel versions of this file at different stages of the compile so if you go exploring you will find them in subdirectories labeled ‘synthesized’, ‘planned’, ‘placed’, ‘routed’, and ‘final’. The one you want is in the ‘final’ directory which is (on my computer) C:\HPStest\a10_devkit_test\qdb_compiler\ghrd_10as066n2\root_partition\17.0.2\final\1\sld_syconfiles

You wont be working in that directory since it can be overwritten on each compile so you should copy the a10_devkit_flat.sopcinfo file into a working directory you make for your Linux C work.

Since the sopc-create-header-files program is a Linux program you need to start a Linux command line shell. Quartus provides one for Windows and it is started by running the Embedded_Command_Line.bat batch file in the (on my installation) C:\intelFPGA_pro\17.0\embedded directory.

One your Linux command shell is started you shoud navigate to your working directory, copy your .sopcinfo file into that directory then run the sopc-create-header-files program and create the C header files.

imageimage.png998x581 148 KB

The header file we want for this program is arria10_hps_0_arm_9_0.h. This contains a lot of info but we only need a few references to addresses inthe FPGA hrdware from this file. Look for the line
#define ONCHIP_MEMORY2_0_BASE 0xc0000000
This has the bas address of the on chip memory on the FPGA. The FIFO memory comes from this memory pool so we need to know where the memory locations are to read and write to them.
We also need the =references
#define FIFO_HPS_TO_FPGA_IN_BASE 0xc0040000
#define FIFO_HPS_TO_FPGA_IN_SPAN 4
so we know where to find the FIFO controls and the length of the FIFO (this was defined earlier in QSYS)

I would just copy the parts that are needed for my C program to make for clearer reading. Once you memory map to these addresses (or offsets from them) your C can read and write to locations that the FPGA will also recognize and you can control the hardware or share control with the FPGA.
