LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := ntfs-3g.c ntfs-3g_common.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include/fuse-lite \
                    $(LOCAL_PATH)/../include/ntfs-3g \
                    $(LOCAL_PATH)/..
LOCAL_CFLAGS := -DHAVE_CONFIG_H -Werror -Wall

LOCAL_MODULE := ntfs-3gd

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES += libntfs-3g libfuse-lite

LOCAL_MODULE_PATH := $(TARGET_RECOVERY_ROOT_OUT)/sbin

include $(BUILD_EXECUTABLE)
