/*
 * Copyright (c) 2011 Frantisek Princ
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libext4
 * @{
 */ 

/**
 * @file	libext4_directory.c
 * @brief	TODO
 */

#include <byteorder.h>
#include <errno.h>
#include "libext4.h"

static int ext4_directory_iterator_set(ext4_directory_iterator_t *,
    uint32_t);


uint32_t ext4_directory_entry_ll_get_inode(ext4_directory_entry_ll_t *de)
{
	return uint32_t_le2host(de->inode);
}

uint16_t ext4_directory_entry_ll_get_entry_length(
    ext4_directory_entry_ll_t *de)
{
	return uint16_t_le2host(de->entry_length);
}

uint16_t ext4_directory_entry_ll_get_name_length(
    ext4_superblock_t *sb, ext4_directory_entry_ll_t *de)
{
	if (ext4_superblock_get_rev_level(sb) == 0 &&
	    ext4_superblock_get_minor_rev_level(sb) < 5) {
		return ((uint16_t)de->name_length_high) << 8 |
		    ((uint16_t)de->name_length);
	}
	return de->name_length;
}

uint8_t ext4_directory_dx_root_info_get_hash_version(ext4_directory_dx_root_info_t *root_info)
{
	return root_info->hash_version;
}

uint8_t ext4_directory_dx_root_info_get_info_length(ext4_directory_dx_root_info_t *root_info)
{
	return root_info->info_length;
}

uint8_t ext4_directory_dx_root_info_get_indirect_levels(ext4_directory_dx_root_info_t *root_info)
{
	return root_info->indirect_levels;
}

uint16_t ext4_directory_dx_countlimit_get_limit(ext4_directory_dx_countlimit_t *countlimit)
{
	return uint16_t_le2host(countlimit->limit);
}
uint16_t ext4_directory_dx_countlimit_get_count(ext4_directory_dx_countlimit_t *countlimit)
{
	return uint16_t_le2host(countlimit->count);
}

uint32_t ext4_directory_dx_entry_get_hash(ext4_directory_dx_entry_t *entry)
{
	return uint32_t_le2host(entry->hash);
}

uint32_t ext4_directory_dx_entry_get_block(ext4_directory_dx_entry_t *entry)
{
	return uint32_t_le2host(entry->block);
}



int ext4_directory_iterator_init(ext4_directory_iterator_t *it,
    ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, aoff64_t pos)
{
	it->inode_ref = inode_ref;
	it->fs = fs;
	it->current = NULL;
	it->current_offset = 0;
	it->current_block = NULL;

	return ext4_directory_iterator_seek(it, pos);
}


int ext4_directory_iterator_next(ext4_directory_iterator_t *it)
{
	uint16_t skip;

	assert(it->current != NULL);

	skip = ext4_directory_entry_ll_get_entry_length(it->current);

	return ext4_directory_iterator_seek(it, it->current_offset + skip);
}


int ext4_directory_iterator_seek(ext4_directory_iterator_t *it, aoff64_t pos)
{
	int rc;

	uint64_t size;
	aoff64_t current_block_idx;
	aoff64_t next_block_idx;
	uint32_t next_block_phys_idx;
	uint32_t block_size;

	size = ext4_inode_get_size(it->fs->superblock, it->inode_ref->inode);

	/* The iterator is not valid until we seek to the desired position */
	it->current = NULL;

	/* Are we at the end? */
	if (pos >= size) {
		if (it->current_block) {
			rc = block_put(it->current_block);
			it->current_block = NULL;
			if (rc != EOK) {
				return rc;
			}
		}

		it->current_offset = pos;
		return EOK;
	}

	block_size = ext4_superblock_get_block_size(it->fs->superblock);
	current_block_idx = it->current_offset / block_size;
	next_block_idx = pos / block_size;

	/* If we don't have a block or are moving accross block boundary,
	 * we need to get another block
	 */
	if (it->current_block == NULL || current_block_idx != next_block_idx) {
		if (it->current_block) {
			rc = block_put(it->current_block);
			it->current_block = NULL;
			if (rc != EOK) {
				return rc;
			}
		}

		rc = ext4_filesystem_get_inode_data_block_index(it->fs,
		    it->inode_ref->inode, next_block_idx, &next_block_phys_idx);
		if (rc != EOK) {
			return rc;
		}

		rc = block_get(&it->current_block, it->fs->device, next_block_phys_idx,
		    BLOCK_FLAGS_NONE);
		if (rc != EOK) {
			it->current_block = NULL;
			return rc;
		}
	}

	it->current_offset = pos;

	return ext4_directory_iterator_set(it, block_size);
}

static int ext4_directory_iterator_set(ext4_directory_iterator_t *it,
    uint32_t block_size)
{
	uint32_t offset_in_block = it->current_offset % block_size;

	it->current = NULL;

	/* Ensure proper alignment */
	if ((offset_in_block % 4) != 0) {
		return EIO;
	}

	/* Ensure that the core of the entry does not overflow the block */
	if (offset_in_block > block_size - 8) {
		return EIO;
	}

	ext4_directory_entry_ll_t *entry = it->current_block->data + offset_in_block;

	/* Ensure that the whole entry does not overflow the block */
	uint16_t length = ext4_directory_entry_ll_get_entry_length(entry);
	if (offset_in_block + length > block_size) {
		return EIO;
	}

	/* Ensure the name length is not too large */
	if (ext4_directory_entry_ll_get_name_length(it->fs->superblock,
	    entry) > length-8) {
		return EIO;
	}

	it->current = entry;
	return EOK;
}


