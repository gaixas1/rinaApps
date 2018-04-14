#!/bin/bash

rina_include="/irati/include/"
rina_lib="/irati/lib/librina-api.so"
build_path="./bin/"
debug=0
info=0
max_pdu=1400
buffer_size=1400

usage(){ echo "Usage: $0 [-h] [-i <string>] [-l <string>] [-b <string>] [-p <int>] [-d] [-f].
i : include path, default \"${rina_include}\"
l : rina lib, default \"${rina_lib}\"
b : build path, default \"${build_path}\"
s : buffer size, default \"${buffer_size}\"
p : max sdu size, default \"${max_pdu}\"
d : debug, default false
f : info, default false" 1>&2; exit 1;}

while getopts ":hi:l:b:s:p:df" o; do
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
        s)
            buffer_size=${OPTARG}
            ;;
        p)
            max_pdu=${OPTARG}
            ;;
        d)
            debug=1
            ;;
        f)
            info=1
            ;;
    esac
done


compile_args=" -I ${rina_include} -L. ${rina_lib} -lpthread -D BUFF_SIZE=${buffer_size}" 
if [ ${debug} -eq 1 ]
then
    compile_args=${compile_args}" -D DEBUG"
fi
if [ ${info} -eq 1 ]
then
    compile_args=${compile_args}" -D INFO"
fi
if [ ${max_pdu} -lt ${buffer_size} ]
then
    compile_args=${compile_args}" -D MAX_PDU=${max_pdu}"
fi


mkdir -p ${build_path}

g++ -o ${build_path}DropServer DropServer.cpp ${compile_args}

g++ -o ${build_path}LogServer LogServer.cpp ${compile_args}

g++ -o ${build_path}DumpServer DumpServer.cpp ${compile_args}

g++ -o ${build_path}OnOffClient OnOffClient.cpp ${compile_args}

g++ -o ${build_path}DataClient DataClient.cpp ${compile_args}

g++ -o ${build_path}VideoClient VideoClient.cpp ${compile_args}

g++ -o ${build_path}VoiceClient VoiceClient.cpp ${compile_args}

g++ -o ${build_path}PoissonClient PoissonClient.cpp ${compile_args}

g++ -o ${build_path}Exponential Exponential.cpp ${compile_args}
