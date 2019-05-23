# !kmk_ash
# $Id: gen-slickedit-workspace.sh $
## @file
# Script for generating a SlickEdit workspace.
#

#
# Copyright (C) 2009-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

#
# Some constants.
#
MY_CAT="kmk_cat"
MY_CP="kmk_cp"
MY_MKDIR="kmk_mkdir"
MY_MV="kmk_mv"
MY_SED="kmk_sed"
MY_RM="kmk_rm"
MY_SLEEP="kmk_sleep"
MY_EXPR="kmk_expr"
MY_SVN="svn"

#MY_SORT="kmk_cat"
MY_SORT="sort"

#
# Globals.
#
MY_PROJECT_FILES=""
MY_OUT_DIRS="\
out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/${KBUILD_TYPE} \
out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/${BUILD_TYPE} \
out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/debug \
out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/debug \
out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/release \
out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/release \
out/linux.amd64/debug \
out/linux.x86/debug \
out/win.amd64/debug \
out/win.x86/debug \
out/darwin.amd64/debug \
out/darwin.x86/debug \
out/haiku.amd64/debug \
out/haiku.x86/debug \
out/solaris.amd64/debug \
out/solaris.x86/debug";


#
# Parameters w/ defaults.
#
MY_ROOT_DIR=".."
MY_OUT_DIR="SlickEdit"
MY_PRJ_PRF="VBox-"
MY_WS_NAME="VirtualBox.vpw"
MY_DBG=""
MY_WINDOWS_HOST=""
MY_OPT_MINIMAL=""
MY_OPT_USE_WILDCARDS="yes"

#MY_KBUILD_PATH="${KBUILD_PATH}"
#test -z "${MY_KBUILD_PATH}" && MY_KBUILD_PATH="${PATH_KBUILD}"
#MY_KBUILD=""


##
# Gets the absolute path to an existing directory.
#
# @param    $1      The path.
my_abs_dir()
{
    if test -n "${PWD}"; then
        MY_ABS_DIR=`cd ${MY_ROOT_DIR}/${1} && echo ${PWD}`
    else
        # old cygwin shell has no PWD and need adjusting.
        MY_ABS_DIR=`cd ${MY_ROOT_DIR}/${1} && pwd | ${MY_SED} -e 's,^/cygdrive/\(.\)/,\1:/,'`
    fi
    if test -z "${MY_ABS_DIR}"; then
        MY_ABS_DIR="${1}"
    fi
}

##
# Gets the file name part of a path.
#
# @param    $1      The path.
my_get_name()
{
    SAVED_IFS=$IFS
    IFS=":/ "
    set $1
    while test $# -gt 1  -a  -n "${2}";
    do
        shift;
    done

    IFS=$SAVED_IFS
    #echo "$1"
    export MY_GET_NAME=$1
}

