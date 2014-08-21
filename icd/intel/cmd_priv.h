/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef CMD_PRIV_H
#define CMD_PRIV_H

#include "genhw/genhw.h"
#include "dev.h"
#include "gpu.h"
#include "cmd.h"

#define CMD_ASSERT(cmd, min_gen, max_gen) \
    INTEL_GPU_ASSERT((cmd)->dev->gpu, (min_gen), (max_gen))

struct intel_cmd_reloc {
    struct intel_cmd_writer *writer;
    XGL_UINT pos;

    uint32_t val;
    const struct intel_mem *mem;

    /*
     * With application state tracking promised by XGL, we should be able to
     * set
     *
     *   I915_EXEC_NO_RELOC
     *   I915_EXEC_HANDLE_LUT
     *   I915_EXEC_IS_PINNED
     *
     * once we figure them out.
     */
    uint16_t read_domains;
    uint16_t write_domain;
};

static inline int cmd_gen(const struct intel_cmd *cmd)
{
    return intel_gpu_gen(cmd->dev->gpu);
}

void cmd_writer_grow(struct intel_cmd *cmd,
                     struct intel_cmd_writer *writer);

/**
 * Reserve \p len DWords in the batch buffer for writing.
 */
static inline void cmd_batch_reserve(struct intel_cmd *cmd, XGL_UINT len)
{
    struct intel_cmd_writer *writer = &cmd->batch;

    if (writer->used + len > writer->size)
        cmd_writer_grow(cmd, writer);
    assert(writer->used + len <= writer->size);
}

/**
 * Write a DWord to the batch buffer and advance.
 */
static inline void cmd_batch_write(struct intel_cmd *cmd, uint32_t val)
{
    struct intel_cmd_writer *writer = &cmd->batch;

    assert(writer->used < writer->size);
    ((uint32_t *) writer->ptr_opaque)[writer->used++] = val;
}

/**
 * Add a reloc for the batch buffer and advance.
 */
static inline void cmd_batch_reloc(struct intel_cmd *cmd,
                                   uint32_t val, const struct intel_mem *mem,
                                   uint16_t read_domains,
                                   uint16_t write_domain)
{
    struct intel_cmd_reloc *reloc = &cmd->relocs[cmd->reloc_used];
    struct intel_cmd_writer *writer = &cmd->batch;

    assert(cmd->reloc_used < cmd->reloc_count);

    reloc->writer = writer;
    reloc->pos = writer->used;
    reloc->val = val;
    reloc->mem = mem;
    reloc->read_domains = read_domains;
    reloc->write_domain = write_domain;

    cmd->reloc_used++;
    writer->used++;
}

/**
 * End the batch buffer.
 */
static inline void cmd_batch_end(struct intel_cmd *cmd)
{
    if (cmd->batch.used & 1) {
        cmd_batch_reserve(cmd, 1);
        cmd_batch_write(cmd, GEN_MI_CMD(MI_BATCH_BUFFER_END));
    } else {
        cmd_batch_reserve(cmd, 2);
        cmd_batch_write(cmd, GEN_MI_CMD(MI_BATCH_BUFFER_END));
        cmd_batch_write(cmd, GEN_MI_CMD(MI_NOOP));
    }
}

#endif /* CMD_PRIV_H */
