/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/3_01.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: R. Christian McDonald <cmcdonald@netgate.com                 |
   +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

/* includes for php */
#include "php.h"
#include "ext/standard/info.h"
#include "php_libbe.h"

/* includes for libbe */
#include <sys/param.h>
#include <be.h>

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

static int le_libbe;

char *root = NULL;
size_t root_len;

/* libbe handle destructor callback */
static
ZEND_RSRC_DTOR_FUNC(libbe_handle_dtor)
{
	libbe_handle_t *be = (libbe_handle_t *)rsrc->ptr;
	if (be)
		libbe_close(be)
}

PHP_MINIT_FUNCTION(libbe)
{	
	le_libbe = zend_register_list_destructors_ex(libbe_handle_dtor, NULL,
			le_libbe_name, module_number);

	/* BE_ERR_* constants */
	REGISTER_LIBBE_CONSTANT(BE_ERR_SUCCESS);
	REGISTER_LIBBE_CONSTANT(BE_ERR_INVALIDNAME);
	REGISTER_LIBBE_CONSTANT(BE_ERR_EXISTS);
	REGISTER_LIBBE_CONSTANT(BE_ERR_NOENT);
	REGISTER_LIBBE_CONSTANT(BE_ERR_PERMS);
	REGISTER_LIBBE_CONSTANT(BE_ERR_DESTROYACT);
	REGISTER_LIBBE_CONSTANT(BE_ERR_DESTROYMNT);
	REGISTER_LIBBE_CONSTANT(BE_ERR_BADPATH);
	REGISTER_LIBBE_CONSTANT(BE_ERR_PATHBUSY);
	REGISTER_LIBBE_CONSTANT(BE_ERR_PATHLEN);
	REGISTER_LIBBE_CONSTANT(BE_ERR_BADMOUNT);
	REGISTER_LIBBE_CONSTANT(BE_ERR_NOORIGIN);
	REGISTER_LIBBE_CONSTANT(BE_ERR_MOUNTED);
	REGISTER_LIBBE_CONSTANT(BE_ERR_NOMOUNT);
	REGISTER_LIBBE_CONSTANT(BE_ERR_ZFSOPEN);
	REGISTER_LIBBE_CONSTANT(BE_ERR_ZFSCLONE);
	REGISTER_LIBBE_CONSTANT(BE_ERR_IO);
	REGISTER_LIBBE_CONSTANT(BE_ERR_NOPOOL);
	REGISTER_LIBBE_CONSTANT(BE_ERR_NOMEM);
	REGISTER_LIBBE_CONSTANT(BE_ERR_UNKNOWN);
	REGISTER_LIBBE_CONSTANT(BE_ERR_INVORIGIN);
	REGISTER_LIBBE_CONSTANT(BE_ERR_HASCLONES);

	/* BE_DESTROY_* constants */
	REGISTER_LIBBE_CONSTANT(BE_DESTROY_FORCE);
	REGISTER_LIBBE_CONSTANT(BE_DESTROY_ORIGIN);
	REGISTER_LIBBE_CONSTANT(BE_DESTROY_AUTOORIGIN);

	/* BE_MNT_* constants */
	REGISTER_LIBBE_CONSTANT(BE_MNT_FORCE);
	REGISTER_LIBBE_CONSTANT(BE_MNT_DEEP);

	return SUCCESS;
}

/* {{{
Takes an optional BE root and initializes libbe.
 */
PHP_FUNCTION(libbe_init)
{
	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(0, 1)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING(root, root_len);
	ZEND_PARSE_PARAMETERS_END();

	if ((be = libbe_init(root)) == NULL) {
		php_error_docref(NULL, E_WARNING, "libbe: could not initialize a new libbe handle");
		RETURN_FALSE;
	}

	RETURN_RES(zend_register_resource((void *)be, le_libbe));
}
/* }}} */

/* {{{
Function frees all resources previously acquired in libbe_init(), invalidating
the handle in the process.
 */
PHP_FUNCTION(libbe_close)
{
	zval *zhdl;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	/* force call the resource destructor whatever the conditions */
	zend_list_close(Z_RES_P(zhdl));
}
/* }}} */

/* {{{
Function refreshes the libbe handle and data by requesting a new one.
 */
