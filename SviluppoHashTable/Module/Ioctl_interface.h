#define IOCTL_MAGIC 'f'

#define FIB_FLS_ALLOC _IOR(IOCTL_MAGIC, 10, struct fiber_arguments* )
#define FIB_FLS_GET	_IOWR(IOCTL_MAGIC, 2, struct fiber_arguments* )
#define FIB_FLS_SET _IOW(IOCTL_MAGIC, 3, struct fiber_arguments* )
#define FIB_FLS_DEALLOC	_IOW(IOCTL_MAGIC, 4, struct fiber_arguments* )
#define FIB_CONVERT	_IO(IOCTL_MAGIC, 5)
#define FIB_CREATE _IOWR(IOCTL_MAGIC, 6, struct fiber_arguments* )
#define FIB_SWITCH_TO _IOW(IOCTL_MAGIC, 7, struct fiber_arguments* )
