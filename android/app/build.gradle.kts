plugins {
    id("com.android.application")
}

android {
    namespace = "com.wolfensteinrpg"
    compileSdk = 37
    ndkVersion = "30.0.16248370"

    defaultConfig {
        applicationId = "com.wolfensteinrpg"
        minSdk = 26
        targetSdk = 37
        versionCode = 1
        versionName = "0.1"

        ndk {
            abiFilters += listOf("x86_64", "arm64-v8a")
        }
    }

    sourceSets {
        getByName("main") {
            java.srcDir(
                "src/main/cpp/SDL/android-project/app/src/main/java"
            )
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "4.1.2"
        }
    }
}