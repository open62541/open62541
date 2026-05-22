/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) o6 Automation GmbH (Author: Julius Pfrommer)
 */

/* Can't use <zephyr/version.h> since v2.7.0 and older stores the file in
 * another place */
# include <version.h>

/* For 3.7.0 or higher use the k_ memory management methods */
#if(KERNEL_VERSION_MAJOR >= 4 || \
   (KERNEL_VERSION_MAJOR == 3 && KERNEL_VERSION_MINOR >=7) )
# include <zephyr/kernel.h>
# define UA_free k_free
# define UA_malloc k_malloc
# define UA_calloc k_calloc
# define UA_realloc k_realloc
#endif
