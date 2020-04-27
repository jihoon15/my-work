/* File system header (FS.H) */

/**
	Some Define values
*/
#define SIMPLE_PARTITION	0x1111

#define INVALID_INODE			0
#define INODE_MODE_AC_ALL		0x777
#define INODE_MODE_AC_USER_R	0x001
#define INODE_MODE_AC_USER_W	0x002
#define INODE_MODE_AC_USER_X	0x004
#define INODE_MODE_AC_OTHER_R	0x010
#define INODE_MODE_AC_OTHER_W	0x020
#define INODE_MODE_AC_OTHER_X	0x040
#define INODE_MODE_AC_GRP_R		0x100
#define INODE_MODE_AC_GRP_W		0x200
#define INODE_MODE_AC_GRP_X		0x400

#define INODE_MODE_REG_FILE		0x10000
#define INODE_MODE_DIR_FILE		0x20000
#define INODE_MODE_DEV_FILE		0x40000

#define DENTRY_TYPE_REG_FILE	0x1
#define DENTRY_TYPE_DIR_FILE	0x2

#define BLOCK_SIZE				0x400
/**
  Partition structure
	ASSUME: data block size: 1K

	partition: 4MB

	Superblock: 1KB
	Inode table (32 bytes inode array) * 224 entries = 7KB
	data blocks: 1KB blocks array (~4K)
*/

/**
  Super block structure
*/
struct super_block {
	unsigned int partition_type;
	unsigned int block_size;
	unsigned int inode_size;
	unsigned int first_inode;

	unsigned int num_inodes;
	unsigned int num_inode_blocks;
	unsigned int num_free_inodes;

	unsigned int num_blocks;
	unsigned int num_free_blocks;
	unsigned int first_data_block;
	char volume_name[24];
	unsigned char padding[960]; //1024-64
};

/**
  32 byte I-node structure
*/
struct inode {
	unsigned int mode; 		// reg. file, directory, dev., permissions
	unsigned int locked; 	// opened for write
	unsigned int date;
	unsigned int size;
	int indirect_block; 	// N.B. -1 for NULL
	unsigned short blocks[0x6];
};

struct blocks {
	unsigned char d[1024];
};

/* physical partition structure */
struct META_DATA {
	struct super_block s;
	struct inode inode_table[224];

};

/**
  Directory entry structure
*/
struct dentry {
	unsigned int inode;
	unsigned int dir_length;
	unsigned int name_len;
	unsigned int file_type;
	union { // name
		unsigned char name[256];
		unsigned char n_pad[16][16];
	};
};


