/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_mem.h"
#include "cr_string.h"
#include "packer.h"
#include "cr_error.h"
#include "cr_protocol.h"
#ifndef IN_RING0
#include "cr_unpack.h"
#endif

#ifndef IN_RING0
void crWriteUnalignedDouble( void *buffer, double d )
{
	unsigned int *ui = (unsigned int *) buffer;
	ui[0] = ((unsigned int *) &d)[0];
	ui[1] = ((unsigned int *) &d)[1];
}

double crReadUnalignedDouble( const void *buffer )
{
	const unsigned int *ui = (unsigned int *) buffer;
	double d;
	((unsigned int *) &d)[0] = ui[0];
	((unsigned int *) &d)[1] = ui[1];
	return d;
}
#endif
/*
 * We need the packer to run as efficiently as possible.  To avoid one
 * pointer dereference from the CRPackContext to the current CRPackBuffer,
 * we keep a _copy_ of the current CRPackBuffer in the CRPackContext and
 * operate on the fields in CRPackContext, rather than the CRPackBuffer.
 *
 * To keep things in sync, when we change a context's
 * buffer, we have to use the crPackSet/GetBuffer() functions.
 */

void crPackSetBuffer( CRPackContext *pc, CRPackBuffer *buffer )
{
	CRASSERT( pc );
	CRASSERT( buffer );

	if (pc->currentBuffer == buffer)
		return; /* re-bind is no-op */

	if (pc->currentBuffer) {
		/* Another buffer currently bound to this packer (shouldn't normally occur)
		 * Release it.  Fixes Ensight issue.
		 */
		crPackReleaseBuffer(pc);
	}

	CRASSERT( pc->currentBuffer == NULL);  /* release if NULL? */
	CRASSERT( buffer->context == NULL );

	/* bind context to buffer */
	pc->currentBuffer = buffer;
	buffer->context = pc;

	/* update the context's packing fields with those from the buffer */
	pc->buffer = *buffer;  /* struct copy */
}

#ifndef IN_RING0
/* This is useful for debugging packer problems */
void crPackSetBufferDEBUG( const char *file, int line, CRPackContext *pc, CRPackBuffer *buffer)
						   
{
	crPackSetBuffer( pc, buffer );
	/* record debugging info */
	pc->file = crStrdup(file);
	pc->line = line;
}
#endif

/*
 * Release the buffer currently attached to the context.
 * Update/resync data structures.
 */
void crPackReleaseBuffer( CRPackContext *pc )
{
	CRPackBuffer *buf;
	CRASSERT( pc );

	if (!pc->currentBuffer) {
		crWarning("crPackReleaseBuffer called with no current buffer");
		return; /* nothing to do */
	}

	CRASSERT( pc->currentBuffer->context == pc );

	/* buffer to release */
	buf = pc->currentBuffer;

	/* copy context's fields back into the buffer to update it */
	*buf = pc->buffer;        /* struct copy */

	/* unbind buffer from context */
	buf->context = NULL;
	pc->currentBuffer = NULL;

	/* zero-out context's packing fields just to be safe */
	crMemZero(&(pc->buffer), sizeof(pc->buffer));

	/* update the debugging fields */
	if (pc->file)
		crFree(pc->file);
	pc->file = NULL;
	pc->line = -1;
}

void crPackFlushFunc( CRPackContext *pc, CRPackFlushFunc ff )
{
	pc->Flush = ff;
}

void crPackFlushArg( CRPackContext *pc, void *flush_arg )
{
	pc->flush_arg = flush_arg;
}

void crPackSendHugeFunc( CRPackContext *pc, CRPackSendHugeFunc shf )
{
	pc->SendHuge = shf;
}

/*
 * This basically resets the buffer attached to <pc> to the default, empty
 * state.
 */
void crPackResetPointers( CRPackContext *pc )
{
	const GLboolean geom_only = pc->buffer.geometry_only;   /* save this flag */
	const GLboolean holds_BeginEnd = pc->buffer.holds_BeginEnd;
	const GLboolean in_BeginEnd = pc->buffer.in_BeginEnd;
	const GLboolean canBarf = pc->buffer.canBarf;
	CRPackBuffer *buf = pc->currentBuffer;
	CRASSERT(buf);
	crPackInitBuffer( buf, buf->pack, buf->size, buf->mtu
#ifdef IN_RING0
	        , 0
#endif
	        );
	pc->buffer.geometry_only = geom_only;   /* restore the flag */
	pc->buffer.holds_BeginEnd = holds_BeginEnd;
	pc->buffer.in_BeginEnd = in_BeginEnd;
	pc->buffer.canBarf = canBarf;
}


