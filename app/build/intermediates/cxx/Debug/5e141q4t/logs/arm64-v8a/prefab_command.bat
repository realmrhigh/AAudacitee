@echo off
"C:\\Program Files\\Eclipse Adoptium\\jdk-21.0.7.6-hotspot\\bin\\java" ^
  --class-path ^
  "C:\\Users\\stant\\.gradle\\caches\\modules-2\\files-2.1\\com.google.prefab\\cli\\2.0.0\\f2702b5ca13df54e3ca92f29d6b403fb6285d8df\\cli-2.0.0-all.jar" ^
  com.google.prefab.cli.AppKt ^
  --build-system ^
  cmake ^
  --platform ^
  android ^
  --abi ^
  arm64-v8a ^
  --os-version ^
  21 ^
  --stl ^
  c++_shared ^
  --ndk-version ^
  25 ^
  --output ^
  "C:\\Users\\stant\\AppData\\Local\\Temp\\agp-prefab-staging10614976653830567633\\staged-cli-output" ^
  "C:\\Users\\stant\\.gradle\\caches\\transforms-3\\750cd355f74f293a14287285b6361954\\transformed\\oboe-1.5.0\\prefab"
