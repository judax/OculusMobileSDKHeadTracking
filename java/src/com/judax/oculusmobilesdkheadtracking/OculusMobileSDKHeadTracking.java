package com.judax.oculusmobilesdkheadtracking;

import java.util.ArrayList;

import android.app.Activity;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

/**
 * This class allows to acquire just the head tracking information (orientation) from the Oculus mobile SDK.
 * An activity life cycle events are passed along to instances of this class.
 * Any class can register to listen to head tracking events. Instances of this class also internally store the
 * state of the head tracking values. These values can be retrieved any time calling the corresponding get methods.
 * At least for now, the view that instances of this class provide needs to be added to the view hierarchy
 * in order for the head tracking acquisition mechanism to work. This view won't be used to render anything
 * so there are several recommendations on how to add the view inside your applications:
 * 1) Use a FrameLayout to add the view and then add your app's views on top of it. Recommended. FrameLayout-s stack views
 * one on top of the other. The view required for the head tracking to work can be occluded by the view of your app.  
 * 2) Add the view as a 1x1 pixel size view using the corresponding layout parameters. Not recommended as at least one
 * pixel of you application will be used by this view. Not a perfect solution but works
 * 
 * @author JudaX
 *
 */
public class OculusMobileSDKHeadTracking
{
	private SurfaceView surfaceView;
	private SurfaceHolder surfaceHolder;
	private long nativeObjectPtr;
	private SurfaceHolderCallback surfaceHolderCallback = new SurfaceHolderCallback();
	private ArrayList<OculusMobileSDKHeadTrackingListener> oculusMobileSDKHeadTrackingListeners = new ArrayList<OculusMobileSDKHeadTrackingListener>();

	private boolean started = false;
	private String errorMessage = "";
	private OculusMobileSDKHeadTrackingData data = new OculusMobileSDKHeadTrackingData();
	
	/**
	 * Call this method to initialize the Oculus mobile head tracking.
	 * @throws IllegalStateException if the Oculus mobile SDK could not be initialized.
	 * @param activity The activity where the head tracking will be executed on.
	 */
	public void start(Activity activity)
	{
		System.loadLibrary("OculusMobileSDKHeadTracking");

		// Create the surface and listen to it's holder's states
		surfaceView = new SurfaceView(activity);
		surfaceView.getHolder().addCallback(surfaceHolderCallback);

		// Call the native side so it is initialized and so it returns the pointer to the main C++ object
		nativeObjectPtr = nativeStart(activity, this);
		if (nativeObjectPtr == 0)
		{
			throw new IllegalStateException("The native corresponding object could not be instantiated to handle the Oculus Mobile SDK Head Tracking.");
		}
	}
	
	/**
	 * Returns a view that needs to be added at some point to the application view hierarchy in order to make the
	 * head tracking acquisition to work. 
	 * @return the view to be added to the application view hierarchy in order to fully initialize the head tracking. 
	 */
	public View getView()
	{
		return surfaceView;
	}
	
	/**
	 * Call this method on the Activity onResume life cycle event. It can also be called at any time to resume
	 * a head tracking that is paused.  
	 */
	public void resume()
	{
		nativeResume(nativeObjectPtr);
	}

	/**
	 * Call this method on the Activity onPause life cycle event. It can also be called at any time to
	 * pause the head tracking.
	 */
	public void pause()
	{
		nativePause(nativeObjectPtr);
	}

	/**
	 * Call this method on the Activity onDestroy life cycle event. It fully finalizes the 
	 * head tracking mechanism.
	 */
	public void stop()
	{
		if (surfaceHolder != null)
		{
			nativeSurfaceDestroyed(nativeObjectPtr);
		}
		nativeStop(nativeObjectPtr);
		nativeObjectPtr = 0;
	}
	
	/**
	 * @return if the head tracking has started (true) or not (false)
	 */
	public boolean hasStarted()
	{
		return started;
	}
		
	/**
	 * @return the errorMessage
	 */
	public String getErrorMessage()
	{
		return errorMessage;
	}
	
	/**
	 * @return the current head tracking data.
	 */
	public OculusMobileSDKHeadTrackingData getData()
	{
		return data;
	}

	private class SurfaceHolderCallback implements SurfaceHolder.Callback
	{
		@Override
		public void surfaceCreated(SurfaceHolder holder)
		{
			if (nativeObjectPtr != 0)
			{
				nativeSurfaceCreated(nativeObjectPtr, holder.getSurface());
				surfaceHolder = holder;
			}
			else
			{
				// TODO throw an exception? The surface has been created and we are not able to notify the native side. This should never happen.
			}
		}
	
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width,
				int height)
		{
			if (nativeObjectPtr != 0)
			{
				nativeSurfaceChanged(nativeObjectPtr, holder.getSurface());
				surfaceHolder = holder;
			}
			else
			{
				// TODO throw an exception? The surface has changed and we are not able to notify the native side. This should never happen.
			}
		}
	