/**
 * Return max number of opcodes that'll fit in the given buffer size.
 * Each opcode has at least a 1-word payload, so opcodes can occupy at most 
 * 20% of the space.
 */
int
crPackMaxOpcodes( int buffer_size )
{
	int n = ( buffer_size - sizeof(CRMessageOpcodes) ) / 5;
	/* Don't forget to add one here in case the buffer size is not
	 * divisible by 4.  Thanks to Ken Moreland for finding this.
	 */
	n++;
	/* round up to multiple of 4 */
	n = (n + 0x3) & (~0x3);
	return n;
}


/**
 * Return max number of data bytes that'll fit in the given buffer size.
 */
int
crPackMaxData( int buffer_size )
{
	int n = buffer_size - sizeof(CRMessageOpcodes);
	n -= crPackMaxOpcodes(buffer_size);
	return n;
}


/**
 * Initialize the given CRPackBuffer object.
 * The buffer may or may not be currently bound to a CRPackContext.
 *
 * Opcodes and operands are packed into a buffer in a special way.
 * Opcodes start at opcode_start and go downward in memory while operands
 * start at data_start and go upward in memory.  The buffer is full when we
 * either run out of opcode space or operand space.
 *
 * Diagram (memory addresses increase upward):
 *
 *     data_end -> |         |    <- buf->pack + buf->size
 *                 +---------+
 *                 |         |
 *                 |         |
 *                 | operands|
 *                 |         |
 *                 |         |
 *   data_start -> +---------+
 * opcode_start -> |         |
 *                 |         |
 *                 | opcodes |
 *                 |         |
 *                 |         |
 *  opcode_end  -> +---------+    <- buf->pack
 *
 * \param buf  the CRPackBuffer to initialize
 * \param pack  the address of the buffer for packing opcodes/operands.
 * \param size  size of the buffer, in bytes
 * \param mtu   max transmission unit size, in bytes.  When the buffer
 *              has 'mtu' bytes in it, we have to send it.  The MTU might
 *              be somewhat smaller than the buffer size.
 */
void crPackInitBuffer( CRPackBuffer *buf, void *pack, int size, int mtu
#ifdef IN_RING0
        , unsigned int num_opcodes
#endif
        )
{
#ifndef IN_RING0
	unsigned int num_opcodes;
#endif

	CRASSERT(mtu <= size);

	buf->size = size;
	buf->mtu  = mtu;
	buf->pack = pack;

#ifdef IN_RING0
	if(num_opcodes)
	{
	    num_opcodes = (num_opcodes + 0x3) & (~0x3);
	}
	else
#endif
	{
	    num_opcodes = crPackMaxOpcodes( buf->size );
	}

	buf->data_start    = 
		(unsigned char *) buf->pack + num_opcodes + sizeof(CRMessageOpcodes);
	buf->data_current  = buf->data_start;
	buf->data_end      = (unsigned char *) buf->pack + buf->size;

	buf->opcode_start   = buf->data_start - 1;
	buf->opcode_current = buf->opcode_start;
	buf->opcode_end     = buf->opcode_start - num_opcodes;

	buf->geometry_only = GL_FALSE;
	buf->holds_BeginEnd = GL_FALSE;
	buf->in_BeginEnd = GL_FALSE;
	buf->canBarf = GL_FALSE;

	if (buf->context) {
		/* Also reset context's packing fields */
		CRPackContext *pc = buf->context;
		CRASSERT(pc->currentBuffer == buf);
		/*crMemcpy( &(pc->buffer), buf, sizeof(*buf) );*/
		pc->buffer = *buf;
	}
}


int crPackCanHoldBuffer( CR_PACKER_CONTEXT_ARGDECL const CRPackBuffer *src )
{
	const int num_data = crPackNumData(src);
	const int num_opcode = crPackNumOpcodes(src);
    int res;
	CR_GET_PACKER_CONTEXT(pc);
    CR_LOCK_PACKER_CONTEXT(pc);
	res = crPackCanHoldOpcode( pc, num_opcode, num_data );
    CR_UNLOCK_PACKER_CONTEXT(pc);
    return res;
}


