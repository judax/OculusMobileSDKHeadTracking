package com.judax.oculusmobilesdkheadtracking;

/**
 * The interface to be implemented by any class that would like to be notified of
 * the Oculus Mobile SDK Head Tracking events.
 * @see OculusMobileSDKHeadTracking
 * @see OculusMobileSDKHeadTrackingData
 * @see OculusMobileSDKHeadTracking#addOculusMobileSDKHeadTrackingListener(OculusMobileSDKHeadTrackingListener)
 * @see OculusMobileSDKHeadTracking#removeOculusMobileSDKHeadTrackingListener(OculusMobileSDKHeadTrackingListener)
 * @see OculusMobileSDKHeadTracking#removeAllOculusMobileSDKHeadTrackingListeners()
 * @author ijamardo
 *
 */
public interface OculusMobileSDKHeadTrackingListener
{
	public void headTrackingStarted(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, OculusMobileSDKHeadTrackingData data);
	
	public void headTrackingError(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, String errorMessage);
	
	/**
	 * This is the method that will be called every time the the head tracking is updated. 
	 * @param oculusMobileSDKHeadTracking The origin of the event.
	 */
	public void headTrackingUpdated(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, OculusMobileSDKHeadTrackingData data);
}