PHP_FUNCTION(libbe_refresh)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ZVAL_DEREF(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	/* force the resource destructor to be called */
	zend_list_close(Z_RES_P(zhdl));

	/* use the same root from libbe_init() */
	if ((be = libbe_init(root)) == NULL) {
		php_error_docref(NULL, E_WARNING, "libbe: could not initialize a new libbe handle");
		RETURN_FALSE;
	}

	/* now we attach the new libbe handle to the resource, returned by reference */
	ZVAL_RES(zhdl, zend_register_resource((void *)be, le_libbe));
	RETURN_TRUE;
}
/* }}} */

/* {{{
Returns the name of the currently booted boot environment.
 */
PHP_FUNCTION(be_active_name)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	ZEND_ASSERT(Z_TYPE_P(zhdl) == IS_RESOURCE);

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_STRING(be_active_name(be))
}
/* }}} */

/* {{{
Returns the full path of the currently booted boot environment.
 */
PHP_FUNCTION(be_active_path)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_STRING(be_active_path(be))	
}
/* }}} */

/* {{{
Returns the name of the boot environment that will be active on reboot.
 */
PHP_FUNCTION(be_nextboot_name)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_STRING(be_nextboot_name(be))	
}
/* }}} */

/* {{{
Returns the full path of the boot environment that will be active on reboot.
 */
PHP_FUNCTION(be_nextboot_path)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_STRING(be_nextboot_path(be))	
}
/* }}} */

/* {{{
Returns the boot environment root path.
 */
PHP_FUNCTION(be_root_path)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_STRING(be_root_path(be))	
}
/* }}} */

/* {{{
Creates a snapshot of be_name named snap_name.
A NULL snap_name may be used, indicating that be_snaphot() should derive
the snapshot name from the current date and time. If recursive is set,
then be_snapshot() will recursively snapshot the dataset. If result is
not NULL, then it will be populated with the final "be_name@snap_name".
 */
PHP_FUNCTION(be_snapshot)
{
	zval *zhdl;
	char *be_name, *snap_name = NULL;
	size_t be_name_len, snap_name_len;
	zend_bool recursive = false;

	libbe_handle_t *be;
	char snapshot[BE_MAXPATHLEN];
	int err;

	ZEND_PARSE_PARAMETERS_START(2, 4)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING(snap_name, snap_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_BOOL(recursive)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	if ((err = be_snapshot(be, be_name, snap_name, recursive, snapshot)) != BE_ERR_SUCCESS)
		RETURN_LONG(err);

	RETURN_STRING(snapshot);
}
/* }}} */

/* {{{
Determine if the gives snapshot name matches the format that the be_snapshot()
function will use by default if it is not given a snapshot name to use.
 */
PHP_FUNCTION(be_is_auto_snapshot_name)
{
	zval *zhdl;
	char *snap;
	size_t snap_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(snap, snap_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;
	
	RETURN_BOOL(be_is_auto_snapshot_name(be, snap));
}
/* }}} */

/* {{{
Function creates a boot environment with the given name. The new boot environment
will be created from a recursive snapshot of the currently booted boot environment.
 */
PHP_FUNCTION(be_create)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_create(be, be_name));
}
/* }}} */

/* {{{
Function creates a boot environment with the given name from an existing snapshot.
The depth parameter specifies the depth of recursion that will be cloned from the
existing snapshot. A depth of '0' is no recursion and '-1' is unlimited (i.e., a
recursive boot environment).
 */
PHP_FUNCTION(be_create_depth)
{
	zval *zhdl;
	char *be_name, *snap;
	size_t be_name_len, snap_len;
	zend_long depth = 0;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(3, 4)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_STRING(snap, snap_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(depth)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_create_depth(be, be_name, snap, depth));
}
/* }}} */

/* {{{
Function creates a boot environment with the given name from the name of an existing
boot environment. A recursive snapshot will be made of the origin boot environment,
and the new boot environment will be created from that.
 */
PHP_FUNCTION(be_create_from_existing)
{
	zval *zhdl;
	char *be_name, *be_origin;
	size_t be_name_len, be_origin_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_STRING(be_origin, be_origin_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_create_from_existing(be, be_name, be_origin));
}
/* }}} */

