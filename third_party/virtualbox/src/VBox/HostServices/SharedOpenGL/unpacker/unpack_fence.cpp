
#include "unpacker.h"

void crUnpackExtendDeleteFencesNV(PCrUnpackerState pState)
{
    CHECK_BUFFER_SIZE_STATIC_LAST(pState, 12, GLuint);
    GLsizei n = READ_DATA(pState, 8, GLsizei );
    const GLuint *fences = DATA_POINTER(pState, 12, GLuint );

    CHECK_ARRAY_SIZE_FROM_PTR_UPDATE_LAST(pState, fences, n, GLuint);

    pState->pDispatchTbl->DeleteFencesNV( n, fences );
}

