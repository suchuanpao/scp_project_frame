/*
 *  wavesense.cpp  2017-08-08
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
 *      FileName: src/wavesense.cpp
 *
 *        Author: Chuanpao Su
 *       Version: 1.0
 *   Description: ----
 *          Mail: suchuanpao@outlook.com
 *        Create: 2017-08-08 08:39:25
 * Last Modified: 2017-08-16 08:33:47
 *  
 ************************************************************************/

#include <sense/wavesense.hpp>
#include <wiringPi.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <iostream>

using namespace std;

typedef int xs_wavesense_handle_t;
#define XS_WAVESENSE_MAX_DEVNUM 2

struct xs_wavesense_timeval {
    struct timeval st, et, dt;
};

struct xs_wavesense_dev {
    unsigned char pin_echo;
    unsigned char pin_trig;
    XS_WAVESENSE_FLAG flag;
    struct xs_wavesense_timeval time;
    unsigned short int result;
};

static inline int timeval2dis(struct timeval *t)
{
    return ((t->tv_usec-700)*170/10000);
}

static struct xs_wavesense_dev devs[] = {
    {
        .pin_echo = 8,
        .pin_trig = 6,
        .flag = XS_WAVESENSE_FLAG_FREE,
        .time = {0},
    },
    {
        .pin_echo = 9,
        .pin_trig = 13,
        .flag = XS_WAVESENSE_FLAG_FREE,
        .time = {0},
    },
};

int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y)
{
    int nsec;

    if ( x->tv_sec>y->tv_sec )
        return -1;

    if ( (x->tv_sec==y->tv_sec) && (x->tv_usec>y->tv_usec) )
        return -1;

    result->tv_sec = ( y->tv_sec-x->tv_sec );
    result->tv_usec = ( y->tv_usec-x->tv_usec );

    if (result->tv_usec<0) {
        result->tv_sec--;
        result->tv_usec+=1000000;
    }
    return 0;
}

static void xs_wavesense_isr(void)
{
    int i;
    struct xs_wavesense_dev *dev = NULL;
    for (i = 0; i < XS_WAVESENSE_MAX_DEVNUM; i++) {
        dev = &devs[i];
        if (dev->flag == XS_WAVESENSE_FLAG_WORKED) {
            gettimeofday(&dev->time.et, NULL);
            timeval_subtract(&dev->time.dt, &dev->time.st, &dev->time.et);
            dev->flag = XS_WAVESENSE_FLAG_IRQ_OCCUR;
        }
    }
}

static int xs_wavesense_request_irq(struct xs_wavesense_dev *dev)
{
    int i = 0;
    // 上锁保护这里，可以做到线程安全
    for (i = 0; i < XS_WAVESENSE_MAX_DEVNUM; i++) {
        if (devs[i].flag == XS_WAVESENSE_FLAG_WORKED || \
            devs[i].flag == XS_WAVESENSE_FLAG_IRQ_OCCUR)
            return -1;
    }
    dev->flag = XS_WAVESENSE_FLAG_WORKED;
    // 解锁
}

static int xs_wavesense_ready(struct xs_wavesense_dev *dev)
{
    memset(&dev->time, 0, sizeof(dev->time));
    if (xs_wavesense_request_irq(dev) < 0)
        return -1;
    digitalWrite(dev->pin_trig, HIGH);
    usleep(20);
    digitalWrite(dev->pin_trig, LOW);
    gettimeofday(&dev->time.st, NULL);
    return 0;
}

static void xs_wavesense_over(struct xs_wavesense_dev *dev)
{
    dev->flag = XS_WAVESENSE_FLAG_FREE;
};

int xs_wavesense_read(xs_wavesense_handle_t handle, int timeout)
{
    int ret;
    struct xs_wavesense_dev *dev = NULL;
    if (timeout > 100 || !handle)
        return XS_WAVESENSE_DIS_ERR_ARGS_INVALID;
    dev = (struct xs_wavesense_dev *)handle;
    if (xs_wavesense_ready(dev) < 0) {
        ret = XS_WAVESENSE_DIS_ERR_IRQ_BUSY;
        goto err;
    }
    if (timeout > 0 && timeout <= 100) {
        if (waitForInterrupt(dev->pin_echo, timeout) < 0) {
            ret = XS_WAVESENSE_DIS_ERR_INTERRUPT_FAILED;
            goto err;
        }
        if (dev->flag != XS_WAVESENSE_FLAG_IRQ_OCCUR) {
            ret = XS_WAVESENSE_DIS_ERR_NOT_SAFE;
            goto err;
        }
        ret = timeval2dis(&dev->time.dt);
    } else if (timeout == 0) {
        if (dev->flag != XS_WAVESENSE_FLAG_IRQ_OCCUR) {
            ret = XS_WAVESENSE_DIS_ERR_NO_INTERRUPT;
            goto err;
        }
        ret = timeval2dis(&dev->time.dt);
    } else if (timeout < 0) {
        while (dev->flag != XS_WAVESENSE_FLAG_IRQ_OCCUR);
        ret = timeval2dis(&dev->time.dt);
    }
err:
    xs_wavesense_over(dev);
    return ret;
}

XS_WAVESENSE_DIS xs_wavesense_read_safe(xs_wavesense_handle_t handle, int timeout)
{
#define XS_WAVESENSE_MAX_SAMPLES 5
    int ret, i = XS_WAVESENSE_MAX_SAMPLES;
    int dis[5] = {0};
    int *p = &dis[1];
    struct xs_wavesense_dev *dev = NULL;
    if (timeout > 100 || !handle)
        return XS_WAVESENSE_DIS_ERR_ARGS_INVALID;
    dev = (struct xs_wavesense_dev *)handle;
    while (i--) {
        ret = xs_wavesense_read(handle, timeout);
        if (ret < 0) {
            p[XS_WAVESENSE_DIS_ERR]++;
        } else {
            ret = ret / 30;
            p[ret>XS_WAVESENSE_DIS_SAFE?XS_WAVESENSE_DIS_SAFE:ret]++;
        }
    }
#define XS_MAX(a,p1,p2) (a[p1])>(a[p2])?(p1):(p2)
    return XS_MAX(p, XS_MAX(p, XS_MAX(p, XS_MAX(p,
            XS_WAVESENSE_DIS_SHORT,
            XS_WAVESENSE_DIS_MEDIUM),
            XS_WAVESENSE_DIS_LARGE),
            XS_WAVESENSE_DIS_SAFE),
            XS_WAVESENSE_DIS_ERR);
}

xs_wavesense_handle_t xs_wavesense_open(int dev_id)
{
    int i;
    struct xs_wavesense_dev *dev = NULL;
    if (dev_id >= XS_WAVESENSE_MAX_DEVNUM || dev_id < 0)
        return -1;
    dev = &devs[dev_id];
    pinMode(dev->pin_trig, OUTPUT);
    digitalWrite(dev->pin_trig, LOW);
    wiringPiISR(dev->pin_echo, INT_EDGE_BOTH, &xs_wavesense_isr);
    wiringPiISR(dev->pin_echo, INT_EDGE_FALLING, &xs_wavesense_isr);
    return (xs_wavesense_handle_t)dev;
}
