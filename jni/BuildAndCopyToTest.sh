# Rebuilt (-B) specifying the makefile to use and the output folder for the final libraries and intermediate files
echo "Rebuilding..."
$ANDROID_NDK_PATH/ndk-build NDK_APPLICATION_MK=./Application.mk NDK_LIBS_OUT=../build NDK_OUT=./objs -B
if [ $? -ne 0 ];then exit $?;fi
echo "Rebuilt!"
echo "Copying final libraries to the default test and the crosswalk test..."
mkdir -p ../test/libs/armeabi-v7a
if [ $? -ne 0 ];then exit $?;fi
cp ../build/armeabi-v7a/*.so ../test/libs/armeabi-v7a
if [ $? -ne 0 ];then exit $?;fi
echo "Copied!"

