#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>

typedef uint64_t seed_type;
static __thread seed_type master_seed;

 double Random(void) {
	uint32_t *seed1;
	uint32_t *seed2;
	fprintf(stderr,"1r\n");

	
		master_seed = random();
	
	fprintf(stderr,"2r\n");

	seed1 = (uint32_t *)&master_seed;
	seed2 = (uint32_t *)((char *)&master_seed + (sizeof(uint32_t)));
	fprintf(stderr,"3r\n");

	*seed1 = 36969u * (*seed1 & 0xFFFFu) + (*seed1 >> 16u);
	*seed2 = 18000u * (*seed2 & 0xFFFFu) + (*seed2 >> 16u);
		fprintf(stderr,"4r \n");

	// The result is strictly between 0 and 1.
	return (((*seed1 << 16u) + (*seed1 >> 16u) + *seed2) + 1.0) * 2.328306435454494e-10;
}
int main()
{

	double d=20*Random();
	fprintf(stderr,"5r %f \n",d);

	
}
