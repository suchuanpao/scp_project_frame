/*
 *  testsense.cpp  2017-08-12
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
 *      FileName: testsense.cpp
 *
 *        Author: Chuanpao Su
 *       Version: 1.0
 *   Description: ----
 *          Mail: suchuanpao@outlook.com
 *        Create: 2017-08-12 14:12:22
 * Last Modified: 2017-08-15 10:49:59
 *  
 ************************************************************************/


#include<iostream>
#include <stdio.h>
#include <wiringPi.h>
using namespace std;
#define GPIO_TOUCHSENSE0 25

int main(int argc, char *argv[])
{
	wiringPiSetup();
	pinMode(GPIO_TOUCHSENSE0, INPUT);
	int flag = 0;
	int var = 0;
	while (1) {
		var = digitalRead(GPIO_TOUCHSENSE0);
		if (flag != var) {
			flag = var;
			printf("touch \n");
		}
	}
	return 0;
}
