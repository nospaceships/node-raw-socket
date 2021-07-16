declare -a nodeVersions=("6" "7" "8" "9" "10" "11" "12" "13" "14")

for (( i=0; i<${#nodeVersions[@]}; i++ ));
do
  echo "Building for Node v${nodeVersions[$i]}.0.0"
  prebuildify --target node@"${nodeVersions[$i]}.0.0" --platform win32 --arch x64
  prebuildify --target node@"${nodeVersions[$i]}.0.0" --platform win32 --arch arm64
  prebuildify --target node@"${nodeVersions[$i]}.0.0" --platform linux --arch x64
done