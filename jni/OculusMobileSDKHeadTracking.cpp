#include <stdlib.h> // fot rand
#include <unistd.h> // fot gettid

#include <sched.h> // for sched_yield

#include <pthread.h>

#include "LogUtils.h"
#include <android/native_window_jni.h>	// for native window JNI
#include <android/input.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

#include "VrApi.h"
#include "VrApi_Helpers.h"
#include "SystemActivities.h"

// ================================================================================================
// Some of the Oculus Mobile SDK Math helpers
// ================================================================================================

// Definitions of axes for coordinate and rotation conversions.
enum Axis
{
    Axis_X = 0, Axis_Y = 1, Axis_Z = 2
};

// RotateDirection describes the rotation direction around an axis, interpreted as follows:
//  CW  - Clockwise while looking "down" from positive axis towards the origin.
//  CCW - Counter-clockwise while looking from the positive axis towards the origin,
//        which is in the negative axis direction.
//  CCW is the default for the RHS coordinate system. Oculus standard RHS coordinate
//  system defines Y up, X right, and Z back (pointing out from the screen). In this
//  system Rotate_CCW around Z will specifies counter-clockwise rotation in XY plane.
enum RotateDirection
{
    Rotate_CCW = 1,
    Rotate_CW  = -1
};

// Constants for right handed and left handed coordinate systems
enum HandedSystem
{
    Handed_R = 1, Handed_L = -1
};

//------------------------------------------------------------------------------------//
// ***** Math
//
// Math class contains constants and functions. This class is a template specialized
// per type, with Math<float> and Math<double> being distinct.
template<class Type>
class Math
{
public:
    // By default, support explicit conversion to float. This allows Vector2<int> to
    // compile, for example.
    typedef float OtherFloatType;
};

// Single-precision Math constants class.
template<>
class Math<float>
{
public:
    static const float Pi;
    static const float TwoPi;
    static const float PiOver2;
    static const float PiOver4;
    static const float E;
    
    static const float MaxValue;			// Largest positive float Value
    static const float MinPositiveValue;	// Smallest possible positive value
    
    static const float RadToDegreeFactor;
    static const float DegreeToRadFactor;
    
    static const float Tolerance;			// 0.00001f;
    static const float SingularityRadius;	// 0.0000001f for Gimbal lock numerical problems
    
    static const float SmallestNonDenormal;	// ( numerator <= 4 ) / ( denominator > SmallestNonDenormal ) < infinite
    static const float HugeNumber;			// HugeNumber * HugeNumber < infinite
    
    // Used to support direct conversions in template classes.
    typedef double OtherFloatType;
};

const float Math<float>::Pi = 3.14159265358979323846;
const float Math<float>::PiOver2 = Math<float>::Pi / 2.0;
const float Math<float>::SingularityRadius = 0.0000001f;

typedef Math<float>  Mathf;

template<class T>
class Quat
{
public:
    // w + Xi + Yj + Zk
    T x, y, z, w;
    
    Quat() : x(0), y(0), z(0), w(1) { }
    Quat(T x_, T y_, T z_, T w_) : x(x_), y(y_), z(z_), w(w_) { }
    
    // GetEulerAngles extracts Euler angles from the quaternion, in the specified order of
    // axis rotations and the specified coordinate system. Right-handed coordinate system
    // is the default, with CCW rotations while looking in the negative axis direction.
    // Here a,b,c, are the Yaw/Pitch/Roll angles to be returned.
    // rotation a around axis A1
    // is followed by rotation b around axis A2
    // is followed by rotation c around axis A3
    // rotations are CCW or CW (D) in LH or RH coordinate system (S)
    template <Axis A1, Axis A2, Axis A3, RotateDirection D, HandedSystem S>
    void GetEulerAngles(T *a, T *b, T *c) const
    {
//        OVR_COMPILER_ASSERT((A1 != A2) && (A2 != A3) && (A1 != A3));
        
        T Q[3] = { x, y, z };  //Quaternion components x,y,z
        
        T ww  = w*w;
        T Q11 = Q[A1]*Q[A1];
        T Q22 = Q[A2]*Q[A2];
        T Q33 = Q[A3]*Q[A3];
        
        T psign = T(-1);
        // Determine whether even permutation
        if (((A1 + 1) % 3 == A2) && ((A2 + 1) % 3 == A3))
            psign = T(1);
        
        T s2 = psign * T(2) * (psign*w*Q[A2] + Q[A1]*Q[A3]);
        
        if (s2 < T(-1) + Math<T>::SingularityRadius)
        { // South pole singularity
            *a = T(0);
            *b = -S*D*Math<T>::PiOver2;
            *c = S*D*atan2(T(2)*(psign*Q[A1]*Q[A2] + w*Q[A3]),
                           ww + Q22 - Q11 - Q33 );
        }
        else if (s2 > T(1) - Math<T>::SingularityRadius)
        {  // North pole singularity
            *a = T(0);
            *b = S*D*Math<T>::PiOver2;
            *c = S*D*atan2(T(2)*(psign*Q[A1]*Q[A2] + w*Q[A3]),
                           ww + Q22 - Q11 - Q33);
        }
        else
        {
            *a = -S*D*atan2(T(-2)*(w*Q[A1] - psign*Q[A2]*Q[A3]),
                            ww + Q33 - Q11 - Q22);
            *b = S*D*asin(s2);
            *c = S*D*atan2(T(2)*(w*Q[A3] - psign*Q[A1]*Q[A2]),
                           ww + Q11 - Q22 - Q33);
        }
        return;
    }
    