##
# Generate file entry for the specified file if it was found to be of interest.
#
# @param    $1      The output file name base.
# @param    $2      The file name.
# @param    $3      Optional folder override.
my_file()
{
    # sort and filter by file type.
    case "$2" in
        # drop these.
        *.kup|*~|*.pyc|*.exe|*.sys|*.dll|*.o|*.obj|*.lib|*.a|*.ko)
            return 0
            ;;

        # by prefix or directory component.
        tst*|*/testcase/*)
            MY_FOLDER="$1-Testcases.lst"
            ;;

        # by extension.
        *.c|*.cpp|*.m|*.mm|*.pl|*.py|*.as|*.c.h|*.cpp.h|*.java)
            MY_FOLDER="$1-Sources.lst"
            ;;

        *.h|*.hpp|*.mm)
            MY_FOLDER="$1-Headers.lst"
            ;;

        *.asm|*.s|*.S|*.inc|*.mac)
            MY_FOLDER="$1-Assembly.lst"
            ;;

        *)
            MY_FOLDER="$1-Others.lst"
            ;;
    esac
    if test -n "$3";
    then
        MY_FOLDER="$1-$3.lst"
    fi

    ## @todo only files which are in subversion.
#    if ${MY_SVN} info "${2}" > /dev/null 2>&1; then
        my_get_name "${2}"
        echo ' <!-- sortkey: '"${MY_GET_NAME}"' --> <F N="'"${2}"'"/>' >> "${MY_FOLDER}"
#    fi
}

##
# Generate file entry for the specified file if it was found to be of interest.
#
# @param    $1      The output file name base.
# @param    $2      The wildcard spec.
# @param    $3      Optional folder override.
my_wildcard()
{
    if test -n "$3"; then
        MY_FOLDER="$1-$3.lst"
    else
        MY_FOLDER="$1-All.lst"
    fi
    EXCLUDES="*.log;*.kup;*~;*.pyc;*.exe;*.sys;*.dll;*.o;*.obj;*.lib;*.a;*.ko;*.class;*.cvsignore;*.done;*.project;*.actionScriptProperties;*.scm-settings;*.svnpatch.rej;*.svn-base;.svn/*;*.gitignore"
    echo '        <F N="'"${2}"'/*" Recurse="1" Excludes="'"${EXCLUDES}"'"/>' >> "${MY_FOLDER}"
}

##
# Generate file entries for the specified sub-directory tree.
#
# @param    $1      The output filename.
# @param    $2      The sub-directory.
# @param    $3      Optional folder override.
my_sub_tree()
{
    if test -n "$MY_DBG"; then echo "dbg: my_sub_tree: ${2}"; fi

    # Skip .svn directories.
    case "$2" in
        */.svn|*/.svn)
            return 0
            ;;
    esac

    # Process the files in the directory.
    for f in $2/*;
    do
        if test -d "${f}";
        then
            my_sub_tree "${1}" "${f}" "${3}"
        else
            my_file "${1}" "${f}" "${3}"
        fi
    done
    return 0;
}


##
# Generate folders for the specified directories and files.
#
# @param    $1      The output (base) file name.
# @param    $2+     The files and directories to traverse.
my_generate_folder()
{
    MY_FILE="$1"
    shift

    # Zap existing file collections.
    > "${MY_FILE}-All.lst"
    > "${MY_FILE}-Sources.lst"
    > "${MY_FILE}-Headers.lst"
    > "${MY_FILE}-Assembly.lst"
    > "${MY_FILE}-Testcases.lst"
    > "${MY_FILE}-Others.lst"

    # Traverse the directories and files.
    while test $# -ge 1  -a  -n "${1}";
    do
        for f in ${MY_ROOT_DIR}/$1;
        do
            if test -d "${f}";
            then
                if test -z "${MY_OPT_USE_WILDCARDS}";
                then
                    my_sub_tree "${MY_FILE}" "${f}"
                else
                    case "${f}" in
                        ${MY_ROOT_DIR}/include*)
                            #my_sub_tree "${MY_FILE}" "${f}" "Headers"
                            my_wildcard "${MY_FILE}" "${f}" "Headers"
                            ;;
                        *)  my_wildcard "${MY_FILE}" "${f}"
                            ;;
                    esac
                fi
            else
                my_file "${MY_FILE}" "${f}"
            fi
        done
        shift
    done

    # Generate the folders.
    if test -s "${MY_FILE}-All.lst";
    then
        ${MY_SORT} "${MY_FILE}-All.lst"   | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Sources.lst";
    then
        echo '        <Folder Name="Sources"  Filters="*.c;*.cpp;*.cpp.h;*.c.h;*.m;*.mm;*.pl;*.py;*.as">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Sources.lst"   | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Headers.lst";
    then
        if test -z "${MY_OPT_USE_WILDCARDS}";
        then
            echo '        <Folder Name="Headers"  Filters="*.h;*.hpp">' >> "${MY_FILE}"
        else
            echo '        <Folder Name="Headers"  Filters="">' >> "${MY_FILE}"
        fi
        ${MY_SORT} "${MY_FILE}-Headers.lst"   | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Assembly.lst";
    then
        echo '        <Folder Name="Assembly" Filters="*.asm;*.s;*.S;*.inc;*.mac">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Assembly.lst"  | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Testcases.lst";
    then
        echo '        <Folder Name="Testcases" Filters="tst*;">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Testcases.lst" | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi
    if test -s "${MY_FILE}-Others.lst";
    then
        echo '        <Folder Name="Others"   Filters="">' >> "${MY_FILE}"
        ${MY_SORT} "${MY_FILE}-Others.lst"    | ${MY_SED} -e 's/<!-- sortkey: [^>]*>/          /' >> "${MY_FILE}"
        echo '        </Folder>' >> "${MY_FILE}"
    fi

    # Cleanup
    ${MY_RM}  "${MY_FILE}-All.lst" "${MY_FILE}-Sources.lst" "${MY_FILE}-Headers.lst" "${MY_FILE}-Assembly.lst" \
        "${MY_FILE}-Testcases.lst" "${MY_FILE}-Others.lst"
}

##
# Function generating a project build config.
#
# @param    $1      The project file name.
# @param    $2      Build config name.
# @param    $3      Extra kBuild command line options, variant 1.
# @param    $4      Extra kBuild command line options, variant 2.
# @param    $4+     Include directories.
# @param    $N      --end-includes
my_generate_project_config()
{
    MY_FILE="${1}";
    MY_CFG_NAME="${2}";
    MY_KMK_EXTRAS1="${3}";
    MY_KMK_EXTRAS2="${4}";
    MY_KMK_EXTRAS3="${5}";
    MY_KMK_EXTRAS4="${6}";
    shift; shift; shift; shift; shift; shift;

    echo '    <Config Name="'"${MY_CFG_NAME}"'" OutputFile="" CompilerConfigName="Latest Version">'             >> "${MY_FILE}"
    echo '        <Menu>'                                                                                       >> "${MY_FILE}"

    echo '            <Target Name="Compile" MenuCaption="&amp;Compile" CaptureOutputWith="ProcessBuffer"'      >> "${MY_FILE}"
    echo '                    SaveOption="SaveCurrent" RunFromDir="%p" ClearProcessBuffer="1">'                 >> "${MY_FILE}"
    echo -n '                <Exec CmdLine="'"${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS1}"' -C %p %n.o'              >> "${MY_FILE}"
    if test -n "${MY_KMK_EXTRAS2}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS2} -C %p %n.o" >> "${MY_FILE}"
    fi
    if test -n "${MY_KMK_EXTRAS3}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS3} -C %p %n.o" >> "${MY_FILE}"
    fi
    if test -n "${MY_KMK_EXTRAS4}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS4} -C %p %n.o" >> "${MY_FILE}"
    fi
    echo  '"/>' >> "${MY_FILE}"
    echo '            </Target>'                                                                                >> "${MY_FILE}"

    echo '            <Target Name="Build" MenuCaption="&amp;Build"CaptureOutputWith="ProcessBuffer"'           >> "${MY_FILE}"
    echo '                    SaveOption="SaveWorkspaceFiles" RunFromDir="%rw" ClearProcessBuffer="1">'         >> "${MY_FILE}"
    echo -n '                <Exec CmdLine="'"${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS1}"' -C %rw'                  >> "${MY_FILE}"
    if test -n "${MY_KMK_EXTRAS2}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS2} -C %rw" >> "${MY_FILE}"
    fi
    if test -n "${MY_KMK_EXTRAS3}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS3} -C %rw" >> "${MY_FILE}"
    fi
    if test -n "${MY_KMK_EXTRAS4}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS4} -C %rw" >> "${MY_FILE}"
    fi
    echo  '"/>' >> "${MY_FILE}"
    echo '            </Target>'                                                                                >> "${MY_FILE}"

    echo '            <Target Name="Rebuild" MenuCaption="&amp;Rebuild" CaptureOutputWith="ProcessBuffer"'      >> "${MY_FILE}"
    echo '                    SaveOption="SaveWorkspaceFiles" RunFromDir="%rw" ClearProcessBuffer="1">'         >> "${MY_FILE}"
    echo -n '                <Exec CmdLine="'"${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS1}"' -C %rw'                  >> "${MY_FILE}"
    if test -n "${MY_KMK_EXTRAS2}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS2} -C %rw rebuild" >> "${MY_FILE}"
    fi
    if test -n "${MY_KMK_EXTRAS3}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS3} -C %rw rebuild" >> "${MY_FILE}"
    fi
    if test -n "${MY_KMK_EXTRAS4}"; then
        echo -n " && ${MY_KMK_INVOCATION} ${MY_KMK_EXTRAS4} -C %rw rebuild" >> "${MY_FILE}"
    fi
    echo  '"/>' >> "${MY_FILE}"
    echo '            </Target>'                                                                                >> "${MY_FILE}"

    #echo '            <Target Name="Debug" MenuCaption="&amp;Debug" SaveOption="SaveNone" RunFromDir="%rw">'    >> "${MY_FILE}"
    #echo '                <Exec/>'                                                                              >> "${MY_FILE}"
    #echo '            </Target>'                                                                                >> "${MY_FILE}"
    #echo '            <Target Name="Execute" MenuCaption="E&amp;xecute" SaveOption="SaveNone" RunFromDir="%rw">'>> "${MY_FILE}"
    #echo '                <Exec/>'                                                                              >> "${MY_FILE}"
    #echo '            </Target>'                                                                                >> "${MY_FILE}"
    echo '        </Menu>'                                                                                      >> "${MY_FILE}"

    #
    # Include directories.
    #
    echo '        <Includes>'                                                                                   >> "${MY_FILE}"
    while test $# -ge 1  -a  "${1}" != "--end-includes";
    do
        for f in $1;
        do
            my_abs_dir ${f}
            echo '            <Include Dir="'"${MY_ABS_DIR}/"'"/>'                                              >> "${MY_FILE}"
        done
        shift
    done
    shift
    echo '        </Includes>'                                                                                  >> "${MY_FILE}"
    echo '    </Config>'                                                                                        >> "${MY_FILE}"
}


##
# Function generating a project.
#
# @param    $1      The project file name.
# @param    $2      The project working directory.
# @param    $3      Dummy separator.
# @param    $4+     Include directories.
# @param    $N      --end-includes
# @param    $N+1    Directory sub-trees and files to include in the project.
#
my_generate_project()
{
    MY_FILE="${MY_PRJ_PRF}${1}.vpj";
    echo "Generating ${MY_FILE}..."
    MY_WRK_DIR="${2}"
    shift
    shift
    shift

    # Add it to the project list for workspace construction later on.
    MY_PROJECT_FILES="${MY_PROJECT_FILES} ${MY_FILE}"


    #
    # Generate the head bits.
    #
    echo '<!DOCTYPE Project SYSTEM "http://www.slickedit.com/dtd/vse/10.0/vpj.dtd">'                                    >  "${MY_FILE}"
    echo '<Project'                                                                                                     >> "${MY_FILE}"
    echo '        Version="10.0"'                                                                                       >> "${MY_FILE}"
    echo '        VendorName="SlickEdit"'                                                                               >> "${MY_FILE}"
    echo '        VCSProject="Subversion:"'                                                                             >> "${MY_FILE}"
#    echo '        Customized="1"'                                                                                       >> "${MY_FILE}"
#    echo '        WorkingDir="."'                                                                                       >> "${MY_FILE}"
    my_abs_dir "${MY_WRK_DIR}"                                                                                          >> "${MY_FILE}"
    echo '        WorkingDir="'"${MY_ABS_DIR}"'"'                                                                       >> "${MY_FILE}"
    echo '        >'                                                                                                    >> "${MY_FILE}"
    my_generate_project_config "${MY_FILE}" "Default" "" "" "" "" $*
    my_generate_project_config "${MY_FILE}" "Debug + hardening"     "KBUILD_TYPE=debug VBOX_WITH_HARDENING=1"   "" "" "" $*
    my_generate_project_config "${MY_FILE}" "Release + hardening"   "KBUILD_TYPE=release VBOX_WITH_HARDENING=1" "" "" "" $*
    my_generate_project_config "${MY_FILE}" "Debug+Release + hardening" \
        "KBUILD_TYPE=debug VBOX_WITH_HARDENING=1" \
        "KBUILD_TYPE=release VBOX_WITH_HARDENING=1" \
        "" "" $*
    my_generate_project_config "${MY_FILE}" "Debug w/o hardening"   "KBUILD_TYPE=debug VBOX_WITHOUT_HARDENING=1"   "" "" $*
    my_generate_project_config "${MY_FILE}" "Release w/o hardening" "KBUILD_TYPE=release VBOX_WITHOUT_HARDENING=1" "" "" $*
    my_generate_project_config "${MY_FILE}" "Debug+Release w/o hardening" \
        "KBUILD_TYPE=debug VBOX_WITHOUT_HARDENING=1" \
        "KBUILD_TYPE=release VBOX_WITHOUT_HARDENING=1" \
        "" "" $*
    my_generate_project_config "${MY_FILE}" "Debug+Release with and without hardening" \
        "KBUILD_TYPE=debug VBOX_WITH_HARDENING=1" \
        "KBUILD_TYPE=release VBOX_WITH_HARDENING=1" \
        "KBUILD_TYPE=debug VBOX_WITHOUT_HARDENING=1" \
        "KBUILD_TYPE=release VBOX_WITHOUT_HARDENING=1" \
        $*

    while test $# -ge 1  -a  "${1}" != "--end-includes";
    do
        shift;
    done;
    shift;

    #
    # Process directories+files and create folders.
    #
    echo '    <Files>'                                                                                                  >> "${MY_FILE}"
    my_generate_folder "${MY_FILE}" $*
    echo '    </Files>'                                                                                                 >> "${MY_FILE}"

    #
    # The tail.
    #
    echo '</Project>'                                                                                                   >> "${MY_FILE}"

    return 0
}


##
# Generate the workspace
#
my_generate_workspace()
{
    MY_FILE="${MY_WS_NAME}"
    echo "Generating ${MY_FILE}..."
    echo '<!DOCTYPE Workspace SYSTEM "http://www.slickedit.com/dtd/vse/10.0/vpw.dtd">'   > "${MY_FILE}"
    echo '<Workspace Version="10.0" VendorName="SlickEdit">'                            >> "${MY_FILE}"
    echo '    <Projects>'                                                               >> "${MY_FILE}"
    for i in ${MY_PROJECT_FILES};
    do
        echo '        <Project File="'"${i}"'" />'                                      >> "${MY_FILE}"
    done
    echo '    </Projects>'                                                              >> "${MY_FILE}"
    echo '</Workspace>'                                                                 >> "${MY_FILE}"
    return 0;
}


##
# Generate stuff
#
my_generate_usercpp_h()
{
    #
    # Probe the slickedit user config, picking the most recent version.
    #
    if test -z "${MY_SLICK_CONFIG}"; then
        if test -d "${HOME}/Library/Application Support/Slickedit"; then
            MY_SLICKDIR_="${HOME}/Library/Application Support/Slickedit"
            MY_USERCPP_H="unxcpp.h"
            MY_VSLICK_DB="vslick.stu"
        elif test -d "${HOMEDRIVE}${HOMEPATH}/Documents/My SlickEdit Config"; then
            MY_SLICKDIR_="${HOMEDRIVE}${HOMEPATH}/Documents/My SlickEdit Config"
            MY_USERCPP_H="usercpp.h"
            MY_VSLICK_DB="vslick.sta"
        else
            MY_SLICKDIR_="${HOME}/.slickedit"
            MY_USERCPP_H="unxcpp.h"
            MY_VSLICK_DB="vslick.stu"
        fi
    else
        MY_SLICKDIR_="${MY_SLICK_CONFIG}"
        if test -n "${MY_WINDOWS_HOST}"; then
            MY_USERCPP_H="usercpp.h"
            MY_VSLICK_DB="vslick.sta"
        else
            MY_USERCPP_H="unxcpp.h"
            MY_VSLICK_DB="vslick.stu"
        fi
        # MacOS: Implement me!
    fi

    MY_VER_NUM="0"
    MY_VER="0.0.0"
    for subdir in "${MY_SLICKDIR_}/"*;
    do
        if test -f "${subdir}/${MY_USERCPP_H}"  -o  -f "${subdir}/${MY_VSLICK_DB}"; then
            MY_CUR_VER_NUM=0
            MY_CUR_VER=`echo "${subdir}" | ${MY_SED} -e 's,^.*/,,g'`

            # Convert the dotted version number to an integer, checking that
            # it is all numbers in the process.
            set `echo "${MY_CUR_VER}" | ${MY_SED} 's/\./ /g' `
            i=100000000
            while test $# -gt 0;
            do
                if ! ${MY_EXPR} "$1" + 1 > /dev/null 2> /dev/null; then
                    MY_CUR_VER_NUM=0;
                    break
                fi
                if test "$i" -gt 0; then
                    MY_CUR_VER_NUM=$((${MY_CUR_VER_NUM} + $1 * $i))
                    i=$(($i / 100))
                fi
                shift
            done

            # More recent that what we have?
            if test "${MY_CUR_VER_NUM}" -gt "${MY_VER_NUM}"; then
                MY_VER_NUM="${MY_CUR_VER_NUM}"
                MY_VER="${MY_CUR_VER}"
            fi
        fi
    done

    MY_SLICKDIR="${MY_SLICKDIR_}/${MY_VER}"
    MY_USERCPP_H_FULL="${MY_SLICKDIR}/${MY_USERCPP_H}"
    if test -d "${MY_SLICKDIR}"; then
        echo "Found SlickEdit v${MY_VER} preprocessor file: ${MY_USERCPP_H_FULL}"
    else
        echo "Failed to locate SlickEdit preprocessor file. You need to manually merge ${MY_USERCPP_H}."
        MY_USERCPP_H_FULL=""
    fi

    # Generate our
    MY_FILE="${MY_USERCPP_H}"
    ${MY_CAT} > ${MY_FILE} <<EOF
