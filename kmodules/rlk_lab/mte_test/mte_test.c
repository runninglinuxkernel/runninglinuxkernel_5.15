   /*
     * To be compiled with -march=armv8.5-a+memtag
     */
    #include <errno.h>
    #include <stdint.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/auxv.h>
    #include <sys/mman.h>
    #include <sys/prctl.h>

    /*
     * From arch/arm64/include/uapi/asm/hwcap.h
     */
    #define HWCAP2_MTE              (1 << 18)

    /*
     * From arch/arm64/include/uapi/asm/mman.h
     */
    #define PROT_MTE                 0x20

    /*
     * From include/uapi/linux/prctl.h
     */
    #define PR_SET_TAGGED_ADDR_CTRL 55
    #define PR_GET_TAGGED_ADDR_CTRL 56
    # define PR_TAGGED_ADDR_ENABLE  (1UL << 0)
    # define PR_MTE_TCF_SHIFT       1
    # define PR_MTE_TCF_NONE        (0UL << PR_MTE_TCF_SHIFT)
    # define PR_MTE_TCF_SYNC        (1UL << PR_MTE_TCF_SHIFT)
    # define PR_MTE_TCF_ASYNC       (2UL << PR_MTE_TCF_SHIFT)
    # define PR_MTE_TCF_MASK        (3UL << PR_MTE_TCF_SHIFT)
    # define PR_MTE_TAG_SHIFT       3
    # define PR_MTE_TAG_MASK        (0xffffUL << PR_MTE_TAG_SHIFT)

    #define MTE_TAG_SHIFT		56

    /*
     * Insert a random logical tag into the given pointer.
     */
    #define insert_random_tag(ptr) ({                       \
            uint64_t __val;                                 \
            asm("irg %0, %1" : "=r" (__val) : "r" (ptr));   \
            __val;                                          \
    })


/* Get allocation tag for the address. */
static unsigned char mte_get_mem_tag(void *addr)
{
	asm("ldg %0, [%0]" : "+r" (addr));

	return (((unsigned long)(addr)) >> MTE_TAG_SHIFT);
}

    /*
     * Set the allocation tag on the destination address.
     */
    #define set_tag(tagged_addr) do {                                      \
            asm volatile("stg %0, [%0]" : : "r" (tagged_addr) : "memory"); \
    } while (0)

    int main()
    {
            unsigned char *a, *b;
            unsigned long page_sz = sysconf(_SC_PAGESIZE);
            unsigned long hwcap2 = getauxval(AT_HWCAP2);

            /* check if MTE is present */
            if (!(hwcap2 & HWCAP2_MTE))
                    return EXIT_FAILURE;

            /*
             * Enable the tagged address ABI, synchronous or asynchronous MTE
             * tag check faults (based on per-CPU preference) and allow all
             * non-zero tags in the randomly generated set.
             */
            if (prctl(PR_SET_TAGGED_ADDR_CTRL,
                      PR_TAGGED_ADDR_ENABLE | PR_MTE_TCF_SYNC | PR_MTE_TCF_ASYNC |
                      (0xfffe << PR_MTE_TAG_SHIFT),
                      0, 0, 0)) {
                    perror("prctl() failed");
                    return EXIT_FAILURE;
            }

            a = mmap(0, page_sz, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (a == MAP_FAILED) {
                    perror("mmap() failed");
                    return EXIT_FAILURE;
            }

            /*
             * Enable MTE on the above anonymous mmap. The flag could be passed
             * directly to mmap() and skip this step.
             */
            if (mprotect(a, page_sz, PROT_READ | PROT_WRITE | PROT_MTE)) {
                    perror("mprotect() failed");
                    return EXIT_FAILURE;
            }

            /* access with the default tag (0) */
            a[0] = 1;
            a[1] = 2;

            printf("a[0] = %hhu a[1] = %hhu\n", a[0], a[1]);

            printf("origin tag:%d\n", mte_get_mem_tag(a));
            printf("origin address a = %p\n", a);

            /* set the logical and allocation tags */
            a = (unsigned char *)insert_random_tag(a);
            printf("after insert key: %p\n", a);
            set_tag(a);

            printf("after set tag: %p, tag:%d\n", a, mte_get_mem_tag(a));

            /* non-zero tag access */
            a[0] = 3;
            printf("a[0] = %hhu a[1] = %hhu\n", a[0], a[1]);

	    b = a + 15;
            printf("a[15] tag:%d\n", mte_get_mem_tag(b));

	    b = a + 16;
            printf("a[16] tag:%d\n", mte_get_mem_tag(b));

            /*
             * If MTE is enabled correctly the next instruction will generate an
             * exception.
             */
            printf("Expecting SIGSEGV...\n");
            a[16] = 0xdd;

            /* this should not be printed in the PR_MTE_TCF_SYNC mode */
            printf("...haven't got one\n");

            return EXIT_FAILURE;
    }