/* {{{
Function creates a recursive boot environment with the given name from an existing snapshot.
 */
PHP_FUNCTION(be_create_from_existing_snap)
{
	zval *zhdl;
	char *be_name, *snap;
	size_t be_name_len, snap_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_STRING(snap, snap_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_create_from_existing_snap(be, be_name, snap));
}
/* }}} */

/* {{{
Renames a boot environment without unmounting it, as if renamed with the -u argument
were passed to zfs rename.
 */
PHP_FUNCTION(be_rename)
{
	zval *zhdl;
	char *be_old, *be_new;
	size_t be_old_len, be_new_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_old, be_old_len)
		Z_PARAM_STRING(be_new, be_new_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_rename(be, be_old, be_new));
}
/* }}} */

/* {{{
Function makes a boot environment active on the next boot. If the temporary
flag is set, then it will be active for the next boot only, as done by zfsbootcfg(8)
 */
PHP_FUNCTION(be_activate)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;
	zend_bool temporary = false;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_BOOL(temporary)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_activate(be, be_name, temporary));
}
/* }}} */

/* {{{
Function deactivates a boot environment. If the temporary flag is set, then it will
cause removal of boot once configuration, set by be_activate() function or by zfsbootcfg(8).
If the temporary flag is not set, be_deactivate() function will set zfs canmount property to noauto.
 */
#if __FreeBSD_version >= 1300000
PHP_FUNCTION(be_deactivate)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;
	zend_bool temporary = false;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_BOOL(temporary)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_deactivate(be, be_name, temporary));
}
#endif
/* }}} */

/* {{{
Function will recursively destroy the given boot environment. It will not destroy a mounted
boot environment unless the BE_DESTROY_FORCE option is set in options. If the BE_DESTROY_ORIGIN
option is set in options, the be_destroy() function will destroy the origin snapshot to this
boot environment as well.
 */
PHP_FUNCTION(be_destroy)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;
	zend_long options = 0;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(options)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_destroy(be, be_name, options));
}
/* }}} */

/* {{{
Function will format name in a traditional ZFS humanized format, similar to humanize_number(3).
This function effectively proxies zfs_nicenum() from libzfs.
 */
PHP_FUNCTION(be_nicenum)
{
	zend_long num;
	char buf[6];

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_LONG(num)
	ZEND_PARSE_PARAMETERS_END();

	be_nicenum(num, buf, sizeof (buf));

	RETURN_STRING(buf);
}
/* }}} */

/* {{{
Function will mount the given boot environment. If mountpoint is NULL, a mount point will be
generated in /tmp using mkdtemp(3). If result is not NULL, it should be large enough to accommodate
BE_MAXPATHLEN including the null terminator. The final mount point will be copied into it. Setting 
the BE_MNT_FORCE flag will pass MNT_FORCE to the underlying mount(2) call.
 */
PHP_FUNCTION(be_mount)
{
	zval *zhdl;
	char *be_name, *mntpoint = NULL;
	size_t be_name_len, mntpoint_len;
	zend_long flags = 0;

	libbe_handle_t *be;
	char result_loc[BE_MAXPATHLEN];
	int err;

	ZEND_PARSE_PARAMETERS_START(2, 4)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_STRING(mntpoint, mntpoint_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	if ((err = be_mount(be, be_name, mntpoint, flags, result_loc)) != BE_ERR_SUCCESS)
		RETURN_LONG(err);

	RETURN_STRING(result_loc);
}	
/* }}} */

/* {{{
Function will check if there is a boot environment mounted at the given path.
If details is not NULL, it will be populated with a list of the mounted dataset's properties.
This list of properties matches the properties collected by be_get_bootenv_props().
 */
PHP_FUNCTION(be_mounted_at)
{
	zval *zhdl;
	char *path;
	size_t path_len;

	libbe_handle_t *be;
	nvlist_t *bes, *curbeprops;
	nvpair_t *curbe, *curbeprop;
	zval zbeprops;
	char *propval;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(path, path_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	if (be_prop_list_alloc(&bes) != 0) {
		php_error_docref(NULL, E_WARNING, "libbe: failed to allocate beprops nvlist");
		RETURN_FALSE;
	}

	if (be_mounted_at(be, path, bes) != 0)
		RETURN_FALSE;

	array_init(return_value);
	for (curbe = nvlist_next_nvpair(bes, NULL);
		curbe != NULL; curbe = nvlist_next_nvpair(bes, curbe)) {
		nvpair_value_nvlist(curbe, &curbeprops);

		array_init(&zbeprops);
		for (curbeprop = nvlist_next_nvpair(curbeprops, NULL);
			curbeprop != NULL; curbeprop = nvlist_next_nvpair(curbeprops, curbeprop)) {
			if (nvpair_value_string(curbeprop, &propval) != 0) {
				continue;
			}

			add_assoc_string(&zbeprops, nvpair_name(curbeprop), propval);
		}
		add_assoc_zval(return_value, nvpair_name(curbe), &zbeprops);
	}

	be_prop_list_free(bes);
}
/* }}} */

/* {{{
Function will unmount the given boot environment. Setting the BE_MNT_FORCE flag will pass MNT_FORCE
to the underlying mount(2) call.
 */
PHP_FUNCTION(be_unmount)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;
	zend_long flags = 0;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_OPTIONAL
		Z_PARAM_LONG(flags)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_unmount(be, be_name, flags));
}	
/* }}} */

