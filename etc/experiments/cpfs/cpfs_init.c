/**
 *
 * CPFS
 *
 * Copyright (C) 2016-2016 Dmitry Zavalishin, dz@dz.ru
 *
 * Init/shutdown.
 *
 *
**/

#include "cpfs.h"
#include "cpfs_local.h"

// Just init - mutexes, memory, etc
errno_t
cpfs_init( cpfs_fs_t *fs )
{
    errno_t rc;

    cpfs_assert( sizeof(struct cpfs_dir_entry) <= CPFS_DIR_REC_SIZE );

    // TODO deinit what is partially inited on fail

    rc = cpfs_fdmap_init();
    if( rc ) return rc;

    rc = cpfs_buf_init( fs );
    if( rc ) return rc;

    rc = cpfs_init_sb( fs );
    if( rc ) return rc;

    fs->inited = 1;
    fs->fic_used = 0;
    fs->last_ino_blk = -1;

    cpfs_mutex_init( &(fs->freelist_mutex) );
    cpfs_mutex_init( &(fs->fic_mutex) );
    cpfs_mutex_init( &(fs->buf_mutex) );


    return 0;
}

// Mount
errno_t
cpfs_mount( cpfs_fs_t *fs )
{
    errno_t rc;

    rc = cpfs_mount_sb( fs );
    if( rc ) return rc;

    fs->sb.dirty = 0xFF; // Next write will update it on disk, we don't need to mark disk dirty if we never had a reason to write sb
    fic_refill( fs ); // fill list of free inodes

    return 0;
}

errno_t
cpfs_umount( cpfs_fs_t *fs )
{
    errno_t rc;

    // TODO check locked inodes

    cpfs_clear_all_buf( fs );

    fs->sb.dirty = 0;
    rc = cpfs_write_sb( fs );

    if( !rc )
    {
        fs->mounted = 0;
    }
    else
        fs->sb.dirty = 1; // failed to write superblock


    return rc;
}




errno_t cpfs_stop( cpfs_fs_t *fs )
{
    errno_t rc;

    if(fs->mounted)
    {
        rc = cpfs_umount( fs );
        if( rc ) return rc;
    }

    fs->inited = 0;
    return 0;
}

