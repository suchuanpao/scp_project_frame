#!/bin/bash

#*  build.sh  17-08-10
#*  Copyright (C) 17  Chuanpao Su
#*
#*  This program is free software: you can redistribute it and/or modify
#*  it under the terms of the GNU General Public License as published by
#*  the Free Software Foundation, either version 3 of the License, or
#*  (at your option) any later version.
#*
#*  This program is distributed in the hope that it will be useful,
#*  but WITHOUT ANY WARRANTY; without even the implied warranty of
#*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#*  GNU General Public License for more details.
#*
#*  You should have received a copy of the GNU General Public License
#*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#*
#*
#************************************************************************
#*      FileName: build.sh
#*
#*        Author: Chuanpao Su
#*       Version: 1.0
#*   Description: ----
#*          Mail: suchuanpao@outlook.com
#*        Create: 2017-08-10 13:38:12
#* Last Modified: 2017-08-30 08:48:59
#*  
#************************************************************************
source env.sh

HELP()
{
	BUILD_FILE=$0
	echo "##############################################################################"
	echo "#"
	echo "# $BUILD_FILE {proj_type} {opt}"
	echo "#"
	echo "# proj_type: { tests | app | lib | all }"
	echo "#"
	echo "# opt:"
	echo "#     tests : [ module_name] { | clean }"
	echo "#       app : [ app_name ] { | clean }"
	echo "#       lib : [ lib_name ] { | clean }"
	echo "#"
	echo "# example:"
	echo "#     $BUILD_FILE tests touchsense"
	echo "#     $BUILD_FILE tests touchsense clean"
	echo "#     $BUILD_FILE app asr"
	echo "#     $BUILD_FILE app asr clean"
	echo "#     $BUILD_FILE lib"
	echo "#     $BUILD_FILE lib clean"
	echo "#"
	echo "##############################################################################"
}

PROJ_TYPE=$1
case $PROJ_TYPE in
	tests)
		PROJ=$2
		if [[ -d $XS_PREFIX/$PROJ_TYPE/$PROJ && x'' != x$PROJ ]]; then
			cd $XS_PREFIX/$PROJ_TYPE/$PROJ
			MAKEFILE_OPT=$3
			make $MAKEFILE_OPT
		else
			echo "Please choose a project to build"
		fi
		;;
	lib)
		PROJ_TYPE=src
		PROJ=$2
		if [[ -d $XS_PREFIX/$PROJ_TYPE/$PROJ && x'' != x$PROJ ]]; then
			cd $XS_PREFIX/$PROJ_TYPE/$PROJ
			MAKEFILE_OPT=$3
			make $MAKEFILE_OPT
		else
			cd $XS_PREFIX/$PROJ_TYPE
			MAKEFILE_OPT=$2
			make $MAKEFILE_OPT
		fi
		;;
	app)
		PROJ=$2
		if [[ -d $XS_PREFIX/$PROJ_TYPE/$PROJ && x'' != x$PROJ ]]; then
			cd $XS_PREFIX/$PROJ_TYPE/$PROJ
			MAKEFILE_OPT=$3
			make $MAKEFILE_OPT
		else
			cd $XS_PREFIX/$PROJ_TYPE
			MAKEFILE_OPT=$2
			make $MAKEFILE_OPT
		fi
		;;
	*)
		HELP $0
		;;
esac
