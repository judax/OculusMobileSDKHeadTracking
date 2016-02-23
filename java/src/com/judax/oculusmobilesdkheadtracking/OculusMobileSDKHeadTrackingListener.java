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
}
