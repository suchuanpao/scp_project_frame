#ifndef __WAVESENSE_HPP__
#define __WAVESENSE_HPP__

typedef enum {
    XS_WAVESENSE_FLAG_FREE = 0x0,
    XS_WAVESENSE_FLAG_WORKED = 0x1,
    XS_WAVESENSE_FLAG_IRQ_OCCUR = 0x2,
} XS_WAVESENSE_FLAG;


#define XS_WAVESENSE_MAX_DEVNUM 2
typedef int xs_wavesense_handle_t;

typedef enum {
    XS_WAVESENSE_DIS_SHORT = 0,
    XS_WAVESENSE_DIS_MEDIUM = 1,
    XS_WAVESENSE_DIS_LARGE = 2,
    XS_WAVESENSE_DIS_SAFE = 3,
    XS_WAVESENSE_DIS_ERR = -1,
    XS_WAVESENSE_DIS_ERR_INTERRUPT_FAILED = -2,
    XS_WAVESENSE_DIS_ERR_NO_INTERRUPT = -3,
    XS_WAVESENSE_DIS_ERR_NOT_SAFE = -4,
    XS_WAVESENSE_DIS_ERR_TIME_OUT = -5,
    XS_WAVESENSE_DIS_ERR_ARGS_INVALID = -6,
    XS_WAVESENSE_DIS_ERR_IRQ_BUSY = -7,
} XS_WAVESENSE_DIS;

xs_wavesense_handle_t xs_wavesense_open(int dev_id);
XS_WAVESENSE_DIS xs_wavesense_read_safe(xs_wavesense_handle_t handle, int timeout);
int xs_wavesense_read(xs_wavesense_handle_t handle, int timeout);

#endif
