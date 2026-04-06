#include "emsdri.h"
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <string.h>   // для strcmp

int main(void) {
    const char msg1[] = "Hello from EMS! This string is stored in expanded memory.";
    const char msg2[] = "Second string at offset 100 in EMS.";
    char buffer[256];

    int error = 0;
    int i;
    struct EMSInfo ems;

    char far *ems_ptr;
    unsigned int handle = 0;

    printf("========================================\n");
    printf("   EMS Memory Demonstration Program    \n");
    printf("   for Borland C++ / Turbo C++        \n");
    printf("========================================\n\n");

    // 1. Check EMS driver
    if (!ems_check_driver()) {
        printf("EMS driver not found!\n");
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }
    printf("EMS driver detected.\n");

    // 2. Get EMS info
    if (!ems_get_info(&ems)) {
        printf("Failed to get EMS info\n");
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }

    printf("EMS Version: %d.%d\n", ems.version >> 8, ems.version & 0xFF);
    printf("Total pages: %d\n", ems.total_pages);
    printf("Free pages: %d\n", ems.free_pages);
    printf("Page frame segment: %04X\n", ems.pageframe_seg);

    // Check memory
    if (ems.free_pages < 2) {
        printf("\nNot enough free EMS memory (need at least 2 pages)\n");
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }

    // 3. Allocate 2 pages
    if (!ems_alloc_pages(2, &handle)) {
        printf("Allocation failed with error: %d\n", ems_error);
        printf("\nPress any key to exit...\n");
        getch();
        return 1;
    }
    printf("Handle allocated: %d\n", handle);

    // 4. Map logical page 0 to physical window 0
    printf("\nMapping logical page 0 to physical window 0...\n");
    if (!ems_map_page(handle, 0, 0)) {
        printf("Map failed with error: %d\n", ems_error);
        error = 1;
        goto cleanup;
    }
    printf("Mapping successful!\n");

    // 5. Write data to EMS
    printf("\n--- Writing data to EMS ---\n");

    ems_write_string(ems.pageframe_seg, 0, 0, msg1);
    printf("String 1 written at %04X:0000\n", ems.pageframe_seg);

    ems_write_string(ems.pageframe_seg, 0, 100, msg2);
    printf("String 2 written at %04X:0064\n", ems.pageframe_seg);

    // 6. Remap logical page 0 to physical window 1
    printf("\nMapping logical page 0 to physical window 0...\n");
    if (!ems_map_page(handle, 0, 1)) {
        printf("Map failed with error: %d\n", ems_error);
        error = 1;
        goto cleanup;
    }
    printf("Mapping successful!\n");

    // 7. Read data from EMS
    printf("\n--- Reading data from EMS ---\n");

    ems_read_string(ems.pageframe_seg, 1, 0, buffer, sizeof(buffer));
    printf("Offset 0: %s\n", buffer);

    ems_read_string(ems.pageframe_seg, 1, 100, buffer, sizeof(buffer));
    printf("Offset 100: %s\n", buffer);

    // Verify write
    printf("\n--- Verification ---\n");
    ems_read_string(ems.pageframe_seg, 1, 100, buffer, sizeof(buffer));
    if (strcmp(buffer, msg2) == 0) {
        printf("String 1 verified OK\n");
    } else {
        printf("String 1 verification FAILED\n");
        error = 1;
    }

    // 7. Direct access via far pointer (unsafe)
    printf("\n--- Direct access via far pointer ---\n");
    ems_ptr = MK_FP(ems.pageframe_seg + (EMS_PAGE_SIZE >> 4), 0);
    if (ems_ptr != NULL) {
        char safe_buf[81];
        printf("EMS poiter: 0x%Fp\n", ems_ptr);
        for (i = 0; i < 80 && ems_ptr[i] != '\0'; i++) {
            safe_buf[i] = ems_ptr[i];
        }
        safe_buf[i] = '\0';
        printf("EMS content: %s\n", safe_buf);
    } else {
        printf("Failed to create far pointer\n");
        error = 1;
    }

cleanup:
    // Cleanup
    printf("\n========================================\n");
    printf("CLEANUP\n");
    printf("========================================\n");

    // Unmap all windows
    printf("Unmapping all physical windows...\n");
    for (i = 0; i < 4; i++) {
        ems_map_page(0, 0, i);
    }

    // Free memory
    if (handle != 0) {
        printf("Freeing handle %d...\n", handle);
        if (ems_free_pages(handle)) {
            printf("Memory successfully freed.\n");
        } else {
            printf("Error freeing memory: %d\n", ems_error);
            error = 1;
        }
    }

    printf("\n========================================\n");
    if (error) {
        printf("   Program completed with errors!\n");
    } else {
        printf("   Program completed successfully!\n");
    }
    printf("========================================\n");
    printf("\nPress any key to exit...\n");
    getch();

    return error;
}