#define IN_SLICKEDIT
#define RT_C_DECLS_BEGIN
#define RT_C_DECLS_END
#define RT_NO_THROW
#define RT_THROW(type) throw(type)
#define RT_GCC_EXTENSION
#define RT_COMPILER_GROKS_64BIT_BITFIELDS
#define RT_COMPILER_WITH_80BIT_LONG_DOUBLE

#define ATL_NO_VTABLE
#define BEGIN_COM_MAP(a)
#define COM_INTERFACE_ENTRY(a)
#define COM_INTERFACE_ENTRY2(a,b)
#define COM_INTERFACE_ENTRY3(a,b,c)
#define COM_INTERFACE_ENTRY4(a,b,c,d)
#define END_COM_MAP(a)

#define COM_DECL_READONLY_ENUM_AND_COLLECTION(a)
#define COMGETTER(n)                    n
#define COMSETTER(n)                    n
#define ComSafeArrayIn(t,a)             t a[]
#define ComSafeArrayOut(t,a)            t * a[]
#define DECLARE_CLASSFACTORY(a)
#define DECLARE_CLASSFACTORY_SINGLETON(a)
#define DECLARE_REGISTRY_RESOURCEID(a)
#define DECLARE_NOT_AGGREGATABLE(a)
#define DECLARE_PROTECT_FINAL_CONSTRUCT(a)
#define DECLARE_EMPTY_CTOR_DTOR(a)      a(); ~a();
#define DEFINE_EMPTY_CTOR_DTOR(a)       a::a() {}   a::~a() {}
#define NS_DECL_ISUPPORTS
#define NS_IMETHOD                      virtual nsresult
#define NS_IMETHOD_(type)               virtual type
#define NS_IMETHODIMP                   nsresult
#define NS_IMETHODIMP_(type)            type
#define PARSERS_EXPORT
EOF
    if test -n "${MY_WINDOWS_HOST}"; then
        ${MY_CAT} >> ${MY_FILE} <<EOF
#define COM_STRUCT_OR_CLASS(I)          struct I
#define STDMETHOD(m)                    virtual HRESULT m
#define STDMETHOD_(type,m)              virtual type m
#define STDMETHODIMP                    HRESULT
#define STDMETHODIMP_(type)             type
EOF
    else
        ${MY_CAT} >> ${MY_FILE} <<EOF
#define COM_STRUCT_OR_CLASS(I)          class I
#define STDMETHOD(m)                    virtual nsresult m
#define STDMETHOD_(type,m)              virtual type m
#define STDMETHODIMP                    nsresult
#define STDMETHODIMP_(type)             type
EOF
    fi
    ${MY_CAT} >> ${MY_FILE} <<EOF
#define VBOX_SCRIPTABLE(a)              public a
#define VBOX_SCRIPTABLE_IMPL(a)
#define VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(a)

#define CTX_SUFF(var)                   var##R3
#define CTXAllSUFF(var)                 var##R3
#define CTXSUFF(var)                    var##HC
#define OTHERCTXSUFF(var)               var##GC
#define CTXALLMID(first, last)          first##R3##last
#define CTXMID(first, last)             first##HC##last
#define OTHERCTXMID(first, last)        first##GC##last
#define CTXTYPE(GCType, R3Type, R0Type) R3Type
#define RCTYPE(RCType, HCType)          RCType
#define GCTYPE(GCType, HCType)          GCType
#define RCPTRTYPE(RCType)               RCType
#define GCPTRTYPE(GCType)               GCType
#define HCPTRTYPE(HCType)               HCType
#define R3R0PTRTYPE(HCType)             HCType
#define R0PTRTYPE(R3Type)               R3Type
#define R3PTRTYPE(R0Type)               R0Type
#define RT_SRC_POS                      __FILE__, __LINE__, __PRETTY_FUNCTION__
#define RT_SRC_POS_DECL                 const char *pszFile, unsigned iLine, const char *pszFunction
#define RT_SRC_POS_ARGS                 pszFile, iLine, pszFunction
#define RTCALL
#define RT_IPRT_FORMAT_ATTR(a_iFmt, a_iArgs)
#define RT_IPRT_FORMAT_ATTR_MAYBE_NULL(a_iFmt, a_iArgs)
#define DECLINLINE(type)                inline type
#define DECL_FORCE_INLINE(type)         inline type
#define DECL_INVALID(type)              type

