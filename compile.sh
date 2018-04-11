#!/bin/bash

rina_include="/irati/include/"
rina_lib="/irati/lib/librina-api.so"
build_path="./bin/"
debug=0
info=0

usage(){ echo "Usage: $0 [-h] [-i <string>] [-l <string>] [-b <string>] [-d] [-f].
i : include path, default \"${rina_include}\"
l : rina lib, default \"${rina_lib}\"
b : build path, default \"${build_path}\"
d : debug, default false
f : info, default false" 1>&2; exit 1;}

while getopts ":hi:l:b:df" o; do
    case ${o} in
        h)
            usage
            ;;
        i)
            rina_include=${OPTARG}
            ;;
        l)
            rina_lib=${OPTARG}
            ;;
        b)
            build_path=${OPTARG}
            ;;
        d)
            debug=1
            ;;
        f)
            info=1
            ;;
    esac
done


compile_args="-I ${rina_include} -L. ${rina_lib} -lpthread" 
if [ ${debug} -eq 1 ]
then
    compile_args=${compile_args}" -D DEBUG"
fi
if [ ${info} -eq 1 ]
then
    compile_args=${compile_args}" -D INFO"
fi

mkdir -p ${build_path}

g++ -o ${build_path}DropServer DropServer.cpp ${compile_args}

g++ -o ${build_path}LogServer LogServer.cpp ${compile_args}

g++ -o ${build_path}OnOffClient OnOffClient.cpp ${compile_args}

g++ -o ${build_path}DataClient DataClient.cpp ${compile_args}

g++ -o ${build_path}VideoClient VideoClient.cpp ${compile_args}

g++ -o ${build_path}VoiceClient VoiceClient.cpp ${compile_args}
