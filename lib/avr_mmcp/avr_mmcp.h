#include <diskio.h>

DSTATUS disk_initialize (void);
DRESULT disk_readp (
	BYTE *buff,		/* Pointer to the read buffer (NULL:Forward to the stream) */
	DWORD sector,	/* Sector number (LBA) */
	UINT offset,	/* Byte offset to read from (0..511) */
	UINT count		/* Number of bytes to read (ofs + cnt mus be <= 512) */
);
DRESULT disk_writep (
	const BYTE *buff,	/* Pointer to the bytes to be written (NULL:Initiate/Finalize sector write) */
	DWORD sc			/* Number of bytes to send, Sector number (LBA) or zero */
);
static BYTE send_cmd (
	BYTE cmd,		/* 1st byte (Start + Index) */
	DWORD arg		/* Argument (32 bits) */
);