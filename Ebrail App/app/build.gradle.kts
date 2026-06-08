plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.example.ebrail"
    compileSdk = 36

    defaultConfig {
        applicationId = "com.example.ebrail"
        minSdk = 24
        targetSdk = 36
        versionCode = 1
        versionName = "1.0"

        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
}

dependencies {

    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.activity)
    implementation(libs.constraintlayout)
    implementation(libs.recyclerview)
    implementation(libs.cardview)
    testImplementation(libs.junit)
    androidTestImplementation(libs.ext.junit)
    androidTestImplementation(libs.espresso.core)
}

// Add compiler arg so Gradle prints full deprecation warnings when compiling Java sources
tasks.withType(org.gradle.api.tasks.compile.JavaCompile::class.java).configureEach {
    options.compilerArgs.add("-Xlint:deprecation")
}
