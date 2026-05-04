#ifndef FUSE_ANDROID_FUSE_CONFIG_H
#define FUSE_ANDROID_FUSE_CONFIG_H

#define PACKAGE_VERSION "3.18.2"
#define FUSERMOUNT_DIR "/system/bin"
#define FUSE_CONF "/etc/fuse.conf"

/*
 * Keep these disabled unless the Android recovery libc/kernel side
 * explicitly supports the newer desktop Linux helpers.
 */
#undef HAVE_CLOSE_RANGE
#undef HAVE_LISTMOUNT
#undef GETMNTENT_NEEDS_UNESCAPING

#endif
