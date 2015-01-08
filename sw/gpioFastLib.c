/*
 * This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/*
 * This program is written By Hardelettrosoft Team. Thanks to Giuseppe Manzoni.
 *
 * It is based on the work of Roberto Asquini for its FastIO_C_example.
 *  FastIO_C_example is based on the work of Douglas Gilbert for its mem2io.c
 *  for accessing input output register of the CPU from userspace
 *
 * Changelog - Hardlettrosoft
 * ---------------------------------
 * 2013 April 01 - First release
 * 2013 April 05 - Update comments and text formatting
 * 2013 April 08 - Update function name readGpio(..) in fastReadGpio(..)
 *
 */


#define verbose 0




#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#define DEV_MEM "/dev/mem"
#define MAP_SIZE 4096   /* needs to be a power of 2! */
#define MAP_MASK (MAP_SIZE - 1)

// addresses of interesting physical Port C registers
#define PIOC_OER  0xfffff810   // (Wr) PIO Output Enable Register -> 1 to the bit that has to be put in output
#define PIOC_ODR  0xfffff814   // (Wr) PIO Output Disable Register -> 1 to the bit that has to be put in input
#define PIOC_SODR 0xfffff830   // (Wr) PIO Set Output Data Register -> 1 to the output bit that has to be set
#define PIOC_CODR 0xfffff834   // (Wr) PIO Clear Output Data Register -> 1 to the output bit that has to be cleared
#define PIOC_ODSR 0xfffff838   // (Rd) PIO Output Data Status Register : to read the output status of the PortC pins
#define PIOC_PDSR 0xfffff83C   // (Rd) PIO Pin Data Status Register _ to read the status of the PortC input pins

int mem_fd;
void * mmap_ptr;
off_t mask_addr;

// variables to store the mapped address of the interesting registers
void * mapped_PIOC_OER_addr;
void * mapped_PIOC_ODR_addr;
void * mapped_PIOC_SODR_addr;
void * mapped_PIOC_CODR_addr;
void * mapped_PIOC_ODSR_addr;
void * mapped_PIOC_PDSR_addr;


unsigned int init_memoryToIO(void);
unsigned int close_memoryToIO(void);
void setPortCinInput(void);
void setPortCinOutput(void);
unsigned int readGeneralRegister(unsigned int reg);
unsigned int readPortCoutbits(void);
unsigned int readPortCinbits(void);
void writePortC(unsigned int uintData);
void setGpioinInput(unsigned int uintGpioC);
void setGpioinOutput(unsigned int uintGpioC);
void fastSetGpio(unsigned int uintGpioC);
void fastClearGpio(unsigned int uintGpioC);
void fastSetPC3(void);
void fastClearPC3(void);
unsigned int fastReadGpio(unsigned int uintGpioC);


// to map in a local page the peripheral address registers used
unsigned int init_memoryToIO(void) {
    mem_fd = -1;

    if ((mem_fd = open(DEV_MEM, O_RDWR | O_SYNC)) < 0) {
        printf("open of " DEV_MEM " failed");
        return 1;
    } else if (verbose) printf("open(" DEV_MEM "O_RDWR | O_SYNC) okay\n");
    mask_addr = (PIOC_OER & ~MAP_MASK);  // preparation of mask_addr (base of the memory accessed)

    if (verbose) printf ("Mask address = %08x\n",mask_addr);
    mmap_ptr = (void *)-1;
    mmap_ptr = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED, mem_fd, mask_addr);
    if (verbose) printf ("Mmap_ptr = %08x\n",mmap_ptr);

    if ((void *)-1 == mmap_ptr) {
        printf("addr=0x%x, mask_addr=0x%lx :\n", PIOC_OER, mask_addr);
        printf("    mmap");
        return 1;
    }
    if (verbose) printf("mmap() ok, mask_addr=0x%lx, mmap_ptr=%p\n", mask_addr, mmap_ptr);
    mapped_PIOC_OER_addr = mmap_ptr + (PIOC_OER & MAP_MASK);
    mapped_PIOC_ODR_addr = mmap_ptr + (PIOC_ODR & MAP_MASK);
    mapped_PIOC_SODR_addr = mmap_ptr + (PIOC_SODR & MAP_MASK);
    mapped_PIOC_CODR_addr = mmap_ptr + (PIOC_CODR & MAP_MASK);
    mapped_PIOC_ODSR_addr = mmap_ptr + (PIOC_ODSR & MAP_MASK);
    mapped_PIOC_PDSR_addr = mmap_ptr + (PIOC_PDSR & MAP_MASK);
    return 0;
}


// closing memory mapping
unsigned int close_memoryToIO(void) {
    if (-1 == munmap(mmap_ptr, MAP_SIZE)) {
        printf("mmap_ptr=%p:\n", mmap_ptr);
        printf("    munmap");
        return 1;
    } else if (verbose) printf("call of munmap() ok, mmap_ptr=%p\n", mmap_ptr);
    if (mem_fd >= 0) close(mem_fd);
    return 0;
}