    template <Axis A1, Axis A2, Axis A3, RotateDirection D>
    void GetEulerAngles(T *a, T *b, T *c) const
    { GetEulerAngles<A1, A2, A3, D, Handed_R>(a, b, c); }
    
    template <Axis A1, Axis A2, Axis A3>
    void GetEulerAngles(T *a, T *b, T *c) const
    { GetEulerAngles<A1, A2, A3, Rotate_CCW, Handed_R>(a, b, c); }
};

// ================================================================================================
// Oculus Mobile SDK ovrMessageQueue library
// ================================================================================================
typedef enum
{
    MQ_WAIT_NONE,		// don't wait
    MQ_WAIT_RECEIVED,	// wait until the consumer thread has received the message
    MQ_WAIT_PROCESSED	// wait until the consumer thread has processed the message
} ovrMQWait;

#define MAX_MESSAGE_PARMS	8
#define MAX_MESSAGES		1024

typedef struct
{
    int			Id;
    ovrMQWait	Wait;
    long long	Parms[MAX_MESSAGE_PARMS];
} ovrMessage;

static void ovrMessage_Init( ovrMessage * message, const int id, const ovrMQWait wait )
{
    message->Id = id;
    message->Wait = wait;
    memset( message->Parms, 0, sizeof( message->Parms ) );
}

static void		ovrMessage_SetPointerParm( ovrMessage * message, int index, void * ptr ) { *(void **)&message->Parms[index] = ptr; }
static void *	ovrMessage_GetPointerParm( ovrMessage * message, int index ) { return *(void **)&message->Parms[index]; }
static void		ovrMessage_SetIntegerParm( ovrMessage * message, int index, int value ) { message->Parms[index] = value; }
static int		ovrMessage_GetIntegerParm( ovrMessage * message, int index ) { return (int)message->Parms[index]; }
static void		ovrMessage_SetFloatParm( ovrMessage * message, int index, float value ) { *(float *)&message->Parms[index] = value; }
static float	ovrMessage_GetFloatParm( ovrMessage * message, int index ) { return *(float *)&message->Parms[index]; }

// Cyclic queue with messages.
typedef struct
{
    ovrMessage	 		Messages[MAX_MESSAGES];
    volatile int		Head;	// dequeue at the head
    volatile int		Tail;	// enqueue at the tail
    ovrMQWait			Wait;
    volatile bool		EnabledFlag;
    volatile bool		PostedFlag;
    volatile bool		ReceivedFlag;
    volatile bool		ProcessedFlag;
    pthread_mutex_t		Mutex;
    pthread_cond_t		PostedCondition;
    pthread_cond_t		ReceivedCondition;
    pthread_cond_t		ProcessedCondition;
} ovrMessageQueue;

static void ovrMessageQueue_Create( ovrMessageQueue * messageQueue )
{
    messageQueue->Head = 0;
    messageQueue->Tail = 0;
    messageQueue->Wait = MQ_WAIT_NONE;
    messageQueue->EnabledFlag = false;
    messageQueue->PostedFlag = false;
    messageQueue->ReceivedFlag = false;
    messageQueue->ProcessedFlag = false;
    
    pthread_mutexattr_t	attr;
    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
    pthread_mutex_init( &messageQueue->Mutex, &attr );
    pthread_mutexattr_destroy( &attr );
    pthread_cond_init( &messageQueue->PostedCondition, NULL );
    pthread_cond_init( &messageQueue->ReceivedCondition, NULL );
    pthread_cond_init( &messageQueue->ProcessedCondition, NULL );
}

static void ovrMessageQueue_Destroy( ovrMessageQueue * messageQueue )
{
    pthread_mutex_destroy( &messageQueue->Mutex );
    pthread_cond_destroy( &messageQueue->PostedCondition );
    pthread_cond_destroy( &messageQueue->ReceivedCondition );
    pthread_cond_destroy( &messageQueue->ProcessedCondition );
}

static void ovrMessageQueue_Enable( ovrMessageQueue * messageQueue, const bool set )
{
    messageQueue->EnabledFlag = set;
}