/* {{{
Function returns the libbe errno.
 */
PHP_FUNCTION(libbe_errno)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(libbe_errno(be));
}
/* }}} */

/* {{{
Function returns a string description of the currently set libbe errno.
 */
PHP_FUNCTION(libbe_error_description)
{
	zval *zhdl;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_STRING(libbe_error_description(be));
}
/* }}} */

/* {{{
Function will change whether or not libbe prints the description of any encountered error to
stderr, based on doprint.
 */
PHP_FUNCTION(libbe_print_on_error)
{
	zval *zhdl;
	zend_bool doprint;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_BOOL(doprint)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	libbe_print_on_error(be, doprint);
}
/* }}} */

/* {{{
Function will concatenate the boot environment root and the given boot environment name.
 */
PHP_FUNCTION(be_root_concat)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;

	libbe_handle_t *be;
	char targetds[BE_MAXPATHLEN];
	int err;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	if ((err = be_root_concat(be, be_name, targetds)) != BE_ERR_SUCCESS)
		RETURN_LONG(err);

	RETURN_STRING(targetds);
}
/* }}} */

/* {{{
Validate the given boot environment name for both length restrictions as well as
valid character restrictions.
 */
PHP_FUNCTION(be_validate_name)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	/* BE datasets with spaces are not bootable. */
	if (strchr(be_name, ' ') != NULL)
		RETURN_LONG(BE_ERR_INVALIDNAME)

	RETURN_LONG(be_validate_name(be, be_name));
}
/* }}} */

/* {{{
Validate the given snapshot name. The snapshot must have a valid name, exist, and
have a mountpoint of /.
 */
PHP_FUNCTION(be_validate_snap)
{
	zval *zhdl;
	char *snap;
	size_t snap_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(snap, snap_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_validate_snap(be, snap));
}
/* }}} */

/* {{{
Function will check whether the given boot environment exists and has a mountpoint
of /. This function does not set the internal library error state, but will return
the appropriate error.
 */
PHP_FUNCTION(be_exists)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	RETURN_LONG(be_exists(be, be_name));
}
/* }}} */

/* {{{
Function will export the given boot environment to the file specified by fd. A snapshot
will be created of the boot environment prior to export.
 */
PHP_FUNCTION(be_export)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;
	zval *zfd;

	libbe_handle_t *be;
	php_stream *stream;
	int fd;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_RESOURCE(zfd)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	php_stream_from_zval(stream, zfd);

	if (php_stream_can_cast(stream, PHP_STREAM_AS_FD) == SUCCESS) {
		php_stream_cast(stream, PHP_STREAM_AS_FD, (void**)&fd, REPORT_ERRORS);
		RETURN_LONG(be_export(be, be_name, fd));
	}

	RETURN_FALSE;
}	
/* }}} */

/* {{{
Function will import the boot environment in the file specified by fd, and give it the name be_name.
 */
