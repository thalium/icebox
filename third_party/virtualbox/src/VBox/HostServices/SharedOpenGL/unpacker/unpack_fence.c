
#include "unpacker.h"

void crUnpackExtendDeleteFencesNV(void)
{
	GLsizei n = READ_DATA( 8, GLsizei );
	const GLuint *fences = DATA_POINTER( 12, GLuint );
	cr_unpackDispatch.DeleteFencesNV( n, fences );
}