#define PDMDEVINSINT_DECLARED           1
#define VBOX_WITH_HGCM                  1
#define VBOXCALL

#define PGM_ALL_CB_DECL(type)           type
#define PGM_ALL_CB2_DECL(type)          type
#define PGM_CTX(a,b)                    b
#define PGM_CTX3(a,b,c)                 c
#define PGM_GST_NAME(name)              PGM_GST_NAME_AMD64(name)
#define PGM_GST_NAME_REAL(name)         PGM_CTX3(name)
#define PGM_GST_NAME_PROT(name)         PGM_CTX3(pgm,GstProt,name)
#define PGM_GST_NAME_32BIT(name)        PGM_CTX3(pgm,Gst32Bit,name)
#define PGM_GST_NAME_PAE(name)          PGM_CTX3(pgm,GstPAE,name)
#define PGM_GST_NAME_AMD64(name)        PGM_CTX3(pgm,GstAMD64,name)
#define PGM_GST_DECL(type, name)        type PGM_GST_NAME(name)
#define PGM_SHW_NAME(name)              PGM_GST_NAME_AMD64(name)
#define PGM_SHW_NAME_32BIT(name)        PGM_CTX3(pgm,Shw32Bit,name)
#define PGM_SHW_NAME_PAE(name)          PGM_CTX3(pgm,ShwPAE,name)
#define PGM_SHW_NAME_AMD64(name)        PGM_CTX3(pgm,ShwAMD64,name)
#define PGM_SHW_NAME_NESTED(name)       PGM_CTX3(pgm,ShwNested,name)
#define PGM_SHW_NAME_EPT(name)          PGM_CTX3(pgm,ShwEPT,name)
#define PGM_SHW_DECL(type, name)        type PGM_SHW_NAME(name)
#define PGM_BTH_NAME(name)              PGM_BTH_NAME_NESTED_AMD64(name)
#define PGM_BTH_NAME_32BIT_REAL(name)   PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_32BIT_PROT(name)   PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_32BIT_32BIT(name)  PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_PAE_REAL(name)     PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_PAE_PROT(name)     PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_PAE_32BIT(name)    PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_PAE_PAE(name)      PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_AMD64_PROT(name)   PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_AMD64_AMD64(name)  PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_NESTED_REAL(name)  PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_NESTED_PROT(name)  PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_NESTED_32BIT(name) PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_NESTED_PAE(name)   PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_NESTED_AMD64(name) PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_EPT_REAL(name)     PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_EPT_PROT(name)     PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_EPT_32BIT(name)    PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_EPT_PAE(name)      PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_NAME_EPT_AMD64(name)    PGM_CTX3(pgm,Bth,name)
#define PGM_BTH_DECL(type, name)        type PGM_BTH_NAME(name)

#define FNIEMOP_STUB(a_Name)               VBOXSTRICTRC a_Name(PIEMCPU pIemCpu) { return VERR_NOT_IMPLEMENTED; }
#define FNIEMOP_DEF(a_Name)                VBOXSTRICTRC a_Name(PIEMCPU pIemCpu)
#define FNIEMOP_DEF_1(a_Name, a_Type0, a_Name0) static VBOXSTRICTRC a_Name(PIEMCPU pIemCpu, a_Type0 a_Name0)
#define FNIEMOP_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) static VBOXSTRICTRC a_Name(PIEMCPU pIemCpu, a_Type0 a_Name0, a_Type1 a_Name1)
#define IEM_CIMPL_DEF_0(a_Name)         static VBOXSTRICTRC a_Name(PIEMCPU pIemCpu)
#define IEM_CIMPL_DEF_1(a_Name, a_Type0, a_Name0) static VBOXSTRICTRC a_Name(PIEMCPU pIemCpu, , a_Type0 a_Name0)
#define IEM_CIMPL_DEF_2(a_Name, a_Type0, a_Name0, a_Type1, a_Name1) static VBOXSTRICTRC a_Name(PIEMCPU pIemCpu, , a_Type0 a_Name0, a_Type1 a_Name1)
#define IEM_CIMPL_DEF_3(a_Name, a_Type0, a_Name0, a_Type1, a_Name1, a_Type2, a_Name2)  static VBOXSTRICTRC a_Name(PIEMCPU pIemCpu, , a_Type0 a_Name0, a_Type1 a_Name1, , a_Type2 a_Name2)
#define IEM_DECL_IMPL_DEF(a_RetType, a_Name, a_ArgList)   a_RetType a_Name a_ArgList
#define IEM_MC_LOCAL(a_Type, a_Name)                       a_Type a_Name
#define IEM_MC_ARG(a_Type, a_Name, a_iArg)                 a_Type a_Name
#define IEM_MC_ARG_CONST(a_Type, a_Name, a_Value, a_iArg)  a_Type const a_Name = a_Value
#define IEM_STATIC

#define RTASN1_IMPL_GEN_SEQ_OF_TYPEDEFS_AND_PROTOS(a_SeqOfType, a_ItemType, a_DeclMacro, a_ImplExtNm) typedef struct a_SeqOfType { RTASN1SEQUENCECORE SeqCore; RTASN1ALLOCATION Allocation; uint32_t cItems; RT_CONCAT(P,a_ItemType) paItems; } a_SeqOfType; typedef a_SeqOfType *P##a_SeqOfType, const *PC##a_SeqOfType; int a_ImplExtNm##_DecodeAsn1(struct RTASN1CURSOR *pCursor, uint32_t fFlags, P##a_SeqOfType pThis, const char *pszErrorTag); int a_ImplExtNm##_Compare(PC##a_SeqOfType pLeft, PC##a_SeqOfType pRight)
#define RTASN1_IMPL_GEN_SET_OF_TYPEDEFS_AND_PROTOS(a_SetOfType, a_ItemType, a_DeclMacro, a_ImplExtNm) typedef struct a_SetOfType { RTASN1SETCORE SetCore; RTASN1ALLOCATION Allocation; uint32_t cItems; RT_CONCAT(P,a_ItemType) paItems; } a_SetOfType; typedef a_SetOfType *P##a_SetOfType, const *PC##a_SetOfType; int a_ImplExtNm##_DecodeAsn1(struct RTASN1CURSOR *pCursor, uint32_t fFlags, P##a_SetOfType pThis, const char *pszErrorTag); int a_ImplExtNm##_Compare(PC##a_SetOfType pLeft, PC##a_SetOfType pRight)
#define RTASN1TYPE_STANDARD_PROTOTYPES_NO_GET_CORE(a_TypeNm, a_DeclMacro, a_ImplExtNm) int  a_ImplExtNm##_Init(P##a_TypeNm pThis, PCRTASN1ALLOCATORVTABLE pAllocator); int  a_ImplExtNm##_Clone(P##a_TypeNm pThis, PC##a_TypeNm) pSrc, PCRTASN1ALLOCATORVTABLE pAllocator); void a_ImplExtNm##_Delete(P##a_TypeNm pThis); int  a_ImplExtNm##_Enum(P##a_TypeNm pThis, PFNRTASN1ENUMCALLBACK pfnCallback, uint32_t uDepth, void *pvUser); int  a_ImplExtNm##_Compare(PC##a_TypeNm) pLeft, PC##a_TypeNm pRight); int  a_ImplExtNm##_DecodeAsn1(PRTASN1CURSOR pCursor, uint32_t fFlags, P##a_TypeNm pThis, const char *pszErrorTag); int  a_ImplExtNm##_CheckSanity(PC##a_TypeNm pThis, uint32_t fFlags, PRTERRINFO pErrInfo, const char *pszErrorTag)
#define RTASN1TYPE_STANDARD_PROTOTYPES(a_TypeNm, a_DeclMacro, a_ImplExtNm, a_Asn1CoreNm) inline PRTASN1CORE a_ImplExtNm##_GetAsn1Core(PC##a_TypeNm pThis) { return (PRTASN1CORE)&pThis->a_Asn1CoreNm; } inline bool a_ImplExtNm##_IsPresent(PC##a_TypeNm pThis) { return pThis && RTASN1CORE_IS_PRESENT(&pThis->a_Asn1CoreNm); } RTASN1TYPE_STANDARD_PROTOTYPES_NO_GET_CORE(a_TypeNm, a_DeclMacro, a_ImplExtNm)

