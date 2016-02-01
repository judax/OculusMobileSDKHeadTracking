# OculusMobileSDKHeadTracking

The Oculus Mobile SDK allows to access a hihly accurate head tracking system available on devices like the Samsung Gear VR by Samsung and Oculus. This library exposes just the access to this head tracking system in the easiest possible way. Any application or library can use this information to develop applications that can take advantage of the high accuracy information that is provided by these sensors.

## Folder Structure

* **3rdparty**: The third party libraries used to build this library. In this case the Oculus Mobile SDK libraries are needed.
* **build**: The final build of this library. You can use these final products if you do not want to build the library yourself.
* **java**: The java side of the library. This is the final API that other projects will see/use.
* **javadoc**: The javadoc of the java side of the library.
* **jni**: The C++ side of the library. In fact, this code is the one that handles the Oculus Mobile SDK communication. All the makefiles to be able to build the code are provided.
* **test**: A simple test that shows how to use the library. It simply shows the orientation quaternion value on screen.

## Assumptions

* This documentation will assume that you have experience on Android development and more specifically on using Eclipse to develop Android projects.
* This project provides some already built libraries. Of course, you can decide to use other versions of the same libraries or even build them yourself.
* Although some links and information will be provided, some knowledge on the needs to be able to develop and specially to deploy Samsung Gear VR Oculus mobile apps will be very helpful.

## How to use the library

The easiest way to have a glimpse on how to use the library is to check the test project. Anyway, these are the steps that should be followed in order to include the library in your project.

1. Copy the already built libraries to your Eclipse project's `libs` folder. You will need to copy:

```
build/oculusmobilesdkheadtracking.jar
3rdparty/ovr_sdk_mobile_1.0.0.0/SystemUtils.jar
3rdparty/ovr_sdk_mobile_1.0.0.0/VrApi.jar
```

Also copy the whole `build/armeabi-v7a` folder to your projects `libs` older.

2. Use the library inside your code. The API is the simplest possible but some steps needs to be followed.
  1. Create an `OculudMobileSDKHeadTracking` instance.
  2. Call the `start` method of the instance passing a reference to the Activity that will include the 
  3. Call the `getView` method of the instance to get a view that needs to be added somehow in the view hierarchy of your app.
  4. Call the `resume`, `pause` and `stop` methods of the instance in the corresponding `onResume`, `onPause` and `onDestroy` of the Activity.

3. Include your Oculus Signature File in the assets folder of your project. You can generate yout Oculus OSIG file at [https://developer.oculus.com/osig/](https://developer.oculus.com/osig/)

## Related Projects

https://github.com/judax/OculusMobileSDKHeadTrackingXWalkViewExtension
https://github.com/judax/OculusMobileSDKHeadTrackingWebVR

## Other References

Oculus Mobile SDK website



IDEAS

* Add explanation on how the final Oculus libraries have been acquired: some already compiled. openglloader has to be compiled and included as whole.

* Different projects
	- OculusMobileSDKHeadTracking
		+ Add pause and resume
		+ Add a listener (decide what info to provide from the headtracking pose)
	- OculusMobileSDKHeadTrackingCrosswalkExtension
	- OculusMobileSDKHeadTrackingCordovaPlugin
	- OculusMobileSDKheadTrackingCrosswalkApp

* Explain why/how xcode
	- Remove build phases.
	- Add the ndk build script phase.
	- If someone wants to add a different IDE support, is more than welcome.
	- Importance of having the ANDROID_NDK_PATH environment variable set.

* Explain why/how Eclipse
	- The version used INDIGO (it should work on newer versions)
	- The builder to copy the jar to the build folder and to the default test.
	- the execution of the .sh to convert JS in one line and the limitations of that.
	- explain that the test needs to be refreshed to be able to see the changes in the other project. Another option is to link both projects in Eclipse.
	- explain the use of crosswalk with a link of the two eclipse projects.
	- explain that libraries are copied to the libs folder and used directly from there.

* Explain what has been taken from the Oculus Mobile SDK v 1.0.0.0
	- Taken only the minimum libraries needed.
	- How the openglloader library has been compiled.

* Explain how the test works copying the final library to the libs folder of the eclipse project

* Explain that I have built everything using NDK-r9b

* Have two different sections: one "How to use the library" and the other for the ones that want to build it and contribute to the project.
	- To be able to test the APKs, the osigs needs to be added to the assets folder. Add links to explanations on how to generate the osigs.

* Explain that is necessary to have the ANDROID_HOME environment varibale set to be able to create javadoc.

