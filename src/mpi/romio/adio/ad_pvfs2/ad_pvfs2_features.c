int ADIOI_PVFS2_Features(int flag)
{
	switch(flag) {
		case ADIOI_SHARED_FILE_PTR:
		case ADIOI_NEEDS_LOCKING:
		case ADIOI_SEQUENTIAL:

		default:
			return 0;
	}
}
