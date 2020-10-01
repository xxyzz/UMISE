#!/bin/bash

command_exists() {
    command -v "$@" >/dev/null 2>&1
}

check_dependency() {
    if ! command_exists $1; then
        echo "$1 is not installed"
        exit 1;
    fi
}

check_dependencies() {
    COMMANDS=('innoextract' 'vgmstream_cli' 'ffmpeg')
    for command in "${COMMANDS[@]}"; do
        check_dependency $command
    done
}

unpack_installer() {
    # $1: OUT_PATH, $2: EXE_PATH
    if [[ -d $1 && -n "$(ls -A $1)" ]]; then
        echo "Skip unpack installer"
    else
        echo "Unpack installer"
        innoextract $2 -d $1
    fi
}

unpack_pak() {
    # $1: UNPACK_PATH, $2: $PAK_PATH
    make
    if [[ -d $1 && -n "$(ls -A $1)" ]]; then
        echo "Skip unpack .pak"
    else
        echo "Unpack pak file"
        ./extractpak -i $2 -o $1 -g
    fi
}

# unpack se sound files
unpack_xwb() {
    # $1: AUDIO_PATH
    if [[ -n "$(find $1 -name '*.wav')" ]]; then
        echo "Skip unpack .xwb files"
    else
        echo "Unpack xwb files"
        cd $1
        for xwb_file in ./*.xwb; do
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

# convert wav to flac
convert_wav() {
    # $1: AUDIO_PATH
    if [[ -n "$(find ./ -name '*.flac')" ]]; then
        echo "Skip convert wav files"
    else
        if [[ ! $(pwd) =~ "app/audio" ]]; then
           cd $1 # unpack xwb is skiped
        fi
        echo "Convert wav to flac"
        for wav_file in MusicNew_track*.wav; do
            INDEX="${wav_file#*track}"
            INDEX="${INDEX%.*}"
            if [[ ! $INDEX =~ ^[0-9]+$ ]]; then
                continue
            fi
            INDEX=$((INDEX - 1))
            ffmpeg -i $wav_file track"$INDEX".flac
        done

        # concatenate track18b and track18c
        TRACK18=('MusicNew_track18b.wav' 'MusicNew_track18c.wav')
        ffmpeg -f concat -safe 0 -i                                        \
               <(for f in "${TRACK18[@]}"; do echo "file '$PWD/$f'"; done) \
               track17.flac
    fi
}

copy_files() {
    # $1: OUT_PATH
    SCUMMVM_FOLDER="monkey"
    if [[ -d $SCUMMVM_FOLDER && -n "$(ls -A $SCUMMVM_FOLDER)" ]]; then
        echo "Skip copy files"
    else
        # cd to OUT_PATH
        if [[ ! $(pwd) =~ "app/audio" ]]; then
            cd $1
        else
            cd ../../
        fi
        echo "Move game files and audio files"
        if [[ ! -d $SCUMMVM_FOLDER  ]]; then
           mkdir $SCUMMVM_FOLDER
        fi
        cp app/audio/*.flac $SCUMMVM_FOLDER
        cp unpack_pak/classic/en/* $SCUMMVM_FOLDER
    fi
}

main() {
    if [[ $# -ne 2 ]]; then
        echo "Usage: ./unpack.sh ./path_of_installer ./path_of_output_folder"
        exit 1;
    fi

    check_dependencies

    EXE_PATH=$1;
    OUT_PATH=$2;
    PAK_PATH=$OUT_PATH"/app/Monkey1.pak"
    UNPACK_PATH=$OUT_PATH"/unpack_pak/"
    AUDIO_PATH=$OUT_PATH"/app/audio"

    unpack_installer $OUT_PATH $EXE_PATH
    unpack_pak $UNPACK_PATH $PAK_PATH
    unpack_xwb $AUDIO_PATH
    convert_wav $AUDIO_PATH
    copy_files $OUT_PATH
}

main "$@"
