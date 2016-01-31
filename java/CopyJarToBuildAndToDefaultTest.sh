echo "Copying jar files..."
# Create the destionation folders in case they do not exist
mkdir -p ../test/libs/
if [ $? -ne 0 ];then exit $?;fi
mkdir -p ../build
if [ $? -ne 0 ];then exit $?;fi
# Copy the output jar to the final build folder and to the default test libs folder
cp bin/oculusmobilesdkheadtracking.jar ../build
if [ $? -ne 0 ];then exit $?;fi
cp bin/oculusmobilesdkheadtracking.jar ../test/libs
if [ $? -ne 0 ];then exit $?;fi
cp ovr_sdk_mobile_1.0.0.0/*.jar ../test/libs
if [ $? -ne 0 ];then exit $?;fi
echo "Copied!"
