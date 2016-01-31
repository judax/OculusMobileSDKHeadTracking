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
 * Any class can register to listen to head tracking orientation updates.
 * At least for now, the view that instances of this class provide needs to be added to the view hierarchy
 * in order for the head tracking acquisition mechanism to work. This view won't be used to render anything
 * so there are several recommendations on how to add the view inside your apps:
 * 1) Add the view as a 1x1 pixel size view using the corresponding layout params.
 * 2) Use a FrameLayout to add the view and then add your app's views on top of it.  
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
	
	private synchronized OculusMobileSDKHeadTrackingListener[] createOculusMobileSDKHeadTrackingListenersArray()
	{
		OculusMobileSDKHeadTrackingListener[] oculusMobileSDKHeadTrackingListenersArray = new OculusMobileSDKHeadTrackingListener[oculusMobileSDKHeadTrackingListeners.size()];
		return oculusMobileSDKHeadTrackingListeners.toArray(oculusMobileSDKHeadTrackingListenersArray);
	}
	
	/**
	 * This method will be called from the native side providing the orientation.
	 * 
	 * @param x
	 * @param y
	 * @param z
	 */
	private void setTrackingOrientationFromNative(float x, float y, float z, float w)
	{
		OculusMobileSDKHeadTrackingListener[] oculusMobileSDKHeadTrackingListenersArray = createOculusMobileSDKHeadTrackingListenersArray();
		for (OculusMobileSDKHeadTrackingListener listener: oculusMobileSDKHeadTrackingListenersArray)
		{
			listener.headTrackingOrientationUpdated(this, x, y, z, w);
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
