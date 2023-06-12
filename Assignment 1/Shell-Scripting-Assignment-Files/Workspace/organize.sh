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
        echo "$1"
    fi
}

evaluate()
{   
    # first parameter is language, C, Java or Python
    # second parameter is student id

    # cd "$target_location"/"$1"

    for each_code_file in "$target_location"/"$1"/"$2"/*
    do
        if [[ "$1" == "C" ]] && [[ "$each_code_file" != *.c ]]
        then
            continue
        elif [[ "$1" == "Java" ]] && [[ "$each_code_file" != *.java ]]
        then
            continue
        elif [[ "$1" == "Python" ]] && [[ "$each_code_file" != *.py ]]
        then
            continue
        fi

        matched=0
        not_matched=0
        # echo "$each_code_file"

        if [[ "$1" == "C" ]]
        then
            gcc "$each_code_file" -o "$target_location"/"$1"/"$student_id"/main.out
        elif [[ "$1" == "Java" ]]
        then
            javac "$each_code_file"
        fi

        for each_test_case in "$test_location"/*
        do
            # echo "$each_test_case"
            test_no=${each_test_case##*test}
            test_no=${test_no%%.txt}
            # echo "$test_no"

            if [[ "$1" == "C" ]]
            then
                "$target_location"/"$1"/"$2"/main.out < "$each_test_case" > "$target_location"/"$1"/"$2"/out"$test_no".txt
            elif [[ "$1" == "Java" ]]
            then
                cd "$target_location"/"$1"/"$2"
                java Main < ../../../"$each_test_case" > out"$test_no".txt
                cd ../../..
            elif [[ "$1" == "Python" ]]
            then
                python3 "$target_location"/"$1"/"$2"/main.py < "$each_test_case" > "$target_location"/"$1"/"$2"/out"$test_no".txt
            fi

            if [[ -z $(diff "$target_location"/"$1"/"$2"/out"$test_no".txt "$answer_location"/ans"$test_no".txt) ]]
            then
                # echo "matched"
                matched=$(($matched + 1))
            else 
                # echo "not matched"
                not_matched=$(($not_matched + 1))
            fi
        done
        echo "$2","$1",$matched,$not_matched >> "$target_location"/result.csv
    done    

    # cd ../..
}

if [[ $# < 4 ]]
then
    echo "Usage:"
    echo "./organize.sh <submission_folder> <target_folder> <test_folder> <answer_folder> [-v] [-noexecute]"
    echo ""
    echo "-v: verbose"
    echo "-noexecute: do not execute code files"
    echo ""
    exit 1
fi

# create the target directory
rm -rf "$target_location"
mkdir -p "$target_location"/C "$target_location"/Java "$target_location"/Python

mkdir -p "$submission_location"/unzipped_submissions

if [[ $# > 4 ]] && [[ $5 != "-noexecute" ]] && [[ $6 != "-noexecute" ]]
then
    touch "$target_location"/result.csv
    echo "student_id,type,matched,not_matched" > "$target_location"/result.csv
fi

for each_submission in "$submission_location"/*
do 
    if file -b --mime-type "$each_submission" | grep -q "application/zip" # b brief, q quiet
    then
        # echo "$each_submission"
        student_id=${each_submission##*_} # remove the longest prefix matching the pattern *_
        student_id=${student_id%%.zip} # remove the longest suffix matching the pattern .zip
        
        if [[ $# > 4 ]] && [[ $5 == "-v" ]]
        then
            echo "Organizing files of" "$student_id"
        fi

        unzip -qq "$each_submission" -d "$submission_location"/unzipped_submissions

        # for each in $(ls "$submission_location"/unzipped_submissions/*) -> here $each would contain just the file name, no path

        main_file_location=$(visit "$submission_location"/unzipped_submissions)

        # echo $main_file_location
        
        if [[ $main_file_location == *.c ]]
        then
            language=C
            mkdir "$target_location"/C/"$student_id"
            mv "$main_file_location" "$target_location"/C/"$student_id"/main.c
        elif [[ $main_file_location == *.java ]]
        then
            language=Java
            mkdir "$target_location"/Java/"$student_id"
            mv "$main_file_location" "$target_location"/Java/"$student_id"/Main.java
        elif [[ $main_file_location == *.py ]]
        then
            language=Python
            mkdir "$target_location"/Python/"$student_id"
            mv "$main_file_location" "$target_location"/Python/"$student_id"/main.py
        fi
        
        if [[ $# > 4 ]] && [[ $5 == "-v" ]]
        then
            echo "Executing files of" "$student_id"
        fi
        
        if [[ $# > 4 ]] && [[ $5 != "-noexecute" ]] && [[ $6 != "-noexecute" ]]
        then
            evaluate "$language" "$student_id"
        fi

        rm -rf "$submission_location"/unzipped_submissions/*
    fi

done

rm -rf "$submission_location"/unzipped_submissions