static void ovrMessageQueue_PostMessage( ovrMessageQueue * messageQueue, const ovrMessage * message )
{
    if ( !messageQueue->EnabledFlag )
    {
        return;
    }
    while ( messageQueue->Tail - messageQueue->Head >= MAX_MESSAGES )
    {
        usleep( 1000 );
    }
    pthread_mutex_lock( &messageQueue->Mutex );
    messageQueue->Messages[messageQueue->Tail & ( MAX_MESSAGES - 1 )] = *message;
    messageQueue->Tail++;
    messageQueue->PostedFlag = true;
    pthread_cond_broadcast( &messageQueue->PostedCondition );
    if ( message->Wait == MQ_WAIT_RECEIVED )
    {
        while ( !messageQueue->ReceivedFlag )
        {
            pthread_cond_wait( &messageQueue->ReceivedCondition, &messageQueue->Mutex );
        }
        messageQueue->ReceivedFlag = false;
    }
    else if ( message->Wait == MQ_WAIT_PROCESSED )
    {
        while ( !messageQueue->ProcessedFlag )
        {
            pthread_cond_wait( &messageQueue->ProcessedCondition, &messageQueue->Mutex );
        }
        messageQueue->ProcessedFlag = false;
    }
    pthread_mutex_unlock( &messageQueue->Mutex );
}

static void ovrMessageQueue_SleepUntilMessage( ovrMessageQueue * messageQueue )
{
    if ( messageQueue->Wait == MQ_WAIT_PROCESSED )
    {
        messageQueue->ProcessedFlag = true;
        pthread_cond_broadcast( &messageQueue->ProcessedCondition );
        messageQueue->Wait = MQ_WAIT_NONE;
    }
    pthread_mutex_lock( &messageQueue->Mutex );
    if ( messageQueue->Tail > messageQueue->Head )
    {
        pthread_mutex_unlock( &messageQueue->Mutex );
        return;
    }
    while ( !messageQueue->PostedFlag )
    {
        pthread_cond_wait( &messageQueue->PostedCondition, &messageQueue->Mutex );
    }
    messageQueue->PostedFlag = false;
    pthread_mutex_unlock( &messageQueue->Mutex );
}

static bool ovrMessageQueue_GetNextMessage( ovrMessageQueue * messageQueue, ovrMessage * message, bool waitForMessages )
{
    if ( messageQueue->Wait == MQ_WAIT_PROCESSED )
    {
        messageQueue->ProcessedFlag = true;
        pthread_cond_broadcast( &messageQueue->ProcessedCondition );
        messageQueue->Wait = MQ_WAIT_NONE;
    }
    if ( waitForMessages )
    {
        ovrMessageQueue_SleepUntilMessage( messageQueue );
    }
    pthread_mutex_lock( &messageQueue->Mutex );
    if ( messageQueue->Tail <= messageQueue->Head )
    {
        pthread_mutex_unlock( &messageQueue->Mutex );
        return false;
    }
    *message = messageQueue->Messages[messageQueue->Head & ( MAX_MESSAGES - 1 )];
    messageQueue->Head++;
    pthread_mutex_unlock( &messageQueue->Mutex );
    if ( message->Wait == MQ_WAIT_RECEIVED )
    {
        messageQueue->ReceivedFlag = true;
        pthread_cond_broadcast( &messageQueue->ReceivedCondition );
    }
    else if ( message->Wait == MQ_WAIT_PROCESSED )
    {
        messageQueue->Wait = MQ_WAIT_PROCESSED;
    }
    return true;
}

// ================================================================================================
// Oculus Mobile SDK EGL library
// ================================================================================================
#if !defined( EGL_OPENGL_ES3_BIT_KHR )
#define EGL_OPENGL_ES3_BIT_KHR		0x0040
#endif

static const char * EglErrorString( const EGLint error )
{
    switch ( error )
    {
        case EGL_SUCCESS:				return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
        case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
        case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
        case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
        case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
        case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
        case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
        case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
        case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
        case EGL_CONTEXT_LOST:			return "EGL_CONTEXT_LOST";
        default:						return "unknown";
    }
}

static const char * GlFrameBufferStatusString( GLenum status )
{
    switch ( status )
    {
        case GL_FRAMEBUFFER_UNDEFINED:						return "GL_FRAMEBUFFER_UNDEFINED";
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
        case GL_FRAMEBUFFER_UNSUPPORTED:					return "GL_FRAMEBUFFER_UNSUPPORTED";
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
        default:											return "unknown";
    }
}

#ifdef CHECK_GL_ERRORS

static const char * GlErrorString( GLenum error )
{
    switch ( error )
    {
        case GL_NO_ERROR:						return "GL_NO_ERROR";
        case GL_INVALID_ENUM:					return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE:					return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION:				return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION:	return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY:					return "GL_OUT_OF_MEMORY";
        default: return "unknown";
    }
}

