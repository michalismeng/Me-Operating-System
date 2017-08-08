#ifndef MEMORY_DEFINITIONS_H_28072017
#define MEMORY_DEFINITIONS_H_28072017

#define MAP_FAILED -1

// protection level flags for the mmap function
enum MMAP_PROT
{
	PROT_NONE = 0,		// Pages are not accesible
	PROT_EXEC = 1,		// Pages can be executed (cannot be checkd by the intel cpu)
	PROT_READ = 2,		// Pages can be read
	PROT_WRITE = 4,		// Pages can be written
	PROT_USER = 8		// Pages can be accessed (by the above restrictions) by the user
};

enum MMAP_FLAGS
{
	/* protection flags */
	MMAP_NO_ACCESS = 0,
	MMAP_EXEC = 1 << 0,
	MMAP_READ = 1 << 1,
	MMAP_WRITE = 1 << 2,
	MMAP_USER = 1 << 3,

	MMAP_PROTECTION = 0xF,

	MMAP_SHARED = 1 << 5,
	MMAP_PRIVATE = 1 << 6,

	MMAP_ANONYMOUS = 1 << 7,
	MMAP_FIXED = 1 << 8,
	MMAP_GROWS_DOWN = 1 << 9,
	MMAP_LOCKED = 1 << 10,
	MMAP_NORESERVE = 1 << 11,
	MAP_UNINITIALIZED = 1 << 12,

	MMAP_NON_REMOVE = 1 << 13,
	MMAP_INVALID = 1 << 14,
};

#endif