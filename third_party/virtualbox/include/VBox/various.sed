
# Check for markers (typically in comments).
/ASM-INC/basm-inc
/ASM-NOINC/basm-noinc

# Newline escapes.
:check-newline-escape
/\\$/!bno-more-newline-escapes
N
b check-newline-escape
:no-more-newline-escapes

# Strip comments and trailing space.
s/[[:space:]][[:space:]]*\/\*.*$//g
s/[[:space:]][[:space:]]*\/\/.*$//g
s/[[:space:]][[:space:]]*$//g

# Try identify the statement.
/#[[:space:]]*define[[:space:]]/bdefine
/#[[:space:]]*ifdef[[:space:]]/bifdef
/#[[:space:]]*ifndef[[:space:]]/bifndef
/#[[:space:]]*if[[:space:]]/bif
/#[[:space:]]*elif[[:space:]]/belif
/#[[:space:]]*else$/belse
/#[[:space:]]*endif$/bendif

# Not recognized, drop it.
:asm-noinc
d
b end

#
# Defines needs some extra massaging to work in yasm.
# Things like trailing type indicators ('U', 'ULL' ++) does not go down well.
#
:define
/\$/d
s/#\([[:space:]]*\)define/\1%define/

s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)U$/\1/
s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)U\([[:space:]]*\))$/\1\2)/
s/\([[:space:]][0-9][0-9]*\)U[[:space:]]*$/\1/
s/\([[:space:]][0-9][0-9]*\)U\([[:space:]]*\))$/\1\2)/

s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)UL$/\1/
s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)UL\([[:space:]]*\))$/\1\2)/
s/\([[:space:]][0-9][0-9]*\)UL[[:space:]]*$/\1/
s/\([[:space:]][0-9][0-9]*\)UL\([[:space:]]*\))$/\1\2)/

s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)ULL$/\1/
s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)ULL\([[:space:]]*\))$/\1\2)/
s/\([[:space:]][0-9][0-9]*\)ULL[[:space:]]*$/\1/
s/\([[:space:]][0-9][0-9]*\)ULL\([[:space:]]*\))$/\1\2)/

s/UINT64_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT64_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/
s/UINT32_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT32_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/
s/UINT16_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT16_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/
s/UINT8_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT8_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/

b end

#
# Conditional statements, 1:1.
#
:ifdef
s/#\([[:space:]]*\)ifdef/\1%ifdef/
b end

:ifndef
s/#\([[:space:]]*\)ifndef/\1%ifndef/
b end

:if
s/#\([[:space:]]*\)if/\1%if/
b end

:elif
s/#\([[:space:]]*\)elif/\1%elif/
b end

:else
s/#\([[:space:]]*\)else.*$/\1%else/
b end

:endif
s/#\([[:space:]]*\)endif.*$/\1%endif/
b end

#
# Assembly statement... may need adjusting when used.
#
:asm-inc
b end

:end

