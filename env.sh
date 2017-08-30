#!/bin/bash

#*  env.sh  2017-08-10
#*  Copyright (C) 2017  Chuanpao Su
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
#*      FileName: env.sh
#*
#*        Author: Chuanpao Su
#*       Version: 1.0
#*   Description: ----
#*          Mail: suchuanpao@outlook.com
#*        Create: 2017-08-10 14:22:50
#* Last Modified: 2017-08-30 10:28:58
#*  
#***********************************************************************
XS_CHIP=arm
export XS_CHIP

XS_CROSS_COMPILE=arm-linux-gnueabihf-
export XS_CROSS_COMPILE

XS_PREFIX=`pwd`
export XS_PREFIX

XS_LIB=$XS_PREFIX/lib
export XS_LIB
mkdir -p $XS_LIB

XS_BIN=$XS_PREFIX/bin
export XS_BIN
mkdir -p $XS_BIN

XS_SRC=$XS_PREFIX/src
export XS_SRC
mkdir -p $XS_SRC

XS_INCLUDE=$XS_PREFIX/include
export XS_INCLUDE
mkdir -p $XS_INCLUDE

XS_SCRIPTS=$XS_PREFIX/scripts
export XS_SCRIPTS
mkdir -p $XS_SCRIPTS

