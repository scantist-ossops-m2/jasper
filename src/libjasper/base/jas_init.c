/*
 * Copyright (c) 2001-2002 Michael David Adams.
 * All rights reserved.
 */

/* __START_OF_JASPER_LICENSE__
 * 
 * JasPer License Version 2.0
 * 
 * Copyright (c) 2001-2006 Michael David Adams
 * Copyright (c) 1999-2000 Image Power, Inc.
 * Copyright (c) 1999-2000 The University of British Columbia
 * 
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person (the
 * "User") obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 * 
 * 1.  The above copyright notices and this permission notice (which
 * includes the disclaimer below) shall be included in all copies or
 * substantial portions of the Software.
 * 
 * 2.  The name of a copyright holder shall not be used to endorse or
 * promote products derived from the Software without specific prior
 * written permission.
 * 
 * THIS DISCLAIMER OF WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS
 * LICENSE.  NO USE OF THE SOFTWARE IS AUTHORIZED HEREUNDER EXCEPT UNDER
 * THIS DISCLAIMER.  THE SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS
 * "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.  IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL
 * INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  NO ASSURANCES ARE
 * PROVIDED BY THE COPYRIGHT HOLDERS THAT THE SOFTWARE DOES NOT INFRINGE
 * THE PATENT OR OTHER INTELLECTUAL PROPERTY RIGHTS OF ANY OTHER ENTITY.
 * EACH COPYRIGHT HOLDER DISCLAIMS ANY LIABILITY TO THE USER FOR CLAIMS
 * BROUGHT BY ANY OTHER ENTITY BASED ON INFRINGEMENT OF INTELLECTUAL
 * PROPERTY RIGHTS OR OTHERWISE.  AS A CONDITION TO EXERCISING THE RIGHTS
 * GRANTED HEREUNDER, EACH USER HEREBY ASSUMES SOLE RESPONSIBILITY TO SECURE
 * ANY OTHER INTELLECTUAL PROPERTY RIGHTS NEEDED, IF ANY.  THE SOFTWARE
 * IS NOT FAULT-TOLERANT AND IS NOT INTENDED FOR USE IN MISSION-CRITICAL
 * SYSTEMS, SUCH AS THOSE USED IN THE OPERATION OF NUCLEAR FACILITIES,
 * AIRCRAFT NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL
 * SYSTEMS, DIRECT LIFE SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH
 * THE FAILURE OF THE SOFTWARE OR SYSTEM COULD LEAD DIRECTLY TO DEATH,
 * PERSONAL INJURY, OR SEVERE PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH
 * RISK ACTIVITIES").  THE COPYRIGHT HOLDERS SPECIFICALLY DISCLAIM ANY
 * EXPRESS OR IMPLIED WARRANTY OF FITNESS FOR HIGH RISK ACTIVITIES.
 * 
 * __END_OF_JASPER_LICENSE__
 */

/******************************************************************************\
* Includes.
\******************************************************************************/

#define JAS_INTERNAL_USE_ONLY

#include "jasper/jas_init.h"
#include "jasper/jas_image.h"
#include "jasper/jas_malloc.h"
#include "jasper/jas_debug.h"
#include "jasper/jas_string.h"
#include "jasper/jas_thread.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

/******************************************************************************\
* Types.
\******************************************************************************/

typedef struct {
	jas_conf_t conf;
	bool initialized;
	size_t num_active_threads;
	jas_ctx_t *ctx;
	jas_ctx_t ctx_buf;
#if defined(JAS_THREADS)
	jas_mutex_t lock;
	jas_tss_t cur_ctx_tss;
	jas_tss_t default_ctx_tss;
#endif
} jas_global_t;

/******************************************************************************\
* Function prototypes.
\******************************************************************************/

