plugins {
    id("MagiskPlugin")
}

tasks.register("clean", Delete::class) {
    delete(rootProject.buildDir)
}