		@Override
		public void surfaceDestroyed(SurfaceHolder holder)
		{
			if (nativeObjectPtr != 0)
			{
				nativeSurfaceDestroyed(nativeObjectPtr);
				surfaceHolder = null;
			}
			else
			{
				// TODO throw an exception? The surface has been destroyed and we are not able to notify the native side. This should never happen
			}		
		}
	}

	/**
	 * Register a listener that will be notified when head tracking changes occur.
	 * @param listener The instance to be registered to listen to head tracking changes.
	 */
	public synchronized void addOculusMobileSDKHeadTrackingListener(OculusMobileSDKHeadTrackingListener listener)
	{
		if (listener == null) throw new NullPointerException("The given OculusMobileSDKHeadTrackingListener cannot be null.");
		if (!oculusMobileSDKHeadTrackingListeners.contains(listener))
		{
			oculusMobileSDKHeadTrackingListeners.add(listener);
		}
	}
	
	/**
	 * Unregister a listener of head tracking changes. 
	 * @param listener The listener to be unregistered from listening to head tracking updates.
	 */
	public synchronized void removeOculusMobileSDKHeadTrackingListener(OculusMobileSDKHeadTrackingListener listener)
	{
		if (listener == null) throw new NullPointerException("The given OculusMobileSDKHeadTrackingListener cannot be null.");
		oculusMobileSDKHeadTrackingListeners.remove(listener);
	}
	
	/**
	 * Remove all the registered listeners for head tracking event changes.
	 */
	public synchronized void removeAllOculusMobileSDKHeadTrackingListeners()
	{
		oculusMobileSDKHeadTrackingListeners.clear();
	}
	
	/**
	 * Copy all the listeners to an array in a synchronized method. Calling an unknown listener may block 
	 * the execution indefinitely so it is better to make a copy synchronously and make the calls without blocking the 
	 * add/remove/removeAll methods to add other listeners from other threads. 
	 * @return A copy of all the listeners to be used in callback calls.
	 */
	private synchronized OculusMobileSDKHeadTrackingListener[] createOculusMobileSDKHeadTrackingListenersArray()
	{
		OculusMobileSDKHeadTrackingListener[] oculusMobileSDKHeadTrackingListenersArray = new OculusMobileSDKHeadTrackingListener[oculusMobileSDKHeadTrackingListeners.size()];
		return oculusMobileSDKHeadTrackingListeners.toArray(oculusMobileSDKHeadTrackingListenersArray);
	}
	
	/**
	 * This method will be called from the native side when the native head tracking system has really started.
	 */
	private void headTrackingStartedFromNative(float xFOV, float yFOV, float interpupillaryDistance)
	{
		data.xFOV = xFOV;
		data.yFOV = yFOV;
		data.interpupillaryDistance = interpupillaryDistance;
		started = true;
		// Notify all the registered listeners
		OculusMobileSDKHeadTrackingListener[] oculusMobileSDKHeadTrackingListenersArray = createOculusMobileSDKHeadTrackingListenersArray();
		for (OculusMobileSDKHeadTrackingListener listener: oculusMobileSDKHeadTrackingListenersArray)
		{
			listener.headTrackingStarted(this, data);
		}
	}
	
	/**
	 * This method will be called from the native side every time there is an error in the head tracking native side.
	 */
	private void headTrackingErrorFromNative(String errorMessage)
	{
		synchronized(this)
		{
			this.errorMessage = errorMessage;
		}
		// Notify all the registered listeners
		OculusMobileSDKHeadTrackingListener[] oculusMobileSDKHeadTrackingListenersArray = createOculusMobileSDKHeadTrackingListenersArray();
		for (OculusMobileSDKHeadTrackingListener listener: oculusMobileSDKHeadTrackingListenersArray)
		{
			listener.headTrackingError(this, errorMessage);
		}
	}
	
	/**
	 * This method will be called from the native side every time there is a head tracking information update.
	 */
	private void headTrackingUpdatedFromNative(
					double timeStamp,
					float orientationX, float orientationY, float orientationZ, float orientationW,
					float linearVelocityX, float linearVelocityY, float linearVelocityZ,
					float angularVelocityX, float angularVelocityY, float angularVelocityZ,
					float linearAccelerationX, float linearAccelerationY, float linearAccelerationZ,
					float angularAccelerationX, float angularAccelerationY, float angularAccelerationZ)
	{
		// Store the information
		synchronized(this)
		{
			data.timeStamp = timeStamp;
			data.orientationX = orientationX;
			data.orientationY = orientationY;
			data.orientationZ = orientationZ;
			data.orientationW = orientationW;
			data.angularVelocityX = angularVelocityX;
			data.angularVelocityY = angularVelocityY;
			data.angularVelocityZ = angularVelocityZ;
			data.linearVelocityX = linearVelocityX;
			data.linearVelocityY = linearVelocityY;
			data.linearVelocityZ = linearVelocityZ;
			data.linearAccelerationX = linearAccelerationX;
			data.linearAccelerationY = linearAccelerationY;
			data.linearAccelerationZ = linearAccelerationZ;
			data.angularAccelerationX = angularAccelerationX;
			data.angularAccelerationY = angularAccelerationY;
			data.angularAccelerationZ = angularAccelerationZ;
		}
		// Notify all the registered listeners
		OculusMobileSDKHeadTrackingListener[] oculusMobileSDKHeadTrackingListenersArray = createOculusMobileSDKHeadTrackingListenersArray();
		for (OculusMobileSDKHeadTrackingListener listener: oculusMobileSDKHeadTrackingListenersArray)
		{
			listener.headTrackingUpdated(this, data);
		}
	}
	
	private native long nativeStart(Activity activity, OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking);
	private native long nativeResume(long nativeObjectPtr);
	private native long nativePause(long nativeObjectPtr);
	private native void nativeStop(long nativeObjectPtr);
	private native void nativeSurfaceCreated(long nativeObjectPtr, Surface surface);
	private native void nativeSurfaceChanged(long nativeObjectPtr, Surface surface);
	private native void nativeSurfaceDestroyed(long nativeObjectPtr);
}
