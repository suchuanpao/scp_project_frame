/*
 *  app/main.c  2017-08-11
 *  Copyright (C) 2017  Chuanpao Su
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 ************************************************************************
 *      FileName: app/main.c
 *
 *        Author: Chuanpao Su
 *       Version: 1.0
 *   Description: ----
 *          Mail: suchuanpao@outlook.com
 *        Create: 2017-08-11 10:08:08
 * Last Modified: 2017-08-11 10:43:02
 *  
 ************************************************************************/


#include<stdio.h>
#include <wiringPi.h>
int main(int argc, char *argv[])
{
	wiringPiSetup();
    printf("hello world!\n");
}
