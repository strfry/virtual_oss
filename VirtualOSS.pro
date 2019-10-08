# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

#TARGET = VirtualOSS

HEADERS = \
   $$PWD/backend_bt/avdtp_signal.h \
   $$PWD/backend_bt/backend_bt.h \
   $$PWD/backend_bt/sbc_coeffs.h \
   $$PWD/backend_bt/sbc_encode.h \
   $$PWD/virtual_backend.h \
   $$PWD/virtual_int.h \
   $$PWD/virtual_oss.h \
   $$PWD/virtual_utils.h

SOURCES = \
   $$PWD/backend_bt/cosdata-gen/cosdata.c \
   $$PWD/backend_bt/avdtp.c \
   $$PWD/backend_bt/backend_bt.c \
   $$PWD/backend_bt/bt_speaker.c \
   $$PWD/backend_bt/sbc_encode.c \
   $$PWD/backend_null/backend_null.c \
   $$PWD/backend_oss/backend_oss.c \
   $$PWD/equalizer/virtual_equalizer.c \
   $$PWD/virtual_audio_delay.c \
   $$PWD/virtual_ctl.c \
   $$PWD/virtual_eq.c \
   $$PWD/virtual_format.c \
   $$PWD/virtual_main.c \
   $$PWD/virtual_mul.c \
   $$PWD/virtual_oss.c \
   $$PWD/virtual_ring.c

INCLUDEPATH = \
    $$PWD/. \
    $$PWD/backend_bt

#DEFINES = 