#define BS3_DECL(type)                  type
#define BS3_DECL_CALLBACK(type)         type
#define TMPL_NM(name)                   name##_mmm
#define TMPL_FAR_NM(name)               name##_mmm_far
#define BS3_CMN_NM(name)                name
#define BS3_CMN_FAR_NM(name)            name
#define BS3_CMN_FN_NM(name)             name
#define BS3_DATA_NM(name)               name
#define BS3_FAR
#define BS3_FAR_CODE
#define BS3_FAR_DATA
#define BS3_NEAR
#define BS3_NEAR_CODE
#define BS3_CMN_PROTO_STUB(a_RetType, a_Name, a_Params) a_RetType a_Name a_Params
#define BS3_CMN_PROTO_NOSB(a_RetType, a_Name, a_Params) a_RetType a_Name a_Params
#define BS3_CMN_DEF(       a_RetType, a_Name, a_Params) a_RetType a_Name a_Params
#define BS3_MODE_PROTO_STUB(a_RetType, a_Name, a_Params) \
    a_RetType a_Name##_mmm           a_Params; \
    a_RetType a_Name##_mmm_far       a_Params; \
    a_RetType a_Name##_rm            a_Params; \
    a_RetType a_Name##_pe16          a_Params; \
    a_RetType a_Name##_pe16_32       a_Params; \
    a_RetType a_Name##_pe16_v86      a_Params; \
    a_RetType a_Name##_pe32          a_Params; \
    a_RetType a_Name##_pe32_16       a_Params; \
    a_RetType a_Name##_pev86         a_Params; \
    a_RetType a_Name##_pp16          a_Params; \
    a_RetType a_Name##_pp16_32       a_Params; \
    a_RetType a_Name##_pp16_v86      a_Params; \
    a_RetType a_Name##_pp32          a_Params; \
    a_RetType a_Name##_pp32_16       a_Params; \
    a_RetType a_Name##_ppv86         a_Params; \
    a_RetType a_Name##_pae16         a_Params; \
    a_RetType a_Name##_pae16_32      a_Params; \
    a_RetType a_Name##_pae16_v86     a_Params; \
    a_RetType a_Name##_pae32         a_Params; \
    a_RetType a_Name##_pae32_16      a_Params; \
    a_RetType a_Name##_paev86        a_Params; \
    a_RetType a_Name##_lm16          a_Params; \
    a_RetType a_Name##_lm32          a_Params; \
    a_RetType a_Name##_lm64          a_Params; \
    a_RetType a_Name##_rm_far        a_Params; \
    a_RetType a_Name##_pe16_far      a_Params; \
    a_RetType a_Name##_pe16_v86_far  a_Params; \
    a_RetType a_Name##_pe32_16_far   a_Params; \
    a_RetType a_Name##_pev86_far     a_Params; \
    a_RetType a_Name##_pp16_far      a_Params; \
    a_RetType a_Name##_pp16_v86_far  a_Params; \
    a_RetType a_Name##_pp32_16_far   a_Params; \
    a_RetType a_Name##_ppv86_far     a_Params; \
    a_RetType a_Name##_pae16_far     a_Params; \
    a_RetType a_Name##_pae16_v86_far a_Params; \
    a_RetType a_Name##_pae32_16_far  a_Params; \
    a_RetType a_Name##_paev86_far    a_Params; \
    a_RetType a_Name##_lm16_far      a_Params
#define BS3_MODE_PROTO_NOSB(a_RetType, a_Name, a_Params) \
    a_RetType a_Name##_mmm           a_Params; \
    a_RetType a_Name##_mmm_far       a_Params; \
    a_RetType a_Name##_rm            a_Params; \
    a_RetType a_Name##_pe16          a_Params; \
    a_RetType a_Name##_pe16_32       a_Params; \
    a_RetType a_Name##_pe16_v86      a_Params; \
    a_RetType a_Name##_pe32          a_Params; \
    a_RetType a_Name##_pe32_16       a_Params; \
    a_RetType a_Name##_pev86         a_Params; \
    a_RetType a_Name##_pp16          a_Params; \
    a_RetType a_Name##_pp16_32       a_Params; \
    a_RetType a_Name##_pp16_v86      a_Params; \
    a_RetType a_Name##_pp32          a_Params; \
    a_RetType a_Name##_pp32_16       a_Params; \
    a_RetType a_Name##_ppv86         a_Params; \
    a_RetType a_Name##_pae16         a_Params; \
    a_RetType a_Name##_pae16_32      a_Params; \
    a_RetType a_Name##_pae16_v86     a_Params; \
    a_RetType a_Name##_pae32         a_Params; \
    a_RetType a_Name##_pae32_16      a_Params; \
    a_RetType a_Name##_paev86        a_Params; \
    a_RetType a_Name##_lm16          a_Params; \
    a_RetType a_Name##_lm32          a_Params; \
    a_RetType a_Name##_lm64          a_Params; \
    a_RetType a_Name##_rm_far        a_Params; \
    a_RetType a_Name##_pe16_far      a_Params; \
    a_RetType a_Name##_pe16_v86_far  a_Params; \
    a_RetType a_Name##_pe32_16_far   a_Params; \
    a_RetType a_Name##_pev86_far     a_Params; \
    a_RetType a_Name##_pp16_far      a_Params; \
    a_RetType a_Name##_pp16_v86_far  a_Params; \
    a_RetType a_Name##_pp32_16_far   a_Params; \
    a_RetType a_Name##_ppv86_far     a_Params; \
    a_RetType a_Name##_pae16_far     a_Params; \
    a_RetType a_Name##_pae16_v86_far a_Params; \
    a_RetType a_Name##_pae32_16_far  a_Params; \
    a_RetType a_Name##_paev86_far    a_Params; \
    a_RetType a_Name##_lm16_far      a_Params
#define BS3_MODE_EXPAND_EXTERN_DATA16(a_VarType, a_VarName, a_Suffix) \
    extern a_VarType a_VarName##_rm        a_Suffix; \
    extern a_VarType a_VarName##_pe16      a_Suffix; \
    extern a_VarType a_VarName##_pe16_32   a_Suffix; \
    extern a_VarType a_VarName##_pe16_v86  a_Suffix; \
    extern a_VarType a_VarName##_pe32      a_Suffix; \
    extern a_VarType a_VarName##_pe32_16   a_Suffix; \
    extern a_VarType a_VarName##_pev86     a_Suffix; \
    extern a_VarType a_VarName##_pp16      a_Suffix; \
    extern a_VarType a_VarName##_pp16_32   a_Suffix; \
    extern a_VarType a_VarName##_pp16_v86  a_Suffix; \
    extern a_VarType a_VarName##_pp32      a_Suffix; \
    extern a_VarType a_VarName##_pp32_16   a_Suffix; \
    extern a_VarType a_VarName##_ppv86     a_Suffix; \
    extern a_VarType a_VarName##_pae16     a_Suffix; \
    extern a_VarType a_VarName##_pae16_32  a_Suffix; \
    extern a_VarType a_VarName##_pae16_v86 a_Suffix; \
    extern a_VarType a_VarName##_pae32     a_Suffix; \
    extern a_VarType a_VarName##_pae32_16  a_Suffix; \
    extern a_VarType a_VarName##_paev86    a_Suffix; \
    extern a_VarType a_VarName##_lm16      a_Suffix; \
    extern a_VarType a_VarName##_lm32      a_Suffix; \
    extern a_VarType a_VarName##_lm64      a_Suffix

