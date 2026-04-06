/* *
 * emsdri.c
 * Test of EMM386 EMS driver
 * */

#include "emsdri.h"

unsigned char ems_error;

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
        printf("Error getting EMS version: %u\n", regs.h.ah);
        return 0;
    }
    info->version = regs.h.al;
    printf("EMS version: %u\n", regs.h.al);

    // Get Page Frame address (EMS window segment)
    regs.x.ax = 0x4100;
    int86(0x67, &regs, &regs);
    if (regs.h.ah != 0) {
        printf("Error getting Page Frame address: %u\n", regs.h.ah);
        return 0;
    }
    info->pageframe_seg = regs.x.bx;
    printf("Page Frame (EMS window) at address: %04X:0000\n", info->pageframe_seg);

    // Get page information
    regs.x.ax = 0x4200;
    int86(0x67, &regs, &regs);
    if (regs.h.ah != 0) {
        printf("Error getting page information: %u\n", regs.h.ah);
        return 0;
    }
    info->free_pages  = regs.x.bx;
    info->total_pages = regs.x.dx;
    printf("EMS pages: %u total, %u free\n", info->total_pages, info->free_pages);

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

// Unmap physical window
int ems_unmap_page(unsigned char physical_page)
{
    return ems_map_page(0, 0xFFFF, physical_page);
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

// Write string to EMS (through physical page)
void ems_write_string(unsigned int pageframe_seg, unsigned int physical_page,
                      unsigned int offset, const char *text) {
    char far *ptr;

    ptr = (char far *)MK_FP(pageframe_seg + physical_page * (EMS_PAGE_SIZE >> 4), offset);

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

    ptr = (char far *)MK_FP(pageframe_seg + physical_page * (EMS_PAGE_SIZE >> 4), offset);

    while (i < max_len - 1 && *ptr) {
        buffer[i] = *ptr;
        ptr++;
        i++;
    }
    buffer[i] = '\0';
}

// Move memory between conventional and EMS memory
struct EMSMoveInfo {
    unsigned long length;

    unsigned char src_type;
    unsigned int  src_handle;
    unsigned int  src_offset;
    unsigned int  src_page_seg;

    unsigned char dst_type;
    unsigned int  dst_handle;
    unsigned int  dst_offset;
    unsigned int  dst_page_seg;
};

int ems_move_memory(struct EMSMoveInfo far *m)
{
    union REGS regs;
    struct SREGS sregs;

    memset(&sregs, 0, sizeof(sregs));

    regs.x.ax = 0x5700;
    regs.x.si = FP_OFF(m);
    sregs.ds  = FP_SEG(m);

    int86x(0x67, &regs, &regs, &sregs);
    ems_error = regs.h.ah;

    return (ems_error == 0 || ems_error == 0x92);
}

void ems_write_string_safe(unsigned int handle, unsigned int logical_page,
                           unsigned int offset, const char *text)
{
    struct EMSMoveInfo m;

    m.length       = (unsigned long)strlen(text) + 1;

    m.src_type     = 0;
    m.src_handle   = 0;
    m.src_offset   = FP_OFF(text);
    m.src_page_seg = FP_SEG(text);

    m.dst_type     = 1;
    m.dst_handle   = handle;
    m.dst_offset   = offset;
    m.dst_page_seg = logical_page;

    ems_move_memory(&m);
}

void ems_read_string_safe(unsigned int handle, unsigned int logical_page,
                          unsigned int offset, char *buffer, int max_len)
{
    struct EMSMoveInfo m;

    m.length       = (unsigned long)max_len;

    m.src_type     = 1;
    m.src_handle   = handle;
    m.src_offset   = offset;
    m.src_page_seg = logical_page;

    m.dst_type     = 0;
    m.dst_handle   = 0;
    m.dst_offset   = FP_OFF(buffer);
    m.dst_page_seg = FP_SEG(buffer);

    ems_move_memory(&m);
    buffer[max_len - 1] = '\0';
}
