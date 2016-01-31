package com.judax.oculusmobilesdkheadtracking.test;

import com.judax.oculusmobilesdkheadtracking.OculusMobileSDKHeadTracking;
import com.judax.oculusmobilesdkheadtracking.OculusMobileSDKHeadTrackingListener;

import android.app.Activity;
import android.os.Bundle;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.TextView;

public class OculusMobileSDKHeadTrackingTestActivity extends Activity
{
	private OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking = new OculusMobileSDKHeadTracking();
	private TextView xTextView = null;
	private TextView yTextView = null;
	private TextView zTextView = null;
	private TextView wTextView = null;
	private TextEditsUpdateRunnable runnable = new TextEditsUpdateRunnable();
	
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
	private class TextEditsUpdateRunnable implements Runnable
	{
		public float x, y, z, w;
		
		@Override
		public void run()
		{
			xTextView.setText("x: " + String.format("%.4f", x));
			yTextView.setText("y: " + String.format("%.4f", y));
			zTextView.setText("z: " + String.format("%.4f", z));
			wTextView.setText("w: " + String.format("%.4f", w));
		}
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		
		setContentView(R.layout.main_layout);
		
		ViewGroup mainViewGroup = (ViewGroup)findViewById(R.id.mainLayout);
		
		xTextView = (TextView)findViewById(R.id.xTextView);
		yTextView = (TextView)findViewById(R.id.yTextView);
		zTextView = (TextView)findViewById(R.id.zTextView);
		wTextView = (TextView)findViewById(R.id.wTextView);
		
		// Initialize the oculus mobile sdk head tracking
		oculusMobileSDKHeadTracking.start(this);
		// Listen to orientation updates
		oculusMobileSDKHeadTracking.addOculusMobileSDKHeadTrackingListener(new OculusMobileSDKHeadTrackingListener()
		{
			@Override
			public void headTrackingOrientationUpdated(OculusMobileSDKHeadTracking oculusMobileSDKHeadTracking, float x, float y, float z, float w)
			{
				runnable.x = x;
				runnable.y = y;
				runnable.z = z;
				runnable.w = w;
				runOnUiThread(runnable);
			}
		});
		// It is necessary to add the view from the oculus mobile sdk head tracking to the view hierarchy.
		// This view does not do/render anything but it has to be in the hierarchy. 
		// For this reason, it will be added of 1 pixel size.
		mainViewGroup.addView(oculusMobileSDKHeadTracking.getView(), new LinearLayout.LayoutParams(1, 1));
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
