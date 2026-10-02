/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Minimal stand-in for Qualcomm's IPC logging framework. The ported dwc3-msm
 * sources use it only for debug output, so no-ops are sufficient.
 */
#ifndef _LINUX_IPC_LOGGING_H
#define _LINUX_IPC_LOGGING_H

struct ipc_log_context;

static inline struct ipc_log_context *ipc_log_context_create(int npages,
		const char *name, u32 feature)
{ return NULL; }
static inline void ipc_log_context_destroy(struct ipc_log_context *ilctxt) { }
#define ipc_log_string(ctx, fmt, ...)	do { } while (0)

#endif /* _LINUX_IPC_LOGGING_H */