// put PortC in input mode
void setPortCinInput(void) {
    *((unsigned long *)mapped_PIOC_ODR_addr) = 0xffffffff;
}


// put PortC in output mode
void setPortCinOutput(void) {
    *((unsigned long *)mapped_PIOC_OER_addr) = 0xffffffff;
}


unsigned int readGeneralRegister(unsigned int reg) {
    void * ap;
    unsigned long ul; // returns the content of the CPU register reg

    ap = mmap_ptr + (reg & MAP_MASK);
    ul = *((unsigned long *)ap);    // read the register
    if (verbose) printf("read: addr=0x%x, val=0x%x\n", reg, (unsigned int)ul);
    return (unsigned int)ul;
}


unsigned int readPortCoutbits(void) {
    unsigned long ul;    // returns the content of the register reg

    ul = *((unsigned long *)mapped_PIOC_ODSR_addr);    // read the register
    if (verbose) printf("read: addr=0x%x, val=0x%x\n", PIOC_ODSR, (unsigned int)ul);
    return (unsigned int)ul;
}


unsigned int readPortCinbits(void) {
    unsigned long ul; // returns the content of the register reg

    ul = *((unsigned long *)mapped_PIOC_PDSR_addr);     // read the register
    if (verbose) printf("read: addr=0x%x, val=0x%x\n", PIOC_PDSR, (unsigned int)ul);
    return (unsigned int)ul;
}


// write the output registers of Port C with the value "data"
void writePortC(unsigned int uintData) {
    *((unsigned long *)mapped_PIOC_SODR_addr) = uintData;
    *((unsigned long *)mapped_PIOC_CODR_addr) = ~uintData;
}


/*
Function:       void setGpioinInput(unsigned int uintGpioC)
Aim:            Put given gpio in input mode
Parameters:
		uintGpioC -> Port C bit
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
void setGpioinInput(unsigned int uintGpioC) {
    unsigned long ulnGpio = 1 << uintGpioC;
    *((unsigned long *)mapped_PIOC_ODR_addr) = ulnGpio;
}


/*
Function:       void setGpioinOutput(unsigned int uintGpioC)
Aim:            Put given gpio in output mode
Parameters:
		uintGpioC -> Port C bit
Return:         -
Revision:       1 - First Release
Author:		Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
void setGpioinOutput(unsigned int uintGpioC) {
    unsigned long ulnGpio = 1 << uintGpioC;
    *((unsigned long *)mapped_PIOC_OER_addr) = ulnGpio;
}


/*
Function:       void fastSetGpio(unsigned int uintGpioC)
Aim:            Set given gpio
Parameters:
		uintGpioC -> Port C bit
Return:         -
Revision:       1 - First Release
Author:		Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
void fastSetGpio(unsigned int uintGpioC) {
    unsigned long ulnGpio = 1 << uintGpioC;
    *((unsigned long *)mapped_PIOC_SODR_addr) = ulnGpio;
}


/*
Function:       void fastClearGpio(unsigned int uintGpioC)
Aim:            Clear given gpio
Parameters:
		uintGpioC -> Port C bit
Return:         -
Revision:       1 - First Release
Author:		Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
void fastClearGpio(unsigned int uintGpioC) {
    unsigned long ulnGpio = 1 << uintGpioC;
    *((unsigned long *)mapped_PIOC_CODR_addr) = ulnGpio;
}


/*
Function:       void fastSetPC3(void)
Aim:            Set PC3 = N5 = Kernel ID 99
Parameters:     -
Return:         -
Revision:       1 - First Release
Author:		Hardelettrosoft Team, Giuseppe Manzoni
Note:           It is not really more faster than fastSetGpio(..)
*/
void fastSetPC3(void){
    *((unsigned long *)mapped_PIOC_SODR_addr) = 8;
}


/*
Function: 	void fastClearPC3(void)
Aim:		Clear PC3 = N5 = Kernel ID 99
Parameters: 	-
Return: 	-
Revision: 	1 - First Release
Author:		Hardelettrosoft Team, Giuseppe Manzoni
Note: 		It is not really more faster than fastClearGpio(..)
*/
void fastClearPC3(void){
    *((unsigned long *)mapped_PIOC_CODR_addr) = 8;
}


/*
Function: 	unsigned int fastReadGpio(unsigned int uintGpioC)
Aim:		read logic level of given gpio
Parameters:
                uintGpioC -> Port C bit
Return: 	logic level
Revision: 	2 - 2013/04/08 Fix function name, was readGpio
		1 - 2013/04/01 First Release
Author:		Hardelettrosoft
Note: 		-
*/
unsigned int fastReadGpio(unsigned int uintGpioC){
    unsigned long ul; // returns the content of the register reg
    unsigned int uintValue;
    ul = *((unsigned long *)mapped_PIOC_PDSR_addr);     // read the register
    uintValue = ( ul & (1 << uintGpioC)) >> uintGpioC;  // mask and get single bit value
    return uintValue;
}
