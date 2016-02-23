package com.judax.oculusmobilesdkheadtracking.test;

import java.util.Timer;
import java.util.TimerTask;

import com.judax.oculusmobilesdkheadtracking.OculusMobileSDKHeadTracking;
import com.judax.oculusmobilesdkheadtracking.OculusMobileSDKHeadTrackingData;
import com.judax.oculusmobilesdkheadtracking.OculusMobileSDKHeadTrackingListener;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

public class OculusMobileSDKHeadTrackingTestActivity extends Activity
{
	private static final String HEAD_TRACKING_TEXT_VIEW_TEXT = "Head Tracking State: ";
	private OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking = new OculusMobileSDKHeadTracking();
	private TextView headTrackingTextView = null;
	private TextView fovTextView = null;
	private TextView interpupillaryDistanceTextView = null;
	private TextView timeStampTextView = null;
	private TextView orientationTextView = null;
	private TextView linearVelocityTextView = null;
	private TextView linearAccelerationTextView = null;
	private TextView angularVelocityTextView = null;
	private TextView angularAccelerationTextView = null;
	private HeadTrackingDataTextEditsUpdateRunnable runnable = new HeadTrackingDataTextEditsUpdateRunnable();
	
	/**
	 * Shows the head tracking orientation values on the corresponding TextView-s.
	 * As values from the OculusMobileSDKHeadTracking system will be provided in a different thread
	 * from the UI thread, this runnable will make it possible to show the values.
	 * Instead of creating a new Runnable on each orientation update call, just create a single object
	 * and reuse it. 
	 * 
	 * @author JudaX
	 *
	 */
	private class HeadTrackingDataTextEditsUpdateRunnable implements Runnable
	{
		@SuppressLint("DefaultLocale")
		private String getString(float x, float y, float z)
		{
			return String.format("(%.4f, %.4f, %.4f)", x, y, z);
		}
		
		@SuppressLint("DefaultLocale")
		private String getString(float x, float y, float z, float w)
		{
			return String.format("(%.4f, %.4f, %.4f, %.4f)", x, y, z, w);
		}
		
		@Override
		public void run()
		{
			OculusMobileSDKHeadTrackingData data = oculusMobileSDKHeadTracking.getData();
			fovTextView.setText("fov: " + String.format("(%.4f, %.4f)", data.xFOV, data.yFOV));
			interpupillaryDistanceTextView.setText("interpupillary distance: " + String.format("%.4f", data.interpupillaryDistance));
			timeStampTextView.setText("timeStamp: " + String.format("%.4f", data.timeStamp));
			orientationTextView.setText("orientation: " + getString(data.orientationX, data.orientationY, data.orientationZ, data.orientationW));
			linearVelocityTextView.setText("linearVelocity: " + getString(data.linearVelocityX, data.linearVelocityY, data.linearVelocityZ));
			linearAccelerationTextView.setText("linearAcceleration: " + getString(data.linearAccelerationX, data.linearAccelerationY, data.linearAccelerationZ));
			angularVelocityTextView.setText("angularVelocity: " + getString(data.angularVelocityX, data.angularVelocityY, data.angularVelocityZ));
			angularAccelerationTextView.setText("angularAcceleration: " + getString(data.angularAccelerationX, data.angularAccelerationY, data.angularAccelerationZ));
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.main_layout);
		
		ViewGroup mainViewGroup = (ViewGroup)findViewById(R.id.mainLayout);
		
		headTrackingTextView = (TextView)findViewById(R.id.headTrackingTextView);
		headTrackingTextView.setText(HEAD_TRACKING_TEXT_VIEW_TEXT + "Not started yet");
		fovTextView = (TextView)findViewById(R.id.fovTextView);
		interpupillaryDistanceTextView = (TextView)findViewById(R.id.interpupillaryDistanceTextView);
		timeStampTextView = (TextView)findViewById(R.id.timeStampTextView);
		orientationTextView = (TextView)findViewById(R.id.orientationTextView);
		linearVelocityTextView = (TextView)findViewById(R.id.linearVelocityTextView);
		linearAccelerationTextView = (TextView)findViewById(R.id.linearAccelerationTextView);
		angularVelocityTextView = (TextView)findViewById(R.id.angularVelocityTextView);
		angularAccelerationTextView = (TextView)findViewById(R.id.angularAccelerationTextView);
		
		// Register to listen to Oculus Mobile SDK head tracking events
		oculusMobileSDKHeadTracking.addOculusMobileSDKHeadTrackingListener(new OculusMobileSDKHeadTrackingListener()
		{
			@Override
			public void headTrackingStarted(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, OculusMobileSDKHeadTrackingData data)
			{
				runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						headTrackingTextView.setText(HEAD_TRACKING_TEXT_VIEW_TEXT + "Started");
					}
				});
			}
			
			@Override
			public void headTrackingError(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, final String errorMessage)
			{
				runOnUiThread(new Runnable()
				{
					@Override
					public void run()
					{
						headTrackingTextView.setText(HEAD_TRACKING_TEXT_VIEW_TEXT + "Error");
						System.err.println("Head Tracking Error: " + errorMessage);
					}
				});
			}
		});
		
		// Initialize the oculus mobile sdk head tracking
		oculusMobileSDKHeadTracking.start(this);
		
		// It is necessary to add the view from the oculus mobile sdk head tracking to the view hierarchy.
		// This view does not do/render anything but it has to be in the hierarchy. 
		// For this reason, it will be added of 1 pixel size.
		// This is definitively not the best way to add the view.
		mainViewGroup.addView(oculusMobileSDKHeadTracking.getView(), new LinearLayout.LayoutParams(1, 1));
		
		// A task to update the UI information at 60 FPS
    Timer timer = new Timer();
    TimerTask doAsynchronousTask = new TimerTask() {       
        @Override
        public void run() {
  				runOnUiThread(runnable);
        }
    };
    timer.schedule(doAsynchronousTask, 0, 16);
	}
	
	@Override
	protected void onResume()
	{
		super.onResume();
		oculusMobileSDKHeadTracking.resume();
	}
	
	@Override
	protected void onPause()
	{
		super.onPause();
		oculusMobileSDKHeadTracking.pause();
	}
	
	@Override
	protected void onDestroy()
	{
		super.onDestroy();
		oculusMobileSDKHeadTracking.stop();
	}
}
