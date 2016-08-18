MY_LOCAL_PATH := $(call my-dir)

# ==========================================================
# vrapi
# ==========================================================
LOCAL_PATH := ../3rdparty/ovr_sdk_mobile_1.0.3.1/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := vrapi
LOCAL_SRC_FILES := \
	libvrapi.so
include $(PREBUILT_SHARED_LIBRARY)

# ==========================================================
# systemutils
# ==========================================================
LOCAL_PATH := ../3rdparty/ovr_sdk_mobile_1.0.3.1/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := systemutils
LOCAL_SRC_FILES := \
	libsystemutils.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# openglloader
# ==========================================================
LOCAL_PATH := ../3rdparty/ovr_sdk_mobile_1.0.3.1/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := openglloader
LOCAL_SRC_FILES := \
	libopenglloader.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# ovrkernel
# ==========================================================
LOCAL_PATH := ../3rdparty/ovr_sdk_mobile_1.0.3.1/armeabi-v7a
include $(CLEAR_VARS)
LOCAL_MODULE := ovrkernel
LOCAL_SRC_FILES := \
	libovrkernel.a
include $(PREBUILT_STATIC_LIBRARY)

# ==========================================================
# OculusMobileSDKHeadTracking
# ==========================================================
LOCAL_PATH := .
include $(CLEAR_VARS)
LOCAL_MODULE := OculusMobileSDKHeadTracking
LOCAL_C_INCLUDES := \
	. \
	../3rdparty/ovr_sdk_mobile_1.0.3.1/include
LOCAL_SRC_FILES := \
	./OculusMobileSDKHeadTracking.cpp 
LOCAL_CFLAGS := -std=c++11 -Werror 
LOCAL_SHARED_LIBRARIES := vrapi
LOCAL_WHOLE_STATIC_LIBRARIES := \
	openglloader \
LOCAL_STATIC_LIBRARIES := \
	systemutils \
	ovrkernel 
LOCAL_LDLIBS := -llog -landroid -lGLESv3 -lEGL
include $(BUILD_SHARED_LIBRARY)