EOF

    MY_HDR_FILES=`  echo ${MY_ROOT_DIR}/include/VBox/*.h ${MY_ROOT_DIR}/include/VBox/vmm/*.h \
                  | ${MY_SED} -e 's,${MY_ROOT_DIR}/include/VBox/err.h,,' `
    MY_HDR_FILES="${MY_HDR_FILES} ${MY_ROOT_DIR}/include/iprt/cdefs.h"
    ${MY_SED} \
        -e '/__cdecl/d' \
        -e '/^ *# *define.*DECL/!d' \
        -e '/DECLS/d' \
        -e '/DECLARE_CLS_/d' \
        -e '/_SRC_POS_DECL/d' \
        -e '/declspec/d' \
        -e '/__attribute__/d' \
        -e 's/#  */#/g' \
        -e 's/   */ /g' \
        -e '/ DECLEXPORT_CLASS/d' \
        -e 's/ *VBOXCALL//' \
        -e 's/ *RTCALL//' \
        -e '/ DECLASM(type) type/d' \
        -e '/define  *DECL..CALLBACKMEMBER(type[^)]*) *RT/d' \
        -e '/define  *DECLINLINE(type)/d' \
        -e '/define  *DECL_FORCE_INLINE(type)/d' \
        -e '/  *DECL_INVALID(/d' \
        \
        -e 's/(type) DECLHIDDEN(type)/(type) type/' \
        -e 's/(type) DECLEXPORT(type)/(type) type/' \
        -e 's/(type) DECLIMPORT(type)/(type) type/' \
        -e 's/(a_Type) DECLHIDDEN(a_Type)/(a_Type) a_Type/' \
        -e 's/(a_Type) DECLEXPORT(a_Type)/(a_Type) a_Type/' \
        -e 's/(a_Type) DECLIMPORT(a_Type)/(a_Type) a_Type/' \
        \
        --append "${MY_FILE}" \
        ${MY_HDR_FILES}

    ${MY_CAT} "${MY_FILE}" \
        | ${MY_SED} -e 's/_/\x1F/g' -e 's/(/\x1E/g' -e 's/[[:space:]][[:space:]]*/\x1C/g' \
        | ${MY_SED} -e 's/\x1F/_/g' -e 's/\x1E/(/g' -e 's/\x1C/ /g' \
        | ${MY_SORT} \
        | ${MY_SED} -e '/#define/s/$/ \/\/ vbox/' --output "${MY_FILE}.2"

    # Eliminate duplicates.
    > "${MY_FILE}.3"
    exec < "${MY_FILE}.2"
    MY_PREV_DEFINE=""
    while read MY_LINE;
    do
        MY_DEFINE=`echo "${MY_LINE}" | ${MY_SED} -e 's/^#define \([^ ()]*\).*$/\1/' `
        if test "${MY_DEFINE}" != "${MY_PREV_DEFINE}"; then
            MY_PREV_DEFINE=${MY_DEFINE}
            echo "${MY_LINE}" >> "${MY_FILE}.3"
        fi
    done

    # Append non-vbox bits from the current user config.
    if test -n "${MY_USERCPP_H_FULL}"  -a  -f "${MY_USERCPP_H_FULL}"; then
        ${MY_SED} -e '/ \/\/ vbox$/d' -e '/^[[:space:]]*$/d' --append "${MY_FILE}.3" "${MY_USERCPP_H_FULL}"
    fi

    # Finalize the file (sort + blank lines).
    ${MY_CAT} "${MY_FILE}.3" \
        | ${MY_SED} -e 's/$/\n/' --output "${MY_FILE}"
    ${MY_RM} -f "${MY_FILE}.2" "${MY_FILE}.3"

    # Install it.
    if test -n "${MY_USERCPP_H_FULL}"  -a  -d "${MY_SLICKDIR}"; then
        if test -f "${MY_USERCPP_H_FULL}"; then
            ${MY_MV} -vf "${MY_USERCPP_H_FULL}" "${MY_USERCPP_H_FULL}.bak"
            ${MY_CP} "${MY_FILE}" "${MY_USERCPP_H_FULL}"
            echo "Updated the SlickEdit preprocessor file. (Previous version renamed to .bak.)"
        else
            ${MY_CP} "${MY_FILE}" "${MY_USERCPP_H_FULL}"
            echo "Created the SlickEdit preprocessor file."
        fi
    fi
}

###### end of functions ####


#
# Parse arguments.
#
while test $# -ge 1;
do
    ARG=$1
    shift
    case "$ARG" in

        --rootdir)
            if test $# -eq 0; then
                echo "error: missing --rootdir argument." 1>&2
                exit 1;
            fi
            MY_ROOT_DIR="$1"
            shift
            ;;

        --outdir)
            if test $# -eq 0; then
                echo "error: missing --outdir argument." 1>&2
                exit 1;
            fi
            MY_OUT_DIR="$1"
            shift
            ;;

        --project-base)
            if test $# -eq 0; then
                echo "error: missing --project-base argument." 1>&2
                exit 1;
            fi
            MY_PRJ_PRF="$1"
            shift
            ;;

        --workspace)
            if test $# -eq 0; then
                echo "error: missing --workspace argument." 1>&2
                exit 1;
            fi
            MY_WS_NAME="$1"
            shift
            ;;

        --windows-host)
            MY_WINDOWS_HOST=1
            ;;

        --minimal)
            MY_OPT_MINIMAL=1
            ;;

        --slickedit-config)
            MY_SLICK_CONFIG="$1"
            shift
            ;;

        # usage
        --h*|-h*|-?|--?)
            echo "usage: $0 [--rootdir <rootdir>] [--outdir <outdir>] [--project-base <prefix>] [--workspace <name>] [--minimal] [--slickedit-config <DIR>]"
            echo ""
            echo "If --outdir is specified, you must specify a --rootdir relative to it as well."
            exit 1;
            ;;

        # default
        *)
            echo "error: Invalid parameter '$ARG'" 1>&2
            exit 1;

    esac
done


#
# From now on everything *MUST* succeed.
#
set -e


#
# Make sure the output directory exists, is valid and clean.
#
${MY_RM} -f \
    "${MY_OUT_DIR}/${MY_PRJ_PRF}"*.vpj \
    "${MY_OUT_DIR}/${MY_WS_NAME}" \
    "${MY_OUT_DIR}/`echo ${MY_WS_NAME} | ${MY_SED} -e 's/\.vpw$/.vtg/'`"
${MY_MKDIR} -p "${MY_OUT_DIR}"
cd "${MY_OUT_DIR}"


#
# Determine the invocation to conjure up kmk.
#
my_abs_dir "tools"
if test -n "${MY_WINDOWS_HOST}"; then
    MY_KMK_INVOCATION="${MY_ABS_DIR}/win.x86/bin/rexx.exe ${MY_ABS_DIR}/envSub.cmd kmk.exe"
else
    MY_KMK_INVOCATION="/usr/bin/env LANG=C ${MY_ABS_DIR}/env.sh --quiet --no-wine kmk"
fi


#
# Generate the projects and workspace.
#
# Note! The configs aren't optimal yet, lots of adjustment wrt headers left to be done.
#

# src/VBox/Runtime
my_generate_project "IPRT"          "src/VBox/Runtime"                      --begin-incs "include" "src/VBox/Runtime/include"               --end-includes "include/iprt" "src/VBox/Runtime"

# src/VBox/VMM
my_generate_project "VMM"           "src/VBox/VMM"                          --begin-incs "include" "src/VBox/VMM"                           --end-includes "src/VBox/VMM" \
    "include/VBox/vmm/cfgm.h" \
    "include/VBox/vmm/cpum.*" \
    "include/VBox/vmm/dbgf.h" \
    "include/VBox/vmm/em.h" \
    "include/VBox/vmm/gim.h" \
    "include/VBox/vmm/apic.h" \
    "include/VBox/vmm/gmm.*" \
    "include/VBox/vmm/gvm.*" \
    "include/VBox/vmm/hm*.*" \
    "include/VBox/vmm/iom.h" \
    "include/VBox/vmm/mm.h" \
    "include/VBox/vmm/patm.*" \
    "include/VBox/vmm/pdm*.h" \
    "include/VBox/vmm/pgm.*" \
    "include/VBox/vmm/rem.h" \
    "include/VBox/vmm/selm.*" \
    "include/VBox/vmm/ssm.h" \
    "include/VBox/vmm/stam.*" \
    "include/VBox/vmm/tm.h" \
    "include/VBox/vmm/trpm.*" \
    "include/VBox/vmm/vm.*" \
    "include/VBox/vmm/vmm.*"

# src/recompiler
my_generate_project "REM"           "src/recompiler"                        --begin-incs \
    "include" \
    "src/recompiler" \
    "src/recompiler/target-i386" \
    "src/recompiler/tcg/i386" \
    "src/recompiler/Sun/crt" \
    --end-includes \
    "src/recompiler" \
    "src/VBox/VMM/include/REMInternal.h" \
    "src/VBox/VMM/VMMAll/REMAll.cpp"