int ext4_directory_iterator_fini(ext4_directory_iterator_t *it)
{
	int rc;

	it->fs = NULL;
	it->inode_ref = NULL;
	it->current = NULL;

	if (it->current_block) {
		rc = block_put(it->current_block);
		if (rc != EOK) {
			return rc;
		}
	}

	return EOK;
}

int ext4_directory_dx_find_entry(ext4_directory_iterator_t *it,
		ext4_filesystem_t *fs, ext4_inode_ref_t *inode_ref, size_t len, const char *name)
{
	int rc;
	uint32_t fblock;
	block_t *phys_block;
	ext4_directory_dx_root_t *root;
	uint32_t hash;
	ext4_hash_info_t hinfo;

	// get direct block 0 (index root)
	rc = ext4_filesystem_get_inode_data_block_index(fs, inode_ref->inode, 0, &fblock);
	if (rc != EOK) {
		return rc;
	}

	rc = block_get(&phys_block, fs->device, fblock, BLOCK_FLAGS_NONE);
	if (rc != EOK) {
		it->current_block = NULL;
			return rc;
	}

	// Now having index root
	root = (ext4_directory_dx_root_t *)phys_block->data;

	// Check hash version - only if supported
	EXT4FS_DBG("hash_version = \%u", root->info.hash_version);

	// Check unused flags
	if (root->info.unused_flags != 0) {
		EXT4FS_DBG("ERR: unused_flags = \%u", root->info.unused_flags);
		block_put(phys_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	// Check indirect levels
	if (root->info.indirect_levels > 1) {
		EXT4FS_DBG("ERR: indirect_levels = \%u", root->info.indirect_levels);
		block_put(phys_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	uint32_t bs = ext4_superblock_get_block_size(fs->superblock);

	uint32_t entry_space = bs - 2* sizeof(ext4_directory_dx_dot_entry_t) - sizeof(ext4_directory_dx_root_info_t);
    entry_space = entry_space / sizeof(ext4_directory_dx_entry_t);


    uint32_t limit = ext4_directory_dx_countlimit_get_limit((ext4_directory_dx_countlimit_t *)&root->entries);
    uint32_t count = ext4_directory_dx_countlimit_get_count((ext4_directory_dx_countlimit_t *)&root->entries);

    if (limit != entry_space) {
		block_put(phys_block);
    	return EXT4_ERR_BAD_DX_DIR;
	}

	if ((count == 0) || (count > limit)) {
		block_put(phys_block);
		return EXT4_ERR_BAD_DX_DIR;
	}

	/* DEBUG list
    for (uint16_t i = 0; i < count; ++i) {
    	uint32_t hash = ext4_directory_dx_entry_get_hash(&root->entries[i]);
    	uint32_t block = ext4_directory_dx_entry_get_block(&root->entries[i]);
    	EXT4FS_DBG("hash = \%u, block = \%u", hash, block);
    }
    */

	hinfo.hash_version = ext4_directory_dx_root_info_get_hash_version(&root->info);
	if ((hinfo.hash_version <= EXT4_HASH_VERSION_TEA)
			&& (ext4_superblock_has_flag(fs->superblock, EXT4_SUPERBLOCK_FLAGS_UNSIGNED_HASH))) {
		// 3 is magic from ext4 linux implementation
		hinfo.hash_version += 3;
	}

	hinfo.seed = ext4_superblock_get_hash_seed(fs->superblock);
	hinfo.hash = 0;
	if (name) {
		ext4_hash_string(&hinfo, len, name);
	}

	hash = hinfo.hash;

	ext4_directory_dx_entry_t *p, *q, *m;

	// TODO cycle
	// while (true)

		p = &root->entries[1];
		q = &root->entries[count - 1];

		while (p <= q) {
			m = p + (q - p) / 2;
			if (ext4_directory_dx_entry_get_hash(m) > hash) {
				q = m - 1;
			} else {
				p = m + 1;
			}
		}

		/* TODO move to leaf or next node
		at = p - 1;
		dxtrace(printk(" %x->%u\n", at == entries? 0: dx_get_hash(at), dx_get_block(at)));
        frame->bh = bh;
        frame->entries = entries;
        frame->at = at;

        if (indirect == 0) {
			// TODO write return values !!!
        	return EOK;
        }

        indirect--;

		// TODO read next block
		if (!(bh = ext4_bread (NULL,dir, dx_get_block(at), 0, err)))
                        goto fail2;
		at = entries = ((struct dx_node *) bh->b_data)->entries;
		if (dx_get_limit(entries) != dx_node_limit (dir)) {
        	ext4_warning(dir->i_sb, "dx entry: limit != node limit");
			brelse(bh);
            *err = ERR_BAD_DX_DIR;
			goto fail2;
		}
		frame++;
		frame->bh = NULL;
		 */

	// } END WHILE


	// TODO delete it !!!
	return EXT4_ERR_BAD_DX_DIR;

	if ((it->current == NULL) || (it->current->inode == 0)) {
			return ENOENT;
	}

	return EOK;
}


/**
 * @}
 */ 
