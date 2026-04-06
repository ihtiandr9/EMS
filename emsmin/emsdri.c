#include <stdio.h>
#include <conio.h>
#include <dos.h>

// EMS function codes (interrupt 0x67)
#define EMS_ALLOC_PAGES      0x43
#define EMS_MAP_PAGE         0x44
#define EMS_FREE_PAGES       0x45
#define EMS_GET_VERSION      0x46
#define EMS_GET_PAGEFRAME    0x41

// EMS page size (16 KB)
#define EMS_PAGE_SIZE        16384

// Global error code variable
unsigned char ems_error = 0;

// Structure for EMS information
struct EMSInfo {
    unsigned int version;       // EMS version (e.g., 0x40 = 4.0)
    unsigned int pageframe_seg; // Page Frame segment (EMS window)
    unsigned int total_pages;   // Total available pages
    unsigned int free_pages;    // Free pages
    unsigned int handle;        // Allocated memory handle
};

// EMS function call
unsigned char ems_call(unsigned char ah, unsigned int bx, unsigned int dx, unsigned char al) {
    union REGS regs;

    regs.x.ax = (ah << 8) | al;
    regs.x.bx = bx;
    regs.x.dx = dx;

    int86(0x67, &regs, &regs);
    ems_error = regs.h.ah;

    return ems_error;
}

// Check if EMS driver is present
int ems_check_driver(void) {
    union REGS regs;

    // Function 0x43 of interrupt 0x2F - check EMM presence
    regs.x.ax = 0x4300;
    int86(0x2F, &regs, &regs);

    if (regs.h.al == 0x80) {
        printf("EMS driver found.\n");
        return 1;
    } else {
        printf("EMS driver NOT FOUND!\n");
        printf("Load EMM386.EXE in config.sys:\n");
        printf("DEVICE=C:\\DOS\\EMM386.EXE RAM\n");
        return 0;
    }
}

// Get EMS version and Page Frame address
int ems_get_info(struct EMSInfo *info) {
    union REGS regs;

    // Get version
    regs.x.ax = 0x4600;
    int86(0x67, &regs, &regs);
    if (regs.h.ah != 0) {
        printf("Error getting EMS version: %d\n", regs.h.ah);
        return 0;
    }
    info->version = regs.x.ax;
    printf("EMS version: %d.%d\n", regs.h.al, regs.h.ah & 0x0F);

    // Get Page Frame address (EMS window segment)
    regs.x.ax = 0x4100;
    int86(0x67, &regs, &regs);
    if (regs.h.ah != 0) {
        printf("Error getting Page Frame address: %d\n", regs.h.ah);
        return 0;
    }
    info->pageframe_seg = regs.x.bx;
    printf("Page Frame (EMS window) at address: %04X:0000\n", info->pageframe_seg);

    // Get page information
    regs.x.ax = 0x4200;
    int86(0x67, &regs, &regs);
    if (regs.h.ah != 0) {
        printf("Error getting page information: %d\n", regs.h.ah);
        return 0;
    }
    info->total_pages = regs.x.bx;
    info->free_pages = regs.x.dx;
    printf("EMS pages available: %d total, %d free\n",
           info->total_pages, info->free_pages);

    return 1;
}

// Allocate EMS pages
int ems_alloc_pages(unsigned int pages, unsigned int *handle) {
    union REGS regs;

    printf("\nAllocating %d EMS pages (%d KB)...\n", pages, pages * 16);

    regs.x.ax = 0x4300;
    regs.x.bx = pages;
    int86(0x67, &regs, &regs);

    if (regs.h.ah != 0) {
        printf("Error allocating memory: %d\n", regs.h.ah);
        return 0;
    }

    *handle = regs.x.dx;
    printf("Allocation successful! Handle: %d\n", *handle);
    return 1;
}

// Map logical page to physical window
int ems_map_page(unsigned int handle, unsigned int logical_page, unsigned char physical_page) {
    union REGS regs;

    // physical_page: 0, 1, 2, 3 (up to 4 windows of 16 KB each)
    regs.x.ax = 0x4400;
    regs.x.bx = logical_page;
    regs.x.dx = handle;
    regs.h.al = physical_page;

    int86(0x67, &regs, &regs);

    if (regs.h.ah != 0) {
        printf("Error mapping page: %d\n", regs.h.ah);
        return 0;
    }

    return 1;
}

// Free allocated EMS pages
int ems_free_pages(unsigned int handle) {
    union REGS regs;

    regs.x.ax = 0x4500;
    regs.x.dx = handle;
    int86(0x67, &regs, &regs);

    if (regs.h.ah != 0) {
        printf("Error freeing memory: %d\n", regs.h.ah);
        return 0;
    }

    return 1;
}

// Write string to EMS (through physical page )
void ems_write_string(unsigned int pageframe_seg, unsigned int physical_page,
                      unsigned int offset, const char *text) {
    char far *ptr;
    unsigned long line_addr = ((unsigned long)pageframe_seg << 16) +
                       (unsigned long)physical_page * EMS_PAGE_SIZE +
                       offset;

    // Create far pointer (segment:offset)
    ptr = (char far *)line_addr;

    printf("Write to adress: %Fp from line address 0x%lx\n", ptr, line_addr);

    // Write string to EMS
    while (*text) {
        *ptr = *text;
        ptr++;
        text++;
    }
    *ptr = '\0'; // Write terminating null
}

// Read string from EMS
void ems_read_string(unsigned int pageframe_seg, unsigned int physical_page,
                     unsigned int offset, char *buffer, int max_len) {
    char far *ptr;
    int i = 0;

    unsigned long line_addr = ((unsigned long)pageframe_seg << 16) +
                       (unsigned long)physical_page * EMS_PAGE_SIZE +
                       offset;

    // Create far pointer (segment:offset)
    ptr = (char far *)line_addr;

    printf("Read from adress: %Fp from line address 0x%lx\n", ptr, line_addr);


    while (i < max_len - 1 && *ptr) {
        buffer[i] = *ptr;
        ptr++;
        i++;
    }
    buffer[i] = '\0';
}

