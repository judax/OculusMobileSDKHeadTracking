# Clean and build specifying the makefile to use and the output folder for the final libraries
echo "Rebuilding..."
$ANDROID_NDK_PATH/ndk-build NDK_APPLICATION_MK=./Application.mk -B
echo "Rebuilt!"
# Copy the final libaries to the default test if there were no errors
if [ $? -eq 0 ];then
	echo "Copying final libraries to the default test..."
	mkdir -p ../test/libs/armeabi-v7a
	cp build/armeabi-v7a/*.so ../test/libs/armeabi-v7a
	echo "Copied!"
fi

