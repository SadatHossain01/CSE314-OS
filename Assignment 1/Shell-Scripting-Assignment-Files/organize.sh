#!/bin/bash

submission_location=$1
target_location=$2
test_location=$3
answer_location=$4

visit() 
{
    # echo "$1"
    if [ -d "$1" ]
    then
        for i in "$1"/*
        do
            visit "$i"
        done
    elif [[ "$1" == *.c ]] || [[ "$1" == *.java ]] || [[ "$1" == *.py ]]
    then
        # echo "yay"
        echo "$1"
    fi
}

# create the target directory
mkdir -p "$target_location"/C "$target_location"/Java "$target_location"/Python

mkdir -p "$submission_location"/unzipped_submissions

for each_submission in "$submission_location"/*
do 
    if file -b --mime-type "$each_submission" | grep -q "application/zip" # b brief, q quiet
    then
        # echo "$each_submission"
        student_id=${each_submission##*_} # remove the longest prefix matching the pattern *_
        student_id=${student_id%%.zip} # remove the longest suffix matching the pattern .zip
        # echo "$student_id"
        unzip "$each_submission" -d "$submission_location"/unzipped_submissions

        # for each in $(ls "$submission_location"/unzipped_submissions/*) -> here $each would contain just the file name, no path

        main_file_location=$(visit "$submission_location"/unzipped_submissions)

        # echo $main_file_location
        
        if [[ $main_file_location == *.c ]]
        then
            mkdir "$target_location"/C/"$student_id"
            mv "$main_file_location" "$target_location"/C/"$student_id"/main.c
        elif [[ $main_file_location == *.java ]]
        then
            mkdir "$target_location"/Java/"$student_id"
            mv "$main_file_location" "$target_location"/Java/"$student_id"/Main.java
        elif [[ $main_file_location == *.py ]]
        then
            mkdir "$target_location"/Python/"$student_id"
            mv "$main_file_location" "$target_location"/Python/"$student_id"/main.py
        fi
        
        rm -r "$submission_location"/unzipped_submissions/*
    fi

done

rm -r "$submission_location"/unzipped_submissions