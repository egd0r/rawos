#define PAGE_DIR_VIRT 0xFFFFFFFFC0000000 // Last entry in L3 maps to L4, indexing this address gives PDEs


void get_physaddr(void *virt_addr) {                         //0xFFFFFFFFC0000000
    // unsigned long pdindex = (unsigned long)virt_addr >> 39   & 0x0000FF8000000000;
    // unsigned long ptl3index = (unsigned long)virt_addr >> 30 & 0x0000007FC0000000;
    // unsigned long ptl2index = (unsigned long)virt_addr >> 21 & 0x000000002FE00000;
    // unsigned long ptl1index = (unsigned long)virt_addr >> 12 & 0x00000000001FF000;

    unsigned long pdindex = ((unsigned long)virt_addr & 0x0000FF8000000000) >> 39;
    unsigned long ptl3index = ((unsigned long)virt_addr & 0x0000007FC0000000) >> 30;
    unsigned long ptl2index = ((unsigned long)virt_addr & 0x000000002FE00000) >> 21;
    unsigned long ptl1index = ((unsigned long)virt_addr & 0x00000000001FF000) >> 12;

    // Extract metadata from each index into next page    

    // Have to check hugepage flags

}