# src/VBox/Additions
my_generate_project "Add-freebsd"   "src/VBox/Additions/freebsd"            --begin-incs "include" "src/VBox/Additions/freebsd"             --end-includes "src/VBox/Additions/freebsd"
my_generate_project "Add-linux"     "src/VBox/Additions/linux"              --begin-incs "include" "src/VBox/Additions/linux"               --end-includes "src/VBox/Additions/linux"
my_generate_project "Add-os2"       "src/VBox/Additions/os2"                --begin-incs "include" "src/VBox/Additions/os2"                 --end-includes "src/VBox/Additions/os2"
my_generate_project "Add-solaris"   "src/VBox/Additions/solaris"            --begin-incs "include" "src/VBox/Additions/solaris"             --end-includes "src/VBox/Additions/solaris"
my_generate_project "Add-haiku"     "src/VBox/Additions/haiku"              --begin-incs "include" "src/VBox/Additions/haiku"               --end-includes "src/VBox/Additions/haiku"
my_generate_project "Add-win"       "src/VBox/Additions/WINNT"              --begin-incs "include" "src/VBox/Additions/WINNT"               --end-includes "src/VBox/Additions/WINNT"
test -z "$MY_OPT_MINIMAL" && \
my_generate_project "Add-x11"       "src/VBox/Additions/x11"                  --begin-incs "include" "src/VBox/Additions/x11"                 --end-includes "src/VBox/Additions/x11"
my_generate_project "Add-Control"   "src/VBox/Additions/common/VBoxControl"   --begin-incs "include" "src/VBox/Additions/common/VBoxControl"  --end-includes "src/VBox/Additions/common/VBoxControl"
my_generate_project "Add-GuestDrv"  "src/VBox/Additions/common/VBoxGuest"     --begin-incs "include" "src/VBox/Additions/common/VBoxGuest"    --end-includes "src/VBox/Additions/common/VBoxGuest"      "include/VBox/VBoxGuest*.*"
my_generate_project "Add-Lib"       "src/VBox/Additions/common/VBoxGuest/lib" --begin-incs "include" "src/VBox/Additions/common/VBoxGuest/lib" --end-includes "src/VBox/Additions/common/VBoxGuest/lib" "include/VBox/VBoxGuest/lib/*.*"
my_generate_project "Add-Service"   "src/VBox/Additions/common/VBoxService"   --begin-incs "include" "src/VBox/Additions/common/VBoxService"  --end-includes "src/VBox/Additions/common/VBoxService"
if test -z "$MY_OPT_MINIMAL"; then
    my_generate_project "Add-crOpenGL"  "src/VBox/Additions/common/crOpenGL"    --begin-incs "include" "src/VBox/Additions/common/crOpenGL"     --end-includes "src/VBox/Additions/common/crOpenGL"
    my_generate_project "Add-CredProv"  "src/VBox/Additions/WINNT/VBoxCredProv" --begin-incs "include" "src/VBox/Additions/WINNT/VBoxCredProv"  --end-includes "src/VBox/Additions/WINNT/VBoxCredProv"
    my_generate_project "Add-GINA"      "src/VBox/Additions/WINNT/VBoxGINA"     --begin-incs "include" "src/VBox/Additions/WINNT/VBoxGINA"      --end-includes "src/VBox/Additions/WINNT/VBoxGINA"
fi

# src/VBox/Debugger
my_generate_project "Debugger"      "src/VBox/Debugger"                     --begin-incs "include" "src/VBox/Debugger"                      --end-includes "src/VBox/Debugger" "include/VBox/dbggui.h" "include/VBox/dbg.h"

# src/VBox/Devices
my_generate_project "Devices"       "src/VBox/Devices"                      --begin-incs "include" "src/VBox/Devices"                       --end-includes "src/VBox/Devices" "include/VBox/pci.h" "include/VBox/pdm*.h"
## @todo split this up.

# src/VBox/Disassembler
my_generate_project "DIS"           "src/VBox/Disassembler"                 --begin-incs "include" "src/VBox/Disassembler"                  --end-includes "src/VBox/Disassembler" "include/VBox/dis*.h"

# src/VBox/Frontends
test -z "$MY_OPT_MINIMAL" && \
my_generate_project "FE-VBoxBalloonCtrl" "src/VBox/Frontends/VBoxBalloonCtrl" --begin-incs "include" "src/VBox/Frontends/VBoxBalloonCtrl"     --end-includes "src/VBox/Frontends/VBoxBalloonCtrl"
my_generate_project "FE-VBoxManage"      "src/VBox/Frontends/VBoxManage"      --begin-incs "include" "src/VBox/Frontends/VBoxManage"          --end-includes "src/VBox/Frontends/VBoxManage"
my_generate_project "FE-VBoxHeadless"    "src/VBox/Frontends/VBoxHeadless"    --begin-incs "include" "src/VBox/Frontends/VBoxHeadless"        --end-includes "src/VBox/Frontends/VBoxHeadless"
my_generate_project "FE-VBoxSDL"         "src/VBox/Frontends/VBoxSDL"         --begin-incs "include" "src/VBox/Frontends/VBoxSDL"             --end-includes "src/VBox/Frontends/VBoxSDL"
my_generate_project "FE-VBoxShell"       "src/VBox/Frontends/VBoxShell"       --begin-incs "include" "src/VBox/Frontends/VBoxShell"           --end-includes "src/VBox/Frontends/VBoxShell"
# noise - my_generate_project "FE-VBoxBFE"      "src/VBox/Frontends/VBoxBFE"      --begin-incs "include" "src/VBox/Frontends/VBoxBFE"            --end-includes "src/VBox/Frontends/VBoxBFE"
FE_VBOX_WRAPPERS=""
for d in ${MY_OUT_DIRS};
do
    if test -d "${MY_ROOT_DIR}/${d}/obj/VirtualBox/include"; then
        FE_VBOX_WRAPPERS="${d}/obj/VirtualBox/include"
        break
    fi
done
if test -n "${FE_VBOX_WRAPPERS}"; then
    my_generate_project "FE-VirtualBox" "src/VBox/Frontends/VirtualBox"     --begin-incs "include" "${FE_VBOX_WRAPPERS}"                    --end-includes "src/VBox/Frontends/VirtualBox" "${FE_VBOX_WRAPPERS}/COMWrappers.cpp" "${FE_VBOX_WRAPPERS}/COMWrappers.h"
else
    my_generate_project "FE-VirtualBox" "src/VBox/Frontends/VirtualBox"     --begin-incs "include"                                          --end-includes "src/VBox/Frontends/VirtualBox"
fi

# src/VBox/GuestHost
my_generate_project "HGSMI-GH"      "src/VBox/GuestHost/HGSMI"              --begin-incs "include"                                          --end-includes "src/VBox/GuestHost/HGSMI"
test -z "$MY_OPT_MINIMAL" && \
my_generate_project "DnD-GH"        "src/VBox/GuestHost/DragAndDrop"        --begin-incs "include"                                          --end-includes "src/VBox/GuestHost/DragAndDrop"
my_generate_project "OpenGL-GH"     "src/VBox/GuestHost/OpenGL"             --begin-incs "include" "src/VBox/GuestHost/OpenGL"              --end-includes "src/VBox/GuestHost/OpenGL"
my_generate_project "ShClip-GH"     "src/VBox/GuestHost/SharedClipboard"    --begin-incs "include"                                          --end-includes "src/VBox/GuestHost/SharedClipboard"

# src/VBox/HostDrivers
my_generate_project "SUP"           "src/VBox/HostDrivers/Support"          --begin-incs "include" "src/VBox/HostDrivers/Support"           --end-includes "src/VBox/HostDrivers/Support" "include/VBox/sup.h" "include/VBox/sup.mac"
my_generate_project "VBoxNetAdp"    "src/VBox/HostDrivers/VBoxNetAdp"       --begin-incs "include" "src/VBox/HostDrivers/VBoxNetAdp"        --end-includes "src/VBox/HostDrivers/VBoxNetAdp" "include/VBox/intnet.h"
my_generate_project "VBoxNetFlt"    "src/VBox/HostDrivers/VBoxNetFlt"       --begin-incs "include" "src/VBox/HostDrivers/VBoxNetFlt"        --end-includes "src/VBox/HostDrivers/VBoxNetFlt" "include/VBox/intnet.h"
my_generate_project "VBoxUSB"       "src/VBox/HostDrivers/VBoxUSB"          --begin-incs "include" "src/VBox/HostDrivers/VBoxUSB"           --end-includes "src/VBox/HostDrivers/VBoxUSB" "include/VBox/usblib*.h" "include/VBox/usbfilter.h"
my_generate_project "AdpCtl"        "src/VBox/HostDrivers/adpctl"           --begin-incs "include"                                          --end-includes "src/VBox/HostDrivers/adpctl"