static void GLCheckErrors()
{
    for ( int i = 0; i < 10; i++ )
    {
        const GLenum error = glGetError();
        if ( error == GL_NO_ERROR )
        {
            break;
        }
        ALOGE( "GL error: %s", GlErrorString( error ) );
    }
}

#define GL( func )		func; GLCheckErrors();

#else // CHECK_GL_ERRORS

#define GL( func )		func;

#endif // CHECK_GL_ERRORS

typedef struct
{
    EGLint		MajorVersion;
    EGLint		MinorVersion;
    EGLDisplay	Display;
    EGLConfig	Config;
    EGLSurface	TinySurface;
    EGLSurface	MainSurface;
    EGLContext	Context;
} ovrEgl;

static void ovrEgl_Clear( ovrEgl * egl )
{
    egl->MajorVersion = 0;
    egl->MinorVersion = 0;
    egl->Display = 0;
    egl->Config = 0;
    egl->TinySurface = EGL_NO_SURFACE;
    egl->MainSurface = EGL_NO_SURFACE;
    egl->Context = EGL_NO_CONTEXT;
}

static void ovrEgl_CreateContext( ovrEgl * egl, const ovrEgl * shareEgl )
{
    if ( egl->Display != 0 )
    {
        return;
    }
    
    egl->Display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    LOG_MESSAGE( "        eglInitialize( Display, &MajorVersion, &MinorVersion )" );
    eglInitialize( egl->Display, &egl->MajorVersion, &egl->MinorVersion );
    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.
    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;
    if ( eglGetConfigs( egl->Display, configs, MAX_CONFIGS, &numConfigs ) == EGL_FALSE )
    {
        LOG_ERROR( "        eglGetConfigs() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    const EGLint configAttribs[] =
    {
        EGL_ALPHA_SIZE, 8, // need alpha for the multi-pass timewarp compositor
        EGL_BLUE_SIZE,  8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE,   8,
        EGL_DEPTH_SIZE, 0,
        EGL_SAMPLES,	0,
        EGL_NONE
    };
    egl->Config = 0;
    for ( int i = 0; i < numConfigs; i++ )
    {
        EGLint value = 0;
        
        eglGetConfigAttrib( egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value );
        if ( ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
        {
            continue;
        }
        
        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib( egl->Display, configs[i], EGL_SURFACE_TYPE, &value );
        if ( ( value & ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) ) != ( EGL_WINDOW_BIT | EGL_PBUFFER_BIT ) )
        {
            continue;
        }
        
        int	j = 0;
        for ( ; configAttribs[j] != EGL_NONE; j += 2 )
        {
            eglGetConfigAttrib( egl->Display, configs[i], configAttribs[j], &value );
            if ( value != configAttribs[j + 1] )
            {
                break;
            }
        }
        if ( configAttribs[j] == EGL_NONE )
        {
            egl->Config = configs[i];
            break;
        }
    }
    if ( egl->Config == 0 )
    {
        LOG_ERROR( "        eglChooseConfig() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    LOG_MESSAGE( "        Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )" );
    egl->Context = eglCreateContext( egl->Display, egl->Config, ( shareEgl != NULL ) ? shareEgl->Context : EGL_NO_CONTEXT, contextAttribs );
    if ( egl->Context == EGL_NO_CONTEXT )
    {
        LOG_ERROR( "        eglCreateContext() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
    const EGLint surfaceAttribs[] =
    {
        EGL_WIDTH, 16,
        EGL_HEIGHT, 16,
        EGL_NONE
    };
    LOG_MESSAGE( "        TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )" );
    egl->TinySurface = eglCreatePbufferSurface( egl->Display, egl->Config, surfaceAttribs );
    if ( egl->TinySurface == EGL_NO_SURFACE )
    {
        LOG_ERROR( "        eglCreatePbufferSurface() failed: %s", EglErrorString( eglGetError() ) );
        eglDestroyContext( egl->Display, egl->Context );
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
    LOG_MESSAGE( "        eglMakeCurrent( Display, TinySurface, TinySurface, Context )" );
    if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
    {
        LOG_ERROR( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        eglDestroySurface( egl->Display, egl->TinySurface );
        eglDestroyContext( egl->Display, egl->Context );
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
}

static void ovrEgl_DestroyContext( ovrEgl * egl )
{
    if ( egl->Display != 0 )
    {
        LOG_ERROR( "        eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )" );
        if ( eglMakeCurrent( egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
        {
            LOG_ERROR( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        }
    }
    if ( egl->Context != EGL_NO_CONTEXT )
    {
        LOG_ERROR( "        eglDestroyContext( Display, Context )" );
        if ( eglDestroyContext( egl->Display, egl->Context ) == EGL_FALSE )
        {
            LOG_ERROR( "        eglDestroyContext() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->Context = EGL_NO_CONTEXT;
    }
    if ( egl->TinySurface != EGL_NO_SURFACE )
    {
        LOG_ERROR( "        eglDestroySurface( Display, TinySurface )" );
        if ( eglDestroySurface( egl->Display, egl->TinySurface ) == EGL_FALSE )
        {
            LOG_ERROR( "        eglDestroySurface() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->TinySurface = EGL_NO_SURFACE;
    }
    if ( egl->Display != 0 )
    {
        LOG_ERROR( "        eglTerminate( Display )" );
        if ( eglTerminate( egl->Display ) == EGL_FALSE )
        {
            LOG_ERROR( "        eglTerminate() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->Display = 0;
    }
}

static void ovrEgl_CreateSurface( ovrEgl * egl, ANativeWindow * nativeWindow )
{
    if ( egl->MainSurface != EGL_NO_SURFACE )
    {
        return;
    }
    LOG_MESSAGE( "        MainSurface = eglCreateWindowSurface( Display, Config, nativeWindow, attribs )" );
    const EGLint surfaceAttribs[] = { EGL_NONE };
    egl->MainSurface = eglCreateWindowSurface( egl->Display, egl->Config, nativeWindow, surfaceAttribs );
    if ( egl->MainSurface == EGL_NO_SURFACE )
    {
        LOG_ERROR( "        eglCreateWindowSurface() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
#if EXPLICIT_GL_OBJECTS == 0
    LOG_MESSAGE( "        eglMakeCurrent( display, MainSurface, MainSurface, Context )" );
    if ( eglMakeCurrent( egl->Display, egl->MainSurface, egl->MainSurface, egl->Context ) == EGL_FALSE )
    {
        LOG_ERROR( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        return;
    }
#endif
}

static void ovrEgl_DestroySurface( ovrEgl * egl )
{
#if EXPLICIT_GL_OBJECTS == 0
    if ( egl->Context != EGL_NO_CONTEXT )
    {
        LOG_MESSAGE( "        eglMakeCurrent( display, TinySurface, TinySurface, Context )" );
        if ( eglMakeCurrent( egl->Display, egl->TinySurface, egl->TinySurface, egl->Context ) == EGL_FALSE )
        {
            LOG_ERROR( "        eglMakeCurrent() failed: %s", EglErrorString( eglGetError() ) );
        }
    }
#endif
    if ( egl->MainSurface != EGL_NO_SURFACE )
    {
        LOG_MESSAGE( "        eglDestroySurface( Display, MainSurface )" );
        if ( eglDestroySurface( egl->Display, egl->MainSurface ) == EGL_FALSE )
        {
            LOG_ERROR( "        eglDestroySurface() failed: %s", EglErrorString( eglGetError() ) );
        }
        egl->MainSurface = EGL_NO_SURFACE;
    }
}


// ================================================================================================
// OculudMobileSDKHeadTracking
// ================================================================================================
class OculusMobileSDKHeadTracking
{
private:
    static const int CPU_LEVEL = 2;
    static const int GPU_LEVEL = 3;
    
    enum MessageTypes
    {
        MESSAGE_START,
        MESSAGE_RESUME,
        MESSAGE_PAUSE,
        MESSAGE_STOP,
        MESSAGE_SURFACE_CREATED,
        MESSAGE_SURFACE_DESTROYED,
        MESSAGE_KEY_EVENT,
        MESSAGE_TOUCH_EVENT
    };
    
    pthread_t thread;
    JavaVM* javaVM;
    jobject activityJObject;
    jobject oculusMobileSDKHeadTrackingJObject;
    jclass oculusMobileSDKHeadTrackingJClass;
    jmethodID setTrackingOrientationMethodID;
    ovrMobile* ovr;
    long long frameIndex;
    ANativeWindow* nativeWindow;
    ovrMessageQueue messageQueue;
    bool destroyed;
    bool resumed;
    ovrEgl egl;
    ovrJava java;
    
    void handleVRModeChanges()
    {
        if (nativeWindow != NULL && egl.MainSurface == EGL_NO_SURFACE )
        {
            ovrEgl_CreateSurface(&egl, nativeWindow);
        }
        
        if (resumed != false && nativeWindow != NULL )
        {
            if ( ovr == NULL )
            {
                ovrModeParms parms = vrapi_DefaultModeParms( &java );
                parms.ResetWindowFullscreen = true;	// Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
                
#if EXPLICIT_GL_OBJECTS == 1
                parms.Display = (size_t)egl.Display;
                parms.WindowSurface = (size_t)egl.MainSurface;
                parms.ShareContext = (size_t)egl.Context;
#else
                LOG_MESSAGE( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
#endif
                
                ovr = vrapi_EnterVrMode( &parms );
                
                LOG_MESSAGE( "        vrapi_EnterVrMode()" );
                
#if EXPLICIT_GL_OBJECTS == 0
                LOG_MESSAGE( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
#endif
            }
        }
        else
        {
            if ( ovr != NULL )
            {
                LOG_MESSAGE( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
                
                vrapi_LeaveVrMode( ovr );
                ovr = NULL;
                
                LOG_MESSAGE( "        vrapi_LeaveVrMode()" );
                LOG_MESSAGE( "        eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
            }
        }
        
        if ( nativeWindow == NULL && egl.MainSurface != EGL_NO_SURFACE )
        {
            ovrEgl_DestroySurface( &egl );
        }
    }
    
    void threadFunction()
    {
        java.Vm = javaVM;
        java.Vm->AttachCurrentThread(&java.Env, NULL);
        java.ActivityObject = activityJObject;
        
        SystemActivities_Init( &java );
        
        const ovrInitParms initParms = vrapi_DefaultInitParms(&java);
        int32_t initResult = vrapi_Initialize(&initParms);
        if (initResult != VRAPI_INITIALIZE_SUCCESS)
        {
            char const * msg = initResult == VRAPI_INITIALIZE_PERMISSIONS_ERROR ?
            "Thread priority security exception. Make sure the APK is signed." :
            "VrApi initialization error.";
            SystemActivities_DisplayError(&java, SYSTEM_ACTIVITIES_FATAL_ERROR_OSIG, __FILE__, msg);
        }
        
        ovrEgl_CreateContext( &egl, NULL );
        
        ovrPerformanceParms perfParms = vrapi_DefaultPerformanceParms();
        perfParms.CpuLevel = CPU_LEVEL;
        perfParms.GpuLevel = GPU_LEVEL;
        perfParms.MainThreadTid = gettid();
        
        LOG_MESSAGE("OculusMobileSDKHeadTracking thread running...");
        
        frameIndex = 0;
        
        for (destroyed = false; !destroyed ;)
        {
            for ( ; ; )
            {
                ovrMessage message;
                const bool waitForMessages = (ovr == NULL && destroyed == false);
                if (!ovrMessageQueue_GetNextMessage(&messageQueue, &message, waitForMessages))
                {
                    break;
                }
                
                LOG_MESSAGE("message.Id = %d", message.Id);
                
                switch (message.Id)
                {
                    case MESSAGE_START:
                        break;
                    case MESSAGE_RESUME:
                        resumed = true;
                        break;
                    case MESSAGE_PAUSE:
                        resumed = false;
                        break;
                    case MESSAGE_STOP:
                        destroyed = true;
                        break;
                    case MESSAGE_SURFACE_CREATED:
                        break;
                    case MESSAGE_SURFACE_DESTROYED:
                        break;
//                    case MESSAGE_KEY_EVENT:
//                        ovrApp_HandleKeyEvent( &appState, ovrMessage_GetIntegerParm( &message, 0 ), ovrMessage_GetIntegerParm( &message, 1 ) );
//                        break;
//                    case MESSAGE_TOUCH_EVENT:
//                        ovrApp_HandleTouchEvent( &appState, ovrMessage_GetIntegerParm( &message, 0 ), ovrMessage_GetFloatParm( &message, 1 ), ovrMessage_GetFloatParm( &message, 2 ) );
//                        break;
                }
                
                handleVRModeChanges();
            }
            
//            LOG_MESSAGE("OculusMobileSDKHeadTracking thread still running...");
            
            frameIndex++;

            const double predictedDisplayTime = vrapi_GetPredictedDisplayTime(ovr, frameIndex);
            const ovrTracking baseTracking = vrapi_GetPredictedTracking(ovr, predictedDisplayTime);
            
            const ovrHeadModelParms headModelParms = vrapi_DefaultHeadModelParms();
            const ovrTracking tracking = vrapi_ApplyHeadModel(&headModelParms, &baseTracking);
            
//            // Position and orientation together.
//            typedef struct ovrPosef_
//            {
//                ovrQuatf	Orientation;
//                ovrVector3f	Position;
//            } ovrPosef;
//            typedef struct ovrRigidBodyPosef_
//            {
//                ovrPosef	Pose;
//                ovrVector3f	AngularVelocity;
//                ovrVector3f	LinearVelocity;
//                ovrVector3f	AngularAcceleration;
//                ovrVector3f	LinearAcceleration;
//                double		TimeInSeconds;			// Absolute time of this pose.
//                double		PredictionInSeconds;	// Seconds this pose was predicted ahead.
//            } ovrRigidBodyPosef;
//            
//            // Bit flags describing the current status of sensor tracking.
//            typedef enum
//            {
//                VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED	= 0x0001,	// Orientation is currently tracked.
//                VRAPI_TRACKING_STATUS_POSITION_TRACKED		= 0x0002,	// Position is currently tracked.
//                VRAPI_TRACKING_STATUS_HMD_CONNECTED			= 0x0080	// HMD is available & connected.
//            } ovrTrackingStatus;
//            
//            // Tracking state at a given absolute time.
//            typedef struct ovrTracking_
//            {
//                // Sensor status described by ovrTrackingStatus flags.
//                unsigned int		Status;
//                // Predicted head configuration at the requested absolute time.
//                // The pose describes the head orientation and center eye position.
//                ovrRigidBodyPosef	HeadPose;
//            } ovrTracking;
            
            // My attempt to try to convert from the head orientation quaternion to deviceorientation euler angles.
//            Quat<float> quat(tracking.HeadPose.Pose.Orientation.x, tracking.HeadPose.Pose.Orientation.y, tracking.HeadPose.Pose.Orientation.z, tracking.HeadPose.Pose.Orientation.w);
//            float yaw, eyePitch, eyeRoll;
//            quat.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &eyePitch, &eyeRoll);
            
            java.Env->CallVoidMethod(oculusMobileSDKHeadTrackingJObject, setTrackingOrientationMethodID, tracking.HeadPose.Pose.Orientation.x, tracking.HeadPose.Pose.Orientation.y, tracking.HeadPose.Pose.Orientation.z, tracking.HeadPose.Pose.Orientation.w);
            
            // Yield or wait
//            sched_yield(); // Does not have to really "free" the CPU from this thread
            usleep(16000); // 60 FPS :(
//            usleep(5000000);
            
        }
        
        if (ovr != NULL)
        {
            vrapi_LeaveVrMode(ovr);
            ovr = NULL;
        }
    
        ovrEgl_DestroyContext( &egl );
        
        vrapi_Shutdown();
        
        SystemActivities_Shutdown( &java );
        
        java.Vm->DetachCurrentThread();
        
        LOG_MESSAGE("OculusMobileSDKHeadTracking thread stopped!");
    }
    
    static void* threadFunctionStatic(void* data)
    {
        OculusMobileSDKHeadTracking* _this = (OculusMobileSDKHeadTracking*)data;
        _this->threadFunction();
        return NULL;
    }
    
public:
    OculusMobileSDKHeadTracking(): javaVM(NULL), resumed(false), destroyed(false), ovr(NULL), frameIndex(0), nativeWindow(NULL)
    {
        ovrEgl_Clear(&egl);
    }

    void init(JNIEnv* jniEnv, jobject activityJObject, jobject oculusMobileSDKHeadTrackingJObject)
    {
        jniEnv->GetJavaVM(&javaVM);
        this->activityJObject = jniEnv->NewGlobalRef(activityJObject);
        this->oculusMobileSDKHeadTrackingJObject = jniEnv->NewGlobalRef(oculusMobileSDKHeadTrackingJObject);
        // Cache the jni method that will be continously called
        oculusMobileSDKHeadTrackingJClass = jniEnv->GetObjectClass(oculusMobileSDKHeadTrackingJObject);
        setTrackingOrientationMethodID = jniEnv->GetMethodID(oculusMobileSDKHeadTrackingJClass, "setTrackingOrientationFromNative", "(FFFF)V");
        
        ovrMessageQueue_Create(&messageQueue);
        
        const int createErr = pthread_create( &thread, NULL, threadFunctionStatic, this);
        if ( createErr != 0 )
        {
            LOG_ERROR("pthread_create returned %i", createErr);
        }
        
        // Post MESSAGE_ON_CREATE
        ovrMessageQueue_Enable(&messageQueue, true);
        ovrMessage message;
        ovrMessage_Init(&message, MESSAGE_START, MQ_WAIT_PROCESSED);
        ovrMessageQueue_PostMessage(&messageQueue, &message);
    }
    
    void resume()
    {
        // Post MESSAGE_ON_RESUME
        ovrMessage message;
        ovrMessage_Init(&message, MESSAGE_RESUME, MQ_WAIT_PROCESSED);
        ovrMessageQueue_PostMessage(&messageQueue, &message);
    }
    
    void pause()
    {
        // Post MESSAGE_ON_PAUSE
        ovrMessage message;
        ovrMessage_Init( &message, MESSAGE_PAUSE, MQ_WAIT_PROCESSED );
        ovrMessageQueue_PostMessage( &messageQueue, &message );
    }
    
    void end(JNIEnv* jniEnv)
    {
        // Post MESSAGE_ON_DESTROY
        ovrMessage message;
        ovrMessage_Init(&message, MESSAGE_STOP, MQ_WAIT_PROCESSED);
        ovrMessageQueue_PostMessage(&messageQueue, &message);
        ovrMessageQueue_Enable(&messageQueue, false);
        
        // Wait for the thread and free resources
        pthread_join(thread, NULL);
        jniEnv->DeleteGlobalRef(activityJObject);
        jniEnv->DeleteGlobalRef(oculusMobileSDKHeadTrackingJObject);
        ovrMessageQueue_Destroy(&messageQueue);
    }
    
    inline void setNativeWindow(ANativeWindow* nativeWindow)
    {
        // Is the new nativeWindow is different from the current one?
        if ( this->nativeWindow != nativeWindow )
        {
            if (this->nativeWindow != NULL)
            {
                // There is a current native window so post MESSAGE_ON_SURFACE_DESTROYED
                ovrMessage message;
                ovrMessage_Init( &message, MESSAGE_SURFACE_DESTROYED, MQ_WAIT_PROCESSED );
                ovrMessageQueue_PostMessage(&messageQueue, &message);
                ANativeWindow_release(this->nativeWindow);
                this->nativeWindow = NULL;
            }
            if (nativeWindow != NULL)
            {
                // A new native window has been provided so post MESSAGE_ON_SURFACE_CREATED
                this->nativeWindow = nativeWindow;
                ovrMessage message;
                ovrMessage_Init(&message, MESSAGE_SURFACE_CREATED, MQ_WAIT_PROCESSED);
                ovrMessageQueue_PostMessage(&messageQueue, &message);
            }
        }
        else if ( nativeWindow != NULL )
        {
            // Both the curent native window and the new one are the same (and not NULL). Release the new one (acquired outside of this call)
            ANativeWindow_release(nativeWindow);
        }
    }
};

extern "C"
{
    // Activity life cycle
    JNIEXPORT jlong JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativeStart(JNIEnv* jniEnv, jobject obj, jobject activityJObject, jobject oculusMobileSDKHeadTrackingJObject)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = new OculusMobileSDKHeadTracking();
        oculusMobileSDKHeadTracking->init(jniEnv, activityJObject, oculusMobileSDKHeadTrackingJObject);
        return (jlong)((size_t)oculusMobileSDKHeadTracking);
        
        return 0;
    }

    JNIEXPORT void JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativeResume(JNIEnv* jniEnv, jobject obj, jlong objectPtr)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = (OculusMobileSDKHeadTracking*)((size_t)objectPtr);
        
        oculusMobileSDKHeadTracking->resume();
    }
    
    JNIEXPORT void JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativePause(JNIEnv* jniEnv, jobject obj, jlong objectPtr)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = (OculusMobileSDKHeadTracking*)((size_t)objectPtr);
        
        oculusMobileSDKHeadTracking->pause();
    }
    
    JNIEXPORT void JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativeStop(JNIEnv* jniEnv, jobject obj, jlong objectPtr)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = (OculusMobileSDKHeadTracking*)((size_t)objectPtr);
        oculusMobileSDKHeadTracking->end(jniEnv);
        delete oculusMobileSDKHeadTracking;
        oculusMobileSDKHeadTracking = 0;
    }
    
    // Surface life cycle
    JNIEXPORT void JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativeSurfaceCreated(JNIEnv* jniEnv, jobject obj, jlong objectPtr, jobject surfaceJObject)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = (OculusMobileSDKHeadTracking*)((size_t)objectPtr);
        
        ANativeWindow * newNativeWindow = ANativeWindow_fromSurface(jniEnv, surfaceJObject);
        if (ANativeWindow_getWidth(newNativeWindow) < ANativeWindow_getHeight(newNativeWindow))
        {
            // An app that is relaunched after pressing the home button gets an initial surface with
            // the wrong orientation even though android:screenOrientation="landscape" is set in the
            // manifest. The choreographer callback will also never be called for this surface because
            // the surface is immediately replaced with a new surface with the correct orientation.
            LOG_ERROR("Surface not in landscape mode!");
        }
        
        oculusMobileSDKHeadTracking->setNativeWindow(newNativeWindow);
    }
    
    JNIEXPORT void JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativeSurfaceChanged(JNIEnv* jniEnv, jobject obj, jlong objectPtr, jobject surfaceJObject)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = (OculusMobileSDKHeadTracking*)((size_t)objectPtr);

        ANativeWindow* newNativeWindow = ANativeWindow_fromSurface(jniEnv, surfaceJObject);
        if (ANativeWindow_getWidth(newNativeWindow) < ANativeWindow_getHeight(newNativeWindow))
        {
            // An app that is relaunched after pressing the home button gets an initial surface with
            // the wrong orientation even though android:screenOrientation="landscape" is set in the
            // manifest. The choreographer callback will also never be called for this surface because
            // the surface is immediately replaced with a new surface with the correct orientation.
            LOG_ERROR("Surface not in landscape mode!");
        }
        
        oculusMobileSDKHeadTracking->setNativeWindow(newNativeWindow);
    }
    
    JNIEXPORT void JNICALL Java_com_judax_oculusmobilesdkheadtracking_OculusMobileSDKHeadTracking_nativeSurfaceDestroyed(JNIEnv* jniEnv, jobject obj, jlong objectPtr)
    {
        OculusMobileSDKHeadTracking* oculusMobileSDKHeadTracking = (OculusMobileSDKHeadTracking*)((size_t)objectPtr);
        
        oculusMobileSDKHeadTracking->setNativeWindow(NULL);
    }
    
}
