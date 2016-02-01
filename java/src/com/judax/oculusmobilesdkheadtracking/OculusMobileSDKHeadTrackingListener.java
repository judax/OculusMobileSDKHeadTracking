package com.judax.oculusmobilesdkheadtracking;

/**
 * The interface to be implemented by any class that would like to be notified of
 * the Oculus Mobile SDK Head Tracking events.
 * @see OculusMobileSDKHeadTracking
 * @see OculusMobileSDKHeadTracking#addOculusMobileSDKHeadTrackingListener(OculusMobileSDKHeadTrackingListener)
 * @see OculusMobileSDKHeadTracking#removeOculusMobileSDKHeadTrackingListener(OculusMobileSDKHeadTrackingListener)
 * @see OculusMobileSDKHeadTracking#removeAllOculusMobileSDKHeadTrackingListeners()
 * @author ijamardo
 *
 */
public interface OculusMobileSDKHeadTrackingListener
{
	/**
	 * This is the method that will be called every time the the head tracking orientation changes. 
	 * @param oculusMobileSDKHeadTracking The origin of the event.
	 * @param x The x value of the orientation quaternion.
	 * @param y The y value of the orientation quaternion.
	 * @param z The z value of the orientation quaternion.
	 * @param w The w value of the orientation quaternion.
	 */
	public void orientationUpdated(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, float x, float y, float z, float w);
}