# src/VBox/HostServices
my_generate_project "GuestCntl"     "src/VBox/HostServices/GuestControl"    --begin-incs "include" "src/VBox/HostServices/GuestControl"     --end-includes "src/VBox/HostServices/GuestControl"
my_generate_project "DragAndDrop"   "src/VBox/HostServices/DragAndDrop"     --begin-incs "include" "src/VBox/HostServices/DragAndDrop"      --end-includes "src/VBox/HostServices/DragAndDrop"
my_generate_project "GuestProps"    "src/VBox/HostServices/GuestProperties" --begin-incs "include" "src/VBox/HostServices/GuestProperties"  --end-includes "src/VBox/HostServices/GuestProperties"
my_generate_project "ShClip-HS"     "src/VBox/HostServices/SharedClipboard" --begin-incs "include" "src/VBox/HostServices/SharedClipboard"  --end-includes "src/VBox/HostServices/SharedClipboard"
my_generate_project "SharedFolders" "src/VBox/HostServices/SharedFolders"   --begin-incs "include" "src/VBox/HostServices/SharedFolders"    --end-includes "src/VBox/HostServices/SharedFolders" "include/VBox/shflsvc.h"
my_generate_project "OpenGL-HS"     "src/VBox/HostServices/SharedOpenGL"    --begin-incs "include" "src/VBox/HostServices/SharedOpenGL"     --end-includes "src/VBox/HostServices/SharedOpenGL"

# src/VBox/ImageMounter
my_generate_project "ImageMounter"  "src/VBox/ImageMounter"                 --begin-incs "include" "src/VBox/ImageMounter"                  --end-includes "src/VBox/ImageMounter"

# src/VBox/Installer
my_generate_project "Installers"    "src/VBox/Installer"                    --begin-incs "include"                                          --end-includes "src/VBox/Installer"

# src/VBox/Main
my_generate_project "Main"          "src/VBox/Main"                         --begin-incs "include" "src/VBox/Main/include"                  --end-includes "src/VBox/Main" "include/VBox/com" "include/VBox/settings.h"
## @todo seperate webservices and Main. pick the right headers. added generated headers.

# src/VBox/Network
my_generate_project "Net-DHCP"      "src/VBox/NetworkServices/DHCP"         --begin-incs "include" "src/VBox/NetworkServices/NetLib"        --end-includes "src/VBox/NetworkServices/DHCP"
my_generate_project "Net-NAT"       "src/VBox/NetworkServices/NAT"          --begin-incs "include" "src/VBox/NetworkServices/NAT"           --end-includes "src/VBox/NetworkServices/NAT" "src/VBox/Devices/Network/slirp"
my_generate_project "Net-NetLib"    "src/VBox/NetworkServices/NetLib"       --begin-incs "include" "src/VBox/NetworkServices/NetLib"        --end-includes "src/VBox/NetworkServices/NetLib"

# src/VBox/RDP
my_generate_project "RDP-Client"    "src/VBox/RDP/client-1.8.3"             --begin-incs "include" "src/VBox/RDP/client-1.8.3"              --end-includes "src/VBox/RDP/client-1.8.3"
my_generate_project "RDP-Server"    "src/VBox/RDP/server"                   --begin-incs "include" "src/VBox/RDP/server"                    --end-includes "src/VBox/RDP/server"
my_generate_project "RDP-WebClient" "src/VBox/RDP/webclient"                --begin-incs "include" "src/VBox/RDP/webclient"                 --end-includes "src/VBox/RDP/webclient"
my_generate_project "RDP-Misc"      "src/VBox/RDP"                          --begin-incs "include"                                          --end-includes "src/VBox/RDP/auth" "src/VBox/RDP/tscpasswd" "src/VBox/RDP/x11server"

# src/VBox/Storage
my_generate_project "Storage"       "src/VBox/Storage"                      --begin-incs "include" "src/VBox/Storage"                       --end-includes "src/VBox/Storage"

# src/VBox/ValidationKit
my_generate_project "ValidationKit" "src/VBox/ValidationKit"                --begin-incs "include"                                          --end-includes "src/VBox/ValidationKit"

# src/VBox/ExtPacks
my_generate_project "ExtPacks"      "src/VBox/ExtPacks"                     --begin-incs "include"                                          --end-includes "src/VBox/ExtPacks"

# src/bldprogs
my_generate_project "bldprogs"      "src/bldprogs"                          --begin-incs "include"                                          --end-includes "src/bldprogs"

# A few things from src/lib
my_generate_project "zlib"          "src/libs/zlib-1.2.8"                   --begin-incs "include"                                          --end-includes "src/libs/zlib-1.2.8/*.c" "src/libs/zlib-1.2.8/*.h"
my_generate_project "liblzf"        "src/libs/liblzf-3.4"                   --begin-incs "include"                                          --end-includes "src/libs/liblzf-3.4"
my_generate_project "libpng"        "src/libs/libpng-1.2.54"                --begin-incs "include"                                          --end-includes "src/libs/libpng-1.2.54/*.c" "src/libs/libpng-1.2.54/*.h"
my_generate_project "openssl"       "src/libs/openssl-1.1.0h"               --begin-incs "include" "src/libs/openssl-1.1.0h/crypto"         --end-includes "src/libs/openssl-1.1.0h"
my_generate_project "curl"          "src/libs/curl-7.57.0"                  --begin-incs "include" "src/libs/curl-7.57.0/include"           --end-includes "src/libs/curl-7.57.0"
my_generate_project "kStuff"        "src/libs/kStuff"                       --begin-incs "include" "src/libs/kStuff/kStuff/include"         --end-includes "src/libs/kStuff"


# include/VBox
my_generate_project "VBoxHeaders"   "include"                               --begin-incs "include"                                          --end-includes "include/VBox"

# Misc
my_generate_project "misc"          "."                                     --begin-incs "include"                                          --end-includes \
    "configure" \
    "configure.vbs" \
    "Config.kmk" \
    "Makefile.kmk" \
    "src/Makefile.kmk" \
    "src/VBox/Makefile.kmk"


# out/x.y/z/bin/sdk/bindings/xpcom
XPCOM_INCS="src/libs/xpcom18a4"
for d in \
    "out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/${KBUILD_TYPE}/dist/sdk/bindings/xpcom" \
    "out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/${BUILD_TYPE}/dist/sdk/bindings/xpcom" \
    "out/${KBUILD_TARGET}.${KBUILD_TARGET_ARCH}/${KBUILD_TYPE}/bin/sdk/bindings/xpcom" \
    "out/${BUILD_TARGET}.${BUILD_TARGET_ARCH}/${BUILD_TYPE}/bin/sdk/bindings/xpcom" \
    "out/linux.amd64/debug/bin/sdk/bindings/xpcom" \
    "out/linux.x86/debug/bin/sdk/bindings/xpcom" \
    "out/darwin.amd64/debug/dist/sdk/bindings/xpcom" \
    "out/darwin.x86/debug/bin/dist/bindings/xpcom" \
    "out/haiku.amd64/debug/bin/sdk/bindings/xpcom" \
    "out/haiku.x86/debug/bin/sdk/bindings/xpcom" \
    "out/solaris.amd64/debug/bin/sdk/bindings/xpcom" \
    "out/solaris.x86/debug/bin/sdk/bindings/xpcom";
do
    if test -d "${MY_ROOT_DIR}/${d}"; then
        my_generate_project "SDK-xpcom" "${d}"  --begin-incs "include" "${d}/include" --end-includes "${d}"
        XPCOM_INCS="${d}/include"
        break
    fi
done

# lib/xpcom
my_generate_project "xpcom"         "src/libs/xpcom18a4"                    --begin-incs "include" "${XPCOM_INCS}"                          --end-includes "src/libs/xpcom18a4"

my_generate_workspace


#
# Update the history file if present.
#
MY_FILE="${MY_WS_NAME}histu"
if test -f "${MY_FILE}"; then
    echo "Updating ${MY_FILE}..."
    ${MY_MV} -f "${MY_FILE}" "${MY_FILE}.old"
    ${MY_SED} -n \
        -e '/PROJECT_CACHE/d' \
        -e '/\[TreeExpansion2\]/d' \
        -e '/^\[/p' \
        -e '/: /p' \
        -e '/^CurrentProject/p' \
        "${MY_FILE}.old" > "${MY_FILE}"
fi


#
# Generate and update the usercpp.h/unxcpp.h file.
#
my_generate_usercpp_h


echo "done"
