echo "Generating javadoc..."
rm -rf ../javadoc
javadoc -classpath $ANDROID_HOME/platforms/android-9/android.jar -d ../javadoc src/com/judax/oculusmobilesdkheadtracking/*.java
if [ $? -ne 0 ];then exit $?;fi
echo "Javadoc generated!"
