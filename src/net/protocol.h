//
// Created by ryan on 2022/5/9.
//

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include "net.h"
#include "buffer.h"

/**
 * Helper function to parse protocol
 * @param buffer
 * @param cache
 * @param begin_idx
 * @param nbytes
 */
void AuxiliaryReadProcCleanup(Buffer &buffer, CommandCache &cache, size_t begin_idx, int nbytes);

/**
 * Helper function to parse protocol
 * @param buffer
 * @param cache
 * @param nbytes
 * @return
 */
bool AuxiliaryReadProc(Buffer &buffer, CommandCache &cache, int nbytes);

#endif // __PROTOCOL_H__
