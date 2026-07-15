# ProGuard rules for RawToHdr
# Keep JNI native methods

-keep class com.rawtohdr.rawtohdr.converter.NativeHdrEngine {
    public *;
}

-keepclasseswithmembernames class * {
    native <methods>;
}

-keepclasseswithmembernames class * {
    @androidx.annotation.Keep <methods>;
}

# Keep libraw classes
-keep class org.libraw.** { *; }
