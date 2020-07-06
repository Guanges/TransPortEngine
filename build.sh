cd TransPortEngineSdk
make clean
make
cd ..
mkdir bin
mkdir TransPortEngine
cp ./TransPortEngineSdk/libTransPortEngineSdk.so ./TransPortEngine
tar -zcvf ./bin/TransPortEngine.tar.gz ./TransPortEngine
