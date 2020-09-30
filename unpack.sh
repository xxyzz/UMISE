#!/bin/bash

command_exists() {
    command -v "$@" >/dev/null 2>&1
}

main() {
    if ! command_exists innoextract; then
        echo "innoextract not installed"
        exit 1;
    fi
    if ! command_exists vgmstream_cli; then
        echo "vgmstream_cli not installed"
        exit 1
    fi
    if [[ $# -ne 2 ]]; then
        echo "Usage: ./unpack.sh ./path_of_installer_exe ./path_of_output_folder"
        exit 1;
    fi

    EXE_PATH=$1;
    OUT_PATH=$2;
    PAK_PATH=$OUT_PATH"/app/Monkey1.pak"
    UNPACK_PATH=$OUT_PATH"/unpack_pak/"
    AUDIO_PATH=$OUT_PATH"/app/audio"

    # unpack installer
    if [[ -d $OUT_PATH && -n "$(ls -A $OUT_PATH)" ]]; then
        echo "Skip unpack installer"
    else
        innoextract $EXE_PATH -d $OUT_PATH
    fi

    make
    # unpack Monkey1.pak
    if [[ -d $UNPACK_PATH && -n "$(ls -A $UNPACK_PATH)" ]]; then
        echo "Skip unpack .pak"
    else
        ./unpackpak -i $PAK_PATH -o $UNPACK_PATH
    fi

    # unpack se sound files
    if [[ -n "$(find $AUDIO_PATH -name '*.wav')" ]]; then
        echo "Skip unpack .xwb files"
    else
        cd $AUDIO_PATH
        for xwb_file in "$AUDIO_PATH"/*.xwb; do
            STREAM_COUNT=$(vgmstream_cli -m $xwb_file | \
                               awk '$1 == "stream" && $2 == "count:" { print $3 }')
            WAV_FILENAME="?n.wav"
            if [[ $xwb_file =~ "New" || $xwb_file =~ "Origin" ]]; then
                WAV_FILENAME=$(basename $xwb_file .xwb)"_?n.wav"
            fi
            for i in $(seq 1 $STREAM_COUNT); do
                vgmstream_cli -o $WAV_FILENAME -s $i $xwb_file
            done
        done
    fi
}

main "$@"
