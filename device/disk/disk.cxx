#include <xinu.h>

#define LBA_MODE    0xe0
#define REQUEST     0x08
#define READY       0x40
#define BUSY        0x80
#define SECTOR_SIZE 512
struct DiskBuffer {
    bool cache;
    uint32 sector, offset;
    char buffer[SECTOR_SIZE];
} diskBuffer;
extern "C"
{

void setSectorNum(uint32 sector) {
    outb(0x1f3, sector & 0xff);
    outb(0x1f4, (sector >> 8) & 0xff);
    outb(0x1f5, (sector >> 16) & 0xff); 
    outb(0x1f6, (sector >> 24) & 0x0f | LBA_MODE);
}

devcall	diskread(
	  struct dentry	*devptr,	/* Entry in device switch table	*/
	  char	*buff,			    /* Buffer of characters		*/
	  int32	count 			    /* Count of character to read	*/
	)
{
    uint32 cnt;
    for(cnt = 0; cnt < count; cnt++) {
        buff[cnt] = diskgetc(devptr);
    }
    return cnt;
}

devcall	diskgetc(
	  struct dentry	*devptr		/* Entry in device switch table	*/
    )
{
    if(diskBuffer.cache == 0) {
        memset(diskBuffer.buffer, 0, SECTOR_SIZE);
        outb(0x1f2, 1);       // Port to send number of sectors
        setSectorNum(diskBuffer.sector);
        outb(0x1f7, 0x20);    // Send read command
        /*  the sector buffer requires servicing until the sector buffer is ready. */
        uint32 flag;
        while(!((flag = inb(0x1f7)) & REQUEST)) {
        }
        int cnt = 0;
        while(cnt < 512) {
            uint32 data = (uint32)inl(0x1f0);
            diskBuffer.buffer[cnt++] = data&0xff;
            diskBuffer.buffer[cnt++] = (data>>8)&0xff;
            diskBuffer.buffer[cnt++] = (data>>16)&0xff;
            diskBuffer.buffer[cnt++] = (data>>24)&0xff;
        }
        diskBuffer.cache = 1;
    }
    assert(diskBuffer.cache);
    char ch = diskBuffer.buffer[diskBuffer.offset++];
    if(diskBuffer.offset >= 512) {
        diskBuffer.offset = 0;
        diskBuffer.cache = 0;
        diskBuffer.sector++;
    }
    return ch;
}   

devcall	diskseek (
	  struct dentry *devptr,	/* Entry in device switch table */
	  uint32	offset		    /* Byte position in the file	*/
	)
{
    diskBuffer.cache = 0;
    diskBuffer.offset = offset % SECTOR_SIZE;
    diskBuffer.sector = offset / SECTOR_SIZE;
    return OK;
}

}