void jas_ctx_init(jas_ctx_t *ctx);
static int jas_init_codecs(jas_ctx_t *ctx);
void jas_ctx_cleanup(jas_ctx_t *ctx);
void jas_set_ctx(jas_ctx_t *ctx);
void jas_set_default_ctx(jas_ctx_t *ctx);
jas_ctx_t *jas_ctx_create(void);
void jas_ctx_destroy(jas_ctx_t *ctx);
jas_ctx_t *jas_get_ctx_impl(void);
jas_ctx_t *jas_get_default_ctx(void);

/******************************************************************************\
* Codec Table.
\******************************************************************************/

static const jas_image_fmt_t jas_image_fmts[] = {

#if defined(JAS_INCLUDE_MIF_CODEC) && defined(JAS_ENABLE_MIF_CODEC)
	{
		"mif",
		"My Image Format (MIF)",
		"mif",
		{
			.decode = mif_decode,
			.encode = mif_encode,
			.validate = mif_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_PNM_CODEC)
	{
		"pnm",
		"Portable Graymap/Pixmap (PNM)",
		"pnm pbm pgm ppm",
		{
			.decode = pnm_decode,
			.encode = pnm_encode,
			.validate = pnm_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_BMP_CODEC)
	{
		"bmp",
		"Microsoft Bitmap (BMP)",
		"bmp",
		{
			.decode = bmp_decode,
			.encode = bmp_encode,
			.validate = bmp_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_RAS_CODEC)
	{
		"ras",
		"Sun Rasterfile (RAS)",
		"ras",
		{
			.decode = ras_decode,
			.encode = ras_encode,
			.validate = ras_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_JP2_CODEC)
	{
		"jp2",
		"JPEG-2000 JP2 File Format Syntax (ISO/IEC 15444-1)",
		"jp2",
		{
			.decode = jp2_decode,
			.encode = jp2_encode,
			.validate = jp2_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_JPC_CODEC) || defined(JAS_INCLUDE_JP2_CODEC)
	{
		"jpc",
		"JPEG-2000 Code Stream Syntax (ISO/IEC 15444-1)",
		"jpc",
		{
			.decode = jpc_decode,
			.encode = jpc_encode,
			.validate = jpc_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_JPG_CODEC)
	{
		"jpg",
		"JPEG (ISO/IEC 10918-1)",
		"jpg",
		{
			.decode = jpg_decode,
			.encode = jpg_encode,
			.validate = jpg_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_HEIC_CODEC)
	{
		"heic",
		"HEIC (ISO/IEC 23008-12)",
		"heic heif",
		{
			.decode = jas_heic_decode,
			.encode = jas_heic_encode,
			.validate = jas_heic_validate
		}
	},
#endif

#if defined(JAS_INCLUDE_PGX_CODEC)
	{
		"pgx",
		"JPEG-2000 VM Format (PGX)",
		"pgx",
		{
			.decode = pgx_decode,
			.encode = pgx_encode,
			.validate = pgx_validate
		}
	},
#endif

};

/******************************************************************************\
* Configuration and Library Data.
\******************************************************************************/

/* MUTABLE_SHARED_STATE_TAG: This is mutable shared state. */
/*
Various user-configurable settings.
*/
jas_conf_t jas_conf = {
	.initialized = 0,
};

/* MUTABLE_SHARED_STATE_TAG: This is mutable shared state. */
jas_global_t jas_global = {
	.initialized = 0,
#if defined(JAS_THREADS)
	.lock = JAS_MUTEX_INITIALIZER,
#endif
};

/******************************************************************************\
* Codec Table Code.
\******************************************************************************/

JAS_EXPORT
void jas_get_image_format_table(const jas_image_fmt_t **formats,
  size_t *num_formats)
{
	*formats = jas_image_fmts;
	*num_formats = sizeof(jas_image_fmts) / sizeof(jas_image_fmt_t);
}

/******************************************************************************\
* Library Configuration.
\******************************************************************************/

JAS_EXPORT
void jas_conf_clear()
{
#if 0
	/* NOTE: The following two lines of code require that configuration,
	  initialization, and cleanup of the library be performed on the
	  same thread. */
	assert(!jas_conf.initialized);
#endif

	//memset(&jas_conf, 0, sizeof(jas_conf_t));
	//jas_conf.configured = 1;
	jas_conf.multithread = 0;
	jas_conf.allocator = 0;
	jas_conf.enable_allocator_wrapper = 1;
	jas_conf.max_mem = JAS_DEFAULT_MAX_MEM_USAGE;
	jas_conf.dec_default_max_samples = JAS_DEC_DEFAULT_MAX_SAMPLES;
	jas_conf.debug_level = 0;
	jas_conf.vlogmsgf = jas_vlogmsgf_stderr;
	//jas_conf.vlogmsgf = jas_vlogmsgf_discard;
	jas_conf.num_image_formats = sizeof(jas_image_fmts) /
	  sizeof(jas_image_fmt_t);
	jas_conf.image_formats = jas_image_fmts;
	jas_conf.initialized = 1;
}

JAS_EXPORT
void jas_conf_set_multithread(int multithread)
{
	jas_conf.multithread = multithread;
}

JAS_EXPORT
void jas_conf_set_allocator(jas_allocator_t *allocator)
{
	jas_conf.allocator = allocator;
}

JAS_EXPORT
void jas_conf_set_allocator_wrapper(int enable)
{
	jas_conf.enable_allocator_wrapper = enable;
}

JAS_EXPORT
void jas_conf_set_debug_level(int debug_level)
{
	jas_conf.debug_level = debug_level;
}

JAS_EXPORT
void jas_conf_set_max_mem(size_t max_mem)
{
	jas_conf.max_mem = max_mem;
}

JAS_EXPORT
void jas_conf_set_dec_default_max_samples(size_t n)
{
	jas_conf.dec_default_max_samples = n;
}

JAS_EXPORT
void jas_conf_set_vlogmsgf(int (*func)(jas_logtype_t type, const char *,
  va_list))
{
	jas_conf.vlogmsgf = func;
}

JAS_EXPORT
void jas_conf_set_image_format_table(const jas_image_fmt_t *formats,
  size_t num_formats)
{
	jas_conf.image_formats = formats;
	jas_conf.num_image_formats = num_formats;
}

/******************************************************************************\
* Library Initialization and Cleanup.
\******************************************************************************/

JAS_EXPORT
int jas_init_library()
{
	bool has_lock = false;
	int ret = 0;
	int status;

	JAS_UNUSED(status);

#if defined(JAS_THREADS)
	jas_mutex_lock(&jas_global.lock);
#endif
	has_lock = true;

	/*
	The following check requires that library configuration be performed on
	the same thread as library initialization.
	*/
	assert(jas_conf.initialized);
	assert(!jas_global.initialized);

	// do not memset. this will trash mutex
	//memset(&jas_global, 0, sizeof(jas_global_t));
	jas_global.conf = jas_conf;

#if !defined(JAS_THREADS)
	if (jas_conf.multithread) {
		jas_eprintf("library not built with multithreading support\n");
		ret = -1;
		goto done;
	}
#endif

#if defined(JAS_THREADS)
	memset(&jas_global.cur_ctx_tss, 0, sizeof(jas_tss_t));
	memset(&jas_global.default_ctx_tss, 0, sizeof(jas_tss_t));
#endif

	/*
	The initialization of the global context must be performed first.
	The memory allocator uses information from the context in order to
	handle log messages.
	*/
	jas_ctx_init(&jas_global.ctx_buf);
	jas_global.ctx = &jas_global.ctx_buf;

	if (jas_global.conf.enable_allocator_wrapper) {
		jas_allocator_t *delegate;
		if (!jas_global.conf.allocator) {
			jas_std_allocator_init(&jas_std_allocator);
			delegate = &jas_std_allocator.base;
		} else {
			delegate = jas_global.conf.allocator;
		}
		jas_basic_allocator_init(&jas_basic_allocator, delegate,
		  jas_global.conf.max_mem);
		jas_allocator = &jas_basic_allocator.base;
	} else {
		if (!jas_global.conf.allocator) {
			jas_std_allocator_init(&jas_std_allocator);
			jas_allocator = &jas_std_allocator.base;
		} else {
			jas_allocator = jas_conf.allocator;
		}
	}

#if defined(JAS_THREADS)
	if ((status = jas_tss_create(&jas_global.cur_ctx_tss, 0))) {
		jas_eprintf("cannot create thread-specific storage %d\n", status);
		ret = -1;
		goto done;
	}
	if ((status = jas_tss_create(&jas_global.default_ctx_tss, 0))) {
		jas_eprintf("cannot create thread-specific storage %d\n", status);
		ret = -1;
		goto done;
	}
#endif

	jas_global.ctx->dec_default_max_samples = jas_conf.dec_default_max_samples;
	jas_global.ctx->debug_level = jas_conf.debug_level;
	//jas_setdbglevel(jas_global.conf.debug_level);
	jas_global.ctx->image_numfmts = 0;

	jas_global.initialized = 1;

#if defined(JAS_THREADS)
	jas_mutex_unlock(&jas_global.lock);
#endif
	has_lock = false;

	JAS_LOGDEBUGF(1, "memory size: %zu\n", jas_get_total_mem_size());
	assert(jas_global.conf.initialized);

done:
	if (has_lock) {
#if defined(JAS_THREADS)
		jas_mutex_unlock(&jas_global.lock);
#endif
	}

	return ret;
}

JAS_EXPORT
int jas_cleanup_library()
{
	jas_ctx_t *ctx;
	bool has_lock = false;

#if defined(JAS_THREADS)
	jas_mutex_lock(&jas_global.lock);
#endif
	has_lock = true;

	assert(jas_global.initialized);
	assert(!jas_global.num_active_threads);

	JAS_LOGDEBUGF(10, "jas_cleanup_library invoked\n");

	ctx = &jas_global.ctx_buf;
	jas_ctx_cleanup(ctx);
	//jas_context_set_debug_level(ctx, 0);

	assert(jas_allocator);
	jas_allocator_cleanup(jas_allocator);
	jas_allocator = 0;

	JAS_LOGDEBUGF(10, "jas_cleanup_library returning\n");

#if defined(JAS_THREADS)
	jas_tss_delete(jas_global.cur_ctx_tss);
	jas_tss_delete(jas_global.default_ctx_tss);
#endif

	/* NOTE: The following two lines of code require that configuration,
	  initialization, and cleanup of the library be performed on the
	  same thread. */
	//jas_conf.configured = 0;
	//jas_conf.initialized = 0;

	if (has_lock) {
#if defined(JAS_THREADS)
		jas_mutex_unlock(&jas_global.lock);
#endif
	}

	return 0;
}

/******************************************************************************\
* Thread Initialization and Cleanup.
\******************************************************************************/

JAS_EXPORT
int jas_init_thread()
{
	int ret = 0;
	jas_ctx_t *ctx = 0;
	bool has_lock = false;

	/*
	The default context must be established as soon as possible.
	Prior to the default context being initialized the global state in
	jas_global.conf is used for some settings (e.g., debug level
	and logging function).
	Due to the use of this global state, we must hold the lock on
	this state until the default context is initialized.	
	*/
#if defined(JAS_THREADS)
	jas_mutex_lock(&jas_global.lock);
#endif
	has_lock = true;

	assert(jas_global.initialized);
#if !defined(JAS_THREADS)
	assert(jas_global.num_active_threads == 0);
#endif

	if (!(ctx = jas_ctx_create())) {
		ret = -1;
		goto done;
	}
	jas_set_ctx(ctx);
	/* As of this point, shared use of jas_global.conf is no longer needed. */
	jas_set_default_ctx(ctx);
	++jas_global.num_active_threads;

#if defined(JAS_THREADS)
	jas_mutex_unlock(&jas_global.lock);
#endif
	has_lock = false;

done:
	if (has_lock) {
#if defined(JAS_THREADS)
		jas_mutex_unlock(&jas_global.lock);
#endif
	}
	if (ret && ctx) {
		jas_ctx_cleanup(ctx);
	}
	return ret;
}

JAS_EXPORT
int jas_cleanup_thread()
{
	bool has_lock = false;
	jas_ctx_t *ctx;

#if defined(JAS_THREADS)
	jas_mutex_lock(&jas_global.lock);
#endif
	has_lock = true;

	/* Ensure that the library user is not doing something insane. */
	assert(jas_get_ctx() == jas_get_default_ctx());

	ctx = jas_get_default_ctx();
	jas_set_default_ctx(0);
	jas_set_ctx(0);
	/*
	As soon as we clear the current context, the global shared state
	jas_conf.conf is used for various settings (e.g., debug level and
	logging function).
	*/

	jas_ctx_destroy(ctx);
	--jas_global.num_active_threads;

	if (has_lock) {
#if defined(JAS_THREADS)
		jas_mutex_unlock(&jas_global.lock);
#endif
	}

	return 0;
}

/******************************************************************************\
* Legacy Library Initialization and Cleanup.
\******************************************************************************/

JAS_EXPORT
int jas_init()
{
	/* NOTE: The following three lines of code require that configuration,
	  initialization, and cleanup of the library be performed on the
	  same thread. */
	assert(!jas_conf.initialized);
	assert(!jas_global.initialized);

	jas_conf_clear();

	if (jas_init_library()) {
		return -1;
	}
	if (jas_init_thread()) {
		jas_cleanup_library();
		return -1;
	}
	return 0;
}

JAS_EXPORT
void jas_cleanup()
{
	if (jas_cleanup_thread()) {
		jas_eprintf("jas_cleanup_thread failed\n");
	}
	if (jas_cleanup_library()) {
		jas_eprintf("jas_cleanup_library failed\n");
	}
}

/******************************************************************************\
* Code.
\******************************************************************************/

/* Initialize the image format table. */
static int jas_init_codecs(jas_ctx_t *ctx)
{
	int fmtid;
	const char delim[] = " \t";

	const jas_image_fmt_t *fmt;
	size_t i;
	fmtid = 0;
	for (fmt = jas_global.conf.image_formats, i = 0;
	  i < jas_global.conf.num_image_formats; ++fmt, ++i) {
		char *buf;
		if (!(buf = jas_strdup(fmt->exts))) {
			return -1;
		}
		bool first = true;
		for (;;) {
			char *saveptr;
			char *ext;
			if (!(ext = jas_strtok(first ? buf : NULL, delim, &saveptr))) {
				break;
			}
			JAS_LOGDEBUGF(10, "adding image format %s %s\n", fmt->name, ext);
			jas_image_addfmt_internal(ctx->image_fmtinfos, &ctx->image_numfmts,
			  fmtid, fmt->name, ext, fmt->desc, &fmt->ops);
			++fmtid;
			first = false;
		}
		jas_free(buf);
	}
	return 0;
}

/******************************************************************************\
\******************************************************************************/

/* For internal library use only. */
jas_conf_t *jas_get_conf_ptr()
{
	return &jas_conf;
}

void jas_ctx_init(jas_ctx_t *ctx)
{
	memset(ctx, 0, sizeof(jas_ctx_t));
	ctx->dec_default_max_samples = jas_conf.dec_default_max_samples;
	ctx->debug_level = jas_conf.debug_level;
	ctx->vlogmsgf = jas_conf.vlogmsgf;
	ctx->image_numfmts = 0;
}

jas_ctx_t *jas_ctx_create()
{
	jas_ctx_t *ctx;
	if (!(ctx = jas_malloc(sizeof(jas_ctx_t)))) {
		return 0;
	}
	jas_ctx_init(ctx);
	jas_init_codecs(ctx);
	return ctx;
}

JAS_EXPORT
jas_context_t jas_context_create()
{
	return JAS_CAST(jas_context_t, jas_ctx_create());
}

void jas_ctx_cleanup(jas_ctx_t *ctx)
{
	jas_image_clearfmts_internal(ctx->image_fmtinfos, &ctx->image_numfmts);
}

void jas_ctx_destroy(jas_ctx_t *ctx)
{
	jas_ctx_cleanup(ctx);
	jas_free(ctx);
}

JAS_EXPORT
void jas_context_destroy(jas_context_t context)
{
	jas_ctx_destroy(JAS_CAST(jas_ctx_t *, context));
}

jas_ctx_t *jas_get_ctx_impl()
{
#if defined(JAS_THREADS)
	jas_ctx_t *ctx = JAS_CAST(jas_ctx_t *, jas_tss_get(jas_global.cur_ctx_tss));
	if (!ctx) {
		ctx = jas_global.ctx;
	}
	return ctx;
#else
	return (jas_global.ctx) ? jas_global.ctx : &jas_global.ctx_buf;
#endif
}

JAS_EXPORT
jas_context_t jas_get_context()
{
	return JAS_CAST(jas_context_t, jas_get_ctx());
}

JAS_EXPORT
jas_context_t jas_get_default_context()
{
	return JAS_CAST(jas_context_t, jas_get_default_ctx());
}

JAS_EXPORT
void jas_set_context(jas_context_t context)
{
#if defined(JAS_THREADS)
	if (jas_tss_set(jas_global.cur_ctx_tss, JAS_CAST(void *, context))) {
		assert(0);
		abort();
	}
#else
	jas_global.ctx = JAS_CAST(jas_ctx_t *, context);
#endif
}

void jas_set_ctx(jas_ctx_t *ctx)
{
#if defined(JAS_THREADS)
	if (jas_tss_set(jas_global.cur_ctx_tss, ctx)) {
		assert(0);
		abort();
	}
#else
	jas_global.ctx = JAS_CAST(jas_ctx_t *, ctx);
#endif
}

jas_ctx_t *jas_get_default_ctx()
{
#if defined(JAS_THREADS)
	jas_ctx_t *ctx = JAS_CAST(jas_ctx_t *, jas_tss_get(jas_global.default_ctx_tss));
	if (!ctx) {
		ctx = jas_global.ctx;
	}
	return ctx;
#else
	return (jas_global.ctx) ? jas_global.ctx : &jas_global.ctx_buf;
#endif
}

void jas_set_default_ctx(jas_ctx_t *ctx)
{
#if defined(JAS_THREADS)
	if (jas_tss_set(jas_global.default_ctx_tss, ctx)) {
		assert(0);
		abort();
	}
#else
	jas_global.ctx = JAS_CAST(jas_ctx_t *, ctx);
#endif
}

JAS_EXPORT
int jas_set_debug_level(int debug_level)
{
	jas_ctx_t *ctx = jas_get_ctx();
	int old = ctx->debug_level;
	ctx->debug_level = debug_level;
	return old;
}

JAS_EXPORT
void jas_set_dec_default_max_samples(size_t max_samples)
{
	jas_ctx_t *ctx = jas_get_ctx();
	ctx->dec_default_max_samples = max_samples;
}

JAS_EXPORT
void jas_set_vlogmsgf(int (*func)(jas_logtype_t, const char *,
  va_list))
{
	jas_ctx_t *ctx = jas_get_ctx();
	ctx->vlogmsgf = func;
}

JAS_EXPORT
void jas_get_image_fmtinfo_table(const jas_image_fmtinfo_t **fmtinfos,
  size_t *numfmts)
{
	jas_ctx_t *ctx = jas_get_ctx();
	*fmtinfos = ctx->image_fmtinfos;
	*numfmts = ctx->image_numfmts;
}
