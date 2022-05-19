void *memmove (void *dest, void *src, int n) {
    if(dest == src || n < 0) {
        return dest;
    }

    if((int)src < dest &&(int)src + n > (int)dest) {
        int r_overlap = (int)dest - (int)src;
        int overlap = n - r_overlap;
        memcpy((void *)((int)dest + r_overlap), dest, overlap);
        return memcpy(dest, src, r_overlap);
    } else{
        return memcpy(dest, src, n);
    }
}