PHP_FUNCTION(be_import)
{
	zval *zhdl;
	char *be_name;
	size_t be_name_len;
	zval *zfd;

	libbe_handle_t *be;
	php_stream *stream;
	int fd;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(be_name, be_name_len)
		Z_PARAM_RESOURCE(zfd)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	php_stream_from_zval(stream, zfd);

	if (php_stream_can_cast(stream, PHP_STREAM_AS_FD) == SUCCESS) {
		php_stream_cast(stream, PHP_STREAM_AS_FD, (void**)&fd, REPORT_ERRORS);
		RETURN_LONG(be_import(be, be_name, fd));
	}

	RETURN_FALSE;
}	
/* }}} */

/* {{{
Function will populate be_list with nvpair_t of boot environment names paired with an
nvlist_t of their properties.
 */
PHP_FUNCTION(be_get_bootenv_props)
{
	zval *zhdl;

	libbe_handle_t *be;
	nvlist_t *bes, *curbeprops;
	nvpair_t *curbe, *curbeprop;
	zval zbeprops;
	char *propval;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(zhdl)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

	if (be_prop_list_alloc(&bes) != 0) {
		php_error_docref(NULL, E_WARNING, "libbe: failed to allocate beprops nvlist");
		RETURN_FALSE;
	}

	if (be_get_bootenv_props(be, bes) != 0) {
		php_error_docref(NULL, E_WARNING, "libbe: failed to fetch boot environments");
		RETURN_FALSE;
	}

	array_init(return_value);
	for (curbe = nvlist_next_nvpair(bes, NULL);
		curbe != NULL; curbe = nvlist_next_nvpair(bes, curbe)) {
		nvpair_value_nvlist(curbe, &curbeprops);

		array_init(&zbeprops);
		for (curbeprop = nvlist_next_nvpair(curbeprops, NULL);
			curbeprop != NULL; curbeprop = nvlist_next_nvpair(curbeprops, curbeprop)) {
			if (nvpair_value_string(curbeprop, &propval) != 0)
				continue;

			add_assoc_string(&zbeprops, nvpair_name(curbeprop), propval);
		}
		add_assoc_zval(return_value, nvpair_name(curbe), &zbeprops);
	}

	be_prop_list_free(bes);
}

/* {{{
Function will get properties of the specified dataset. props is populated directly with a 
list of the properties as returned by be_get_bootenv_props()
 */
PHP_FUNCTION(be_get_dataset_props)
{
	zval *zhdl;
	char *ds_name;
	size_t ds_name_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(ds_name, ds_name_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;

}	
/* }}} */

/* {{{
Function will retrieve all snapshots of the given dataset. snap_list will be populated with
a list of nvpair_t exactly as specified by be_get_bootenv_props()
 */
PHP_FUNCTION(be_get_dataset_snapshots)
{
	zval *zhdl;
	char *ds_name;
	size_t ds_name_len;

	libbe_handle_t *be;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(zhdl)
		Z_PARAM_STRING(ds_name, ds_name_len)
	ZEND_PARSE_PARAMETERS_END();

	if ((be = (libbe_handle_t *)zend_fetch_resource(Z_RES_P(zhdl), le_libbe_name, le_libbe)) == NULL)
		RETURN_FALSE;
}	
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(libbe)
{
#if defined(ZTS) && defined(COMPILE_DL_LIBBE)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(libbe)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "libbe support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ arginfo
 */
ZEND_BEGIN_ARG_INFO(arginfo_libbe_init, 0)
	ZEND_ARG_INFO(0, root)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_libbe_close, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_libbe_refresh, 0)
	ZEND_ARG_INFO(1, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_active_name, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_active_path, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_nextboot_name, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_nextboot_path, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_root_path, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_snapshot, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, snap_name)
	ZEND_ARG_INFO(0, recursive)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_is_auto_snapshot_name, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, snap)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_create, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_create_depth, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, snap)
	ZEND_ARG_INFO(0, depth)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_create_from_existing, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, be_origin)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_create_from_existing_snap, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, snap)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_rename, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_new)
	ZEND_ARG_INFO(0, be_old)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_activate, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, temporary)
ZEND_END_ARG_INFO()

#if __FreeBSD_version >= 1300000
ZEND_BEGIN_ARG_INFO(arginfo_be_deactivate, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, temporary)
ZEND_END_ARG_INFO()
#endif

ZEND_BEGIN_ARG_INFO(arginfo_be_destroy, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_nicenum, 0)
	ZEND_ARG_INFO(0, num)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_mount, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, mntpoint)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_mounted_at, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_unmount, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_libbe_errno, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_libbe_error_description, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_libbe_print_on_error, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, doprint)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_root_concat, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_validate_name, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_validate_snap, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, snap)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_exists, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_export, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_import, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, be_name)
	ZEND_ARG_INFO(0, fd)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_get_bootenv_props, 0)
	ZEND_ARG_INFO(0, hdl)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_get_dataset_props, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, ds_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_be_get_dataset_snapshots, 0)
	ZEND_ARG_INFO(0, hdl)
	ZEND_ARG_INFO(0, ds_name)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ libbe_functions[]
 */
static const zend_function_entry libbe_functions[] = {
	PHP_FE(libbe_init,			arginfo_libbe_init)
	PHP_FE(libbe_close,			arginfo_libbe_close)
	PHP_FE(libbe_refresh,			arginfo_libbe_refresh)
	PHP_FE(be_active_name,			arginfo_be_active_name)
	PHP_FE(be_active_path,			arginfo_be_active_path)
	PHP_FE(be_nextboot_name,		arginfo_be_nextboot_name)
	PHP_FE(be_nextboot_path,		arginfo_be_nextboot_path)
	PHP_FE(be_root_path,			arginfo_be_root_path)
	PHP_FE(be_snapshot,			arginfo_be_snapshot)
	PHP_FE(be_is_auto_snapshot_name,	arginfo_be_is_auto_snapshot_name)
	PHP_FE(be_create,			arginfo_be_create)
	PHP_FE(be_create_depth,			arginfo_be_create_depth)
	PHP_FE(be_create_from_existing,		arginfo_be_create_from_existing)
	PHP_FE(be_create_from_existing_snap,	arginfo_be_create_from_existing_snap)
	PHP_FE(be_rename,			arginfo_be_rename)
	PHP_FE(be_activate,			arginfo_be_activate)
#if __FreeBSD_version >= 1300000
	PHP_FE(be_deactivate,			arginfo_be_deactivate)
#endif
	PHP_FE(be_destroy,			arginfo_be_destroy)
	PHP_FE(be_nicenum,			arginfo_be_nicenum)
	PHP_FE(be_mount,			arginfo_be_mount)
	PHP_FE(be_mounted_at,			arginfo_be_mounted_at)
	PHP_FE(be_unmount,			arginfo_be_unmount)
	PHP_FE(libbe_errno,			arginfo_libbe_errno)
	PHP_FE(libbe_error_description,		arginfo_libbe_error_description)
	PHP_FE(libbe_print_on_error,		arginfo_libbe_print_on_error)
	PHP_FE(be_root_concat,			arginfo_be_root_concat)
	PHP_FE(be_validate_name,		arginfo_be_validate_name)
	PHP_FE(be_validate_snap,		arginfo_be_validate_snap)
	PHP_FE(be_exists,			arginfo_be_exists)
	PHP_FE(be_export,			arginfo_be_export)
	PHP_FE(be_import,			arginfo_be_import)
	PHP_FE(be_get_bootenv_props,		arginfo_be_get_bootenv_props)
	PHP_FE(be_get_dataset_props,		arginfo_be_get_dataset_props)
	PHP_FE(be_get_dataset_snapshots,	arginfo_be_get_dataset_snapshots)
	PHP_FE_END
};
/* }}} */

/* {{{ libbe_module_entry
 */
zend_module_entry libbe_module_entry = {
	STANDARD_MODULE_HEADER,
	"libbe",						/* Extension name */
	libbe_functions,					/* zend_function_entry */
	PHP_MINIT(libbe),					/* PHP_MINIT - Module initialization */
	NULL,							/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(libbe),					/* PHP_RINIT - Request initialization */
	NULL,							/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(libbe),					/* PHP_MINFO - Module info */
	PHP_LIBBE_VERSION,					/* Version */
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LIBBE
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(libbe)
#endif