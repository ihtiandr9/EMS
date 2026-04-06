/* *
 * emsdri.h
 * */

#ifndef H_EMSDRI_H
#define H_EMSDRI_H

#include <stdio.h>
#include <conio.h>
#include <string.h>
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
extern unsigned char ems_error;

// Structure for EMS information
struct EMSInfo {
    unsigned int version;       // EMS version (e.g., 0x40 = 4.0)
    unsigned int pageframe_seg; // Page Frame segment (EMS window)
    unsigned int total_pages;   // Total available pages
    unsigned int free_pages;    // Free pages
    unsigned int handle;        // Allocated memory handle
};

// EMS function call
unsigned char ems_call(unsigned char ah, unsigned int bx, unsigned int dx, unsigned char al);

// Check if EMS driver is present
int ems_check_driver(void);

// Get EMS version and Page Frame address
int ems_get_info(struct EMSInfo *info);

// Allocate EMS pages
int ems_alloc_pages(unsigned int pages, unsigned int *handle);

// Map logical page to physical window
int ems_map_page(unsigned int handle, unsigned int logical_page, unsigned char physical_page);

// Free allocated EMS pages
int ems_free_pages(unsigned int handle);

// Write string to EMS
void ems_write_string(unsigned int pageframe_seg, unsigned int physical_page,
                      unsigned int offset, const char *text);

// Read string from EMS
void ems_read_string(unsigned int pageframe_seg, unsigned int physical_page,
                     unsigned int offset, char *buffer, int max_len);

// Write string to EMS (through physical page 0)
void ems_write_string_safe(unsigned int handle, unsigned int logical_page,
                           unsigned int offset, const char *text);

// Read string from EMS (through physical page 0)
void ems_read_string_safe(unsigned int handle, unsigned int logical_page,
                          unsigned int offset, char *buffer, int max_len);

int ems_unmap_page(unsigned char physical_page);

#endif