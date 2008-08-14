#include "adio.h"

int ADIOI_PVFS2_Features(int flag)
{
	switch(flag) {
		case ADIO_SCALABLE_OPEN:
			return 1;
		case ADIO_SHARED_FP:
		case ADIO_LOCKS:
		case ADIO_SEQUENTIAL:
		case ADIO_DATA_SIEVING_WRITES:
		default:
			return 0;
	}
}
