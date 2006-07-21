#! /bin/sh

if test $# -lt 1 ; then
    echo "Usage: ./distribute.sh [OPTIONS] DIRECTORY PACKAGE"
    echo " "
    echo "OPTIONS:"
    echo "-uHOSTNAME   choose upload hostname (def:ftp-upload.gnu.org)"
    echo "-hHOSTNAME   choose the destination hostname [ftp|alpha] (def:ftp)"
    echo "-cCOMMENT    add a comment to the upload directive"
    echo " "
    echo "DIRECTORY:   the destination directory on the remote host"
    echo "PACKAGE:     the tarball file to distribute"
    echo " "
    exit 1
fi

UPLOAD_HOST=ftp-upload.gnu.org
DESTINATION_HOST=ftp
COMMENT=
DIRECTORY=
PACKAGE=

processing_options=1

for ARG in "$@" ; do
    if test $processing_options = 1 ; then
	case $ARG in
	    -u*)
		UPLOAD_HOST="${ARG#-u}"
		echo UPLOAD_HOST=${UPLOAD_HOST}
		continue
		;;
	    -h*)
		DESTINATION_HOST="${ARG#-h}"
		echo DESTINATION_HOST=${DESTINATION_HOST}
		continue
		;;
	    -c*)
		COMMENT="${ARG#-c}"
		echo COMMENT=${COMMENT}
		continue
		;;
	    -*)
		echo "unknown option: $ARG" >&2
		exit 1
	esac
    fi
    
    processing_options=0

    if test "x$DIRECTORY" = "x" ; then
	DIRECTORY="${ARG}"
	echo DIRECTORY=${DIRECTORY}
	continue;
    fi
	
    if test "x$PACKAGE" = "x" ; then
	PACKAGE="${ARG}"
	echo PACKAGE=${PACKAGE}
	continue;
    fi

    echo "too many arguments: $ARG" >&2
    exit 1
done

if test "x$DIRECTORY" = "x" ; then
    echo "missing required DIRECTORY argument" >&2
    exit 1
fi

if test "x$PACKAGE" = "x" ; then
    echo "missing required PACKAGE argument" >&2
    exit 1
fi

if test -f "$PACKAGE" ; then
    echo "${PACKAGE} is a regular file."
else
    echo "${PACKAGE} is not an existing regular file" >&2
    exit 1
fi

gpg -b --yes ${PACKAGE}

echo "version: 1.1" > ${PACKAGE}.directive
echo "directory: ${DIRECTORY}" >> ${PACKAGE}.directive
echo "filename: ${PACKAGE}" >> ${PACKAGE}.directive

if test "x$COMMENT" != "x" ; then
    echo "comment: ${COMMENT}" >> ${PACKAGE}.directive
fi

gpg --clearsign --yes ${PACKAGE}.directive

#upload results to ftp.
cmdftp ${UPLOAD_HOST}<<EOF
cd /incoming/${DESTINATION_HOST}
u ${PACKAGE} .
u ${PACKAGE}.sig .
u ${PACKAGE}.directive.asc .
exit
EOF
