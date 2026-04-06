#include "emsdri.h"
#include <stdio.h>
#include <conio.h>

// Main program
int main(void) {
    // Step 1: Declare variables
    // 1.1 EMS information structure to store driver details
    struct EMSInfo ems;

    // 1.2 EMS handle for allocated memory (must be initialized to 0)
    unsigned int handle = 0;

    // 1.3 Buffer for reading strings from EMS
    char buffer[256];

    // 1.4 Far pointer for direct EMS memory access
    char far *ems_ptr;

    // 1.5 Error flag for program execution
    int error = 0;

    // Step 2: Display program header
    printf("========================================\n");
    printf("   EMS Memory Demonstration Program    \n");
    printf("   for Borland C++ / Turbo C++        \n");
    printf("========================================\n\n");

    // Step 3: Initialize EMS driver
    // 3.1 Check if EMS driver is present in the system
    if (!ems_check_driver()) {
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }

    // Step 4: Get EMS system information
    // 4.1 Retrieve version, page frame address, and page counts
    if (!ems_get_info(&ems)) {
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }

    // Step 5: Verify sufficient EMS memory is available
    // 5.1 Check if at least 2 EMS pages (32 KB) are free
    if (ems.free_pages < 2) {
        printf("\nNot enough free EMS memory (need at least 2 pages)\n");
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }

    // Step 6: Allocate EMS memory pages
    // 6.1 Allocate 2 EMS pages (32 KB total) and receive a handle
    if (!ems_alloc_pages(2, &handle)) {
        printf("Allocation failed with error: %d\n", ems_error);
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }
    printf("Handle allocated: %d\n", handle);

    // Step 7: Map windows
    if (!ems_map_page(handle, 0, 0)) {
        printf("Initial map failed with error: %d\n", ems_error);
        return 1;
    }

    // Step 8: Write data to EMS using unsafe functions
    // 8.1 Write first string to logical page 0 at offset 0
    ems_write_string(ems.pageframe_seg, 0, 0,
                         "Hello from EMS! This string is stored in expanded memory.");
    printf("String 1 written to logical page 0, offset 0\n");

    // 8.2 Write second string to logical page 0 at offset 100
    ems_write_string(ems.pageframe_seg, 0, 100,
                         "This is the second string, written at offset 100 in EMS.");
    printf("String 2 written to logical page 0, offset 100\n");

    // Step 9: Read data from EMS using unsafe functions
    // 9.1 Read and display first string from logical page 0 at offset 0
    ems_read_string(ems.pageframe_seg, 0, 0, buffer, sizeof(buffer));
    printf("Read unsafe from window 0 offset 0: %s\n", buffer);

    // 9.2 Read and display second string from logical page 0 at offset 100
    ems_read_string(ems.pageframe_seg, 0, 100, buffer, sizeof(buffer));
    printf("Read unsafe from window 0 offset 100: %s\n", buffer);

    // Step 10: Test data persistence across different physical windows
    // 10.1 Map logical page 0 to physical window 1
    printf("\n========================================\n");
    printf("Testing physical window mapping\n");
    printf("========================================\n");
    printf("Mapping logical page 0 to physical window 1...\n");
    if (!ems_map_page(handle, 0, 1)) {
        printf("Map failed with error: %d\n", ems_error);
        error = 1;
        goto cleanup;
    }
    printf("Mapping successful!\n");

    // 10.2 Read data directly through physical window 1 using far pointer
    // NOTE: Physical window 1 starts at offset 0x4000 (16KB) from page frame
    ems_ptr = (char far *)MK_FP(ems.pageframe_seg, 0x4000);
    printf("\nDirect read via window 1 (offset 0x4000): %Fs\n", ems_ptr);

    // 10.3 Map logical page 0 to physical window 0
    printf("\nMapping logical page 0 to physical window 0...\n");
    if (!ems_map_page(handle, 0, 0)) {
        printf("Map failed with error: %d\n", ems_error);
        error = 1;
        goto cleanup;
    }
    printf("Mapping successful!\n");

    // 10.4 Read data directly through physical window 0 using far pointer
    // NOTE: Physical window 0 starts at offset 0x0000 from page frame
    ems_ptr = (char far *)MK_FP(ems.pageframe_seg, 0);
    printf("Direct read via window 0 (offset 0x0000): %Fs\n", ems_ptr);

    // 10.5 Compare results to demonstrate potential EMS bug
    printf("\n--- Analysis ---\n");
    printf("If both direct reads show the same correct data, EMS works correctly.\n");
    printf("If window 0 shows garbage, your EMS driver has a window sync bug.\n");
    printf("Safe functions (ems_read_string_safe) work correctly in both cases.\n");

    // Step 9.6 Demonstrate safe function still works after window changes
    ems_read_string_safe(handle, 0, 0, buffer, sizeof(buffer));
    printf("\nSafe read after window remapping: %s\n", buffer);

cleanup:
    // Step 10: Cleanup and resource deallocation
    // 10.1 Display cleanup section header
    printf("\n========================================\n");
    printf("CLEANUP\n");
    printf("========================================\n");

    // 10.2 Unmap all physical windows before freeing memory
    //      This ensures no windows point to the handle being freed
    printf("Unmapping all physical windows...\n");
    ems_unmap_page(0);  // Unmap window 0
    ems_unmap_page(1);  // Unmap window 1
    ems_unmap_page(2);  // Unmap window 2
    ems_unmap_page(3);  // Unmap window 3

    // 10.3 Free allocated EMS pages
    //      Only free if handle is valid (not zero)
    if (handle != 0) {
        printf("Freeing handle %d...\n", handle);
        if (ems_free_pages(handle)) {
            printf("Memory successfully freed.\n");
        } else {
            printf("Error freeing memory: %d\n", ems_error);
        }
        // 10.4 Reset handle to zero after freeing
        handle = 0;
    }

    // Step 11: Display program completion status
    printf("\n========================================\n");
    if (error) {
        printf("   Program completed with errors!      \n");
    } else {
        printf("   Program completed successfully!     \n");
    }
    printf("========================================\n");

    // Step 12: Wait for user input before exiting (restored)
    printf("\nPress any key to exit...\n");
    getch();

    // Step 13: Return error code (0 = success, non-zero = error)
    return error;
}
