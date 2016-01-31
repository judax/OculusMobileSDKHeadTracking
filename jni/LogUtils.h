//
//  LogUtils.h
//  OculusMobileSDKTest
//
//  Created by Iker Jamardo Zugaza on 24/01/16.
//  Copyright © 2016 Iker Jamardo Zugaza (aka JudaX). All rights reserved.
//

#ifndef LogUtils_h
#define LogUtils_h

#include <android/log.h>

#define LOG_TAG "OculusMobileSDKTest"
#define LOG_ERROR(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )
#define LOG_MESSAGE(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )

#endif /* LogUtils_h */