int crPackCanHoldBoundedBuffer( CR_PACKER_CONTEXT_ARGDECL const CRPackBuffer *src )
{
	const int len_aligned = (src->data_current - src->opcode_current - 1 + 3) & ~3;
	CR_GET_PACKER_CONTEXT(pc);
	/* 24 is the size of the bounds-info packet... */
	return crPackCanHoldOpcode( pc, 1, len_aligned + 24 );
}

void crPackAppendBuffer( CR_PACKER_CONTEXT_ARGDECL const CRPackBuffer *src )
{
	CR_GET_PACKER_CONTEXT(pc);
	const int num_data = crPackNumData(src);
	const int num_opcode = crPackNumOpcodes(src);

	CRASSERT(num_data >= 0);
	CRASSERT(num_opcode >= 0);

    CR_LOCK_PACKER_CONTEXT(pc);

	/* don't append onto ourself! */
	CRASSERT(pc->currentBuffer);
	CRASSERT(pc->currentBuffer != src);

	if (!crPackCanHoldBuffer(CR_PACKER_CONTEXT_ARG src))
	{
		if (src->holds_BeginEnd)
		{
			crWarning( "crPackAppendBuffer: overflowed the destination!" );
            CR_UNLOCK_PACKER_CONTEXT(pc);
			return;
		}
		else
        {
			crError( "crPackAppendBuffer: overflowed the destination!" );
            CR_UNLOCK_PACKER_CONTEXT(pc);
        }
	}

	/* Copy the buffer data/operands which are at the head of the buffer */
	crMemcpy( pc->buffer.data_current, src->data_start, num_data );
	pc->buffer.data_current += num_data;

	/* Copy the buffer opcodes which are at the tail of the buffer */
	CRASSERT( pc->buffer.opcode_current - num_opcode >= pc->buffer.opcode_end );
	crMemcpy( pc->buffer.opcode_current + 1 - num_opcode, src->opcode_current + 1,
			num_opcode );
	pc->buffer.opcode_current -= num_opcode;
	pc->buffer.holds_BeginEnd |= src->holds_BeginEnd;
	pc->buffer.in_BeginEnd = src->in_BeginEnd;
	pc->buffer.holds_List |= src->holds_List;
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


void
crPackAppendBoundedBuffer( CR_PACKER_CONTEXT_ARGDECL const CRPackBuffer *src, const CRrecti *bounds )
{
	CR_GET_PACKER_CONTEXT(pc);
	const GLbyte *payload = (const GLbyte *) src->opcode_current + 1;
	const int num_opcodes = crPackNumOpcodes(src);
	const int length = src->data_current - src->opcode_current - 1;

	CRASSERT(pc);
    CR_LOCK_PACKER_CONTEXT(pc);
	CRASSERT(pc->currentBuffer);
	CRASSERT(pc->currentBuffer != src);

	/*
	 * payload points to the block of opcodes immediately followed by operands.
	 */

	if ( !crPackCanHoldBoundedBuffer( CR_PACKER_CONTEXT_ARG src ) )
	{
		if (src->holds_BeginEnd)
		{
			crWarning( "crPackAppendBoundedBuffer: overflowed the destination!" );
            CR_UNLOCK_PACKER_CONTEXT(pc);
			return;
		}
		else
        {
			crError( "crPackAppendBoundedBuffer: overflowed the destination!" );
            CR_UNLOCK_PACKER_CONTEXT(pc);
        }
	}

	crPackBoundsInfoCR( CR_PACKER_CONTEXT_ARG bounds, payload, length, num_opcodes );
	pc->buffer.holds_BeginEnd |= src->holds_BeginEnd;
	pc->buffer.in_BeginEnd = src->in_BeginEnd;
	pc->buffer.holds_List |= src->holds_List;
    CR_UNLOCK_PACKER_CONTEXT(pc);
}


/*
 * Allocate space for a command that might be very large, such as
 * glTexImage2D or glBufferDataARB call.
 * The command buffer _MUST_ then be transmitted by calling crHugePacket.
 */
void *crPackAlloc( CR_PACKER_CONTEXT_ARGDECL unsigned int size )
{
	CR_GET_PACKER_CONTEXT(pc);
	unsigned char *data_ptr;

	/* include space for the length and make the payload word-aligned */
	size = ( size + sizeof(unsigned int) + 0x3 ) & ~0x3;

    CR_LOCK_PACKER_CONTEXT(pc);

	if ( crPackCanHoldOpcode( pc, 1, size ) )
	{
		/* we can just put it in the current buffer */
		CR_GET_BUFFERED_POINTER_NOLOCK(pc, size );  /* NOTE: this sets data_ptr */
	}
	else 
	{
		/* Okay, it didn't fit.  Maybe it will after we flush. */
		CR_UNLOCK_PACKER_CONTEXT(pc);
		pc->Flush( pc->flush_arg );
		CR_LOCK_PACKER_CONTEXT(pc);
		if ( crPackCanHoldOpcode( pc, 1, size ) )
		{
			CR_GET_BUFFERED_POINTER_NOLOCK(pc, size );  /* NOTE: this sets data_ptr */
		}
		else
		{
			/* It's really way too big, so allocate a temporary packet
			 * with space for the single opcode plus the payload &
			 * header.
			 */
			data_ptr = (unsigned char *) 
				crAlloc( sizeof(CRMessageOpcodes) + 4 + size );

			/* skip the header & opcode space */
			data_ptr += sizeof(CRMessageOpcodes) + 4;
		}
	}

	/* At the top of the function, we added four to the request size and
	 * rounded it up to the next multiple of four.
	 *
	 * At this point, we have:
	 *
	 * HIGH MEM        | byte size - 1    |  \
	 *                   ...                  |
	 *                   ...                  | - original 'size' bytes for data
	 *                 | operand data     |   |
	 * return value -> | operand data     |  /
	 *                 | byte 3           |  \
	 *                 | byte 2           |   |- These bytes will store 'size'
	 *                 | byte 1           |   |  
	 * data_ptr ->     | byte 0           |  /
	 *                 | CR opcode        |  <- Set in packspuHuge()
	 *                 | unused           |
	 *                 | unused           |
	 *                 | unused           |
	 *                 | CRMessageOpcodes |
	 *                 | CRMessageOpcodes |
	 *                   ...               
	 *                 | CRMessageOpcodes |
	 *                 | CRMessageOpcodes |
	 * LOW MEM         +------------------+
	 */

	*((unsigned int *) data_ptr) = size;
	return data_ptr + 4;
}

#define IS_BUFFERED( packet ) \
    ((unsigned char *) (packet) >= pc->buffer.data_start && \
	 (unsigned char *) (packet) < pc->buffer.data_end)


/*
 * Transmit a packet which was allocated with crPackAlloc.
 */
void crHugePacket( CR_PACKER_CONTEXT_ARGDECL CROpcode opcode, void *packet )
{
	CR_GET_PACKER_CONTEXT(pc);

	if ( IS_BUFFERED( packet ) )
		WRITE_OPCODE( pc, opcode );
	else
		pc->SendHuge( opcode, packet );
}

void crPackFree( CR_PACKER_CONTEXT_ARGDECL void *packet )
{
	CR_GET_PACKER_CONTEXT(pc);
    
	if ( IS_BUFFERED( packet ) )
    {
        CR_UNLOCK_PACKER_CONTEXT(pc);
		return;
    }

    CR_UNLOCK_PACKER_CONTEXT(pc);
	
	/* the pointer passed in doesn't include the space for the single
	 * opcode (4 bytes because of the alignment requirement) or the
	 * length field or the header */
	crFree( (unsigned char *) packet - 8 - sizeof(CRMessageOpcodes) );
}

void crNetworkPointerWrite( CRNetworkPointer *dst, void *src )
{
	/* init CRNetworkPointer with invalid values */
	dst->ptrAlign[0] = 0xDeadBeef;
	dst->ptrAlign[1] = 0xCafeBabe;
	/* copy the pointer's value into the CRNetworkPointer */
	crMemcpy( dst, &src, sizeof(src) );

	/* if either assertion fails, it probably means that a packer function
	 * (which returns a value) was called without setting up the writeback
	 * pointer, or something like that.
	 */
	CRASSERT(dst->ptrAlign[0] != 0xffffffff);
	CRASSERT(dst->ptrAlign[0] != 0xDeadBeef);
}
