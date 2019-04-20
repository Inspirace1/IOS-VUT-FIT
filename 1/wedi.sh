#!/bin/sh

#--------------------------------------------------------------------------#
#																		   #
#						Author: Tomáš Sasák - xsasak01					   #
#				  Name: WEDI - Ultimate text editor wrapper				   #
#						Made for IOS as Project 1 						   #
#							   VUT FIT 2018								   #
#																		   #
#--------------------------Function declarations---------------------------#
############################################################################
#--------------------------------------------------------------------------#

# Update - This function is called whenever any file that was previously edited with WEDI is runned. #
# Function parameters: $1 = File/Directory realpath/dirname #
# Function greps name and path of the file and updates WEDIRC text file with new informations 		 #
# Text file looks like:
#	TIMESUSED "NAMEOFFILE" REALPATHOFFILE DIRNAMEOFFILE DATEWHENOPENEDBYWEDI						#
# Update function greps TIMESUSED, increment it by one and saves into the wedirc, if file is opened #
# on date which has never been before, date is updated right before the new date 					#

update() 
{
	FDATE=`date +%Y%m%d`
	FNAME=`basename "$1"`
	FPATH=`realpath -q "$1"`
	RPATH=`realpath -q "$1"`
	RPATH="`grep "$RPATH" "$WEDI_RC"`"
	NUM=$(echo "$RPATH" | grep -o "[0-9]*\s" | head -1 | sed 's/.$//')
	NUM=$((NUM+1))
	RPATH=$(echo $RPATH | sed s/"\/"/\\\\\\//g)
	PREV_LAUNCHES=$(echo "$RPATH" | grep -o "\s[0-9]*$")

	TEMP=$(cat "$WEDI_RC" | grep -v "$RPATH")
	echo "$TEMP" > "$WEDI_RC"

	if echo "$RPATH" | grep -q "\s$FDATE\s" || echo "$RPATH" | grep -q "\s$FDATE"
	then
		echo "$NUM \"$FNAME\" $FPATH `dirname "$FPATH"` `date +%Y%m%d` " >> "$WEDI_RC"
	else
		echo "$NUM \"$FNAME\" $FPATH `dirname "$FPATH"` `date +%Y%m%d`"$PREV_LAUNCHES" " >> "$WEDI_RC"
	fi
}

# Function finds which finds the latest file by dirname " #
# Function parameters: $1 - dirname input #
# Function returns: realpath to the latest used file #
# Function greps out the directory-dirname , takes the latest line, cuts realpath of the file and returns the latest realpath #
# Latest line is the latest file edited #
find_latest()
{
	echo "$(grep "\s$1\s" "$WEDI_RC" | tail -1 | grep -o "\/.*\s\/" | head -c -3)"	
}

# Function finds which file was mostly used by dirname #
# Function parameters: $1 - dirname input #
# Function returns: Realpath to the most used file #
# Function greps out the directory-dirname matches, greps out all the first numbers (TIMESUSED), sorts them out by highest #
# and saves the highest number in the TIMESUSED variable. #
# Again greps out the directory-dirname matches, greps out the highest TIMESUSED number and greps out the realpath of the file #
most_used()
{
	TIMESUSED=$(grep "\s$1\s" "$WEDI_RC" | grep -o "^[0-9]*" | sort -nr | head -1)
	echo $(grep "\s$1\s" "$WEDI_RC" | grep "^$TIMESUSED\s" | tail -1 | grep -o "\/.*\s\/" | head -c -3)
}	

# Function finds out which files were used in current (input) folder #
# Function parameters: $1 - dirname input #
# Function returns: List of files which were edited in input folder by WEDI #
# Function greps out the dirname input and greps out the filenames	#
files_list()
{
	echo $(grep "\s$1\s" "$WEDI_RC" | grep -o "\s\".*\"\s" )
}

# Function finds out the files edited after input DATE #
# Function arguments: $1 dirname input #
#					  $2 date input	   #
# Function returns: List of files which were edited by WEDI after DATE #
# Function greps out the dirname input, greps out the dates, and filters out the numbers which are lower than date input. #
# After that, function greps out the dirname input and greps out matches with correct dates and greps out the "NAMEOFFILE" #
get_after()
{
	WANTED_DATE=$(echo "$2" | sed "s/-//g")
	DATES=$(grep "\s$1\s" "$WEDI_RC" | grep -o "\s[0-9]*" | sed '/^\s*$/d')
	for i in $DATES
	do
		if [ "$i" -lt "$WANTED_DATE" ]
		then
			DATES=$(echo "$DATES" | grep -v "$i" )
		fi
	done

	if [ "$DATES" = "" ]
	then
		echo "$DATES"
		return
	fi

	FILES=$(grep "\s$1\s" "$WEDI_RC" | grep "$DATES" | grep -o "\s\".*\"\s" )
	echo "$FILES"
}

# Function finds out the files edited before input DATE #
# Function arguments: $1 dirname input #
#					  $2 date input	   #
# Function returns: List of files which were edited by WEDI before DATE #
# Function greps out the dirname input, greps out the dates, and filters out the numbers which are greater or equal than date input. #
# After that, function greps out the dirname input and greps out matches with correct dates and greps out the "NAMEOFFILE" #
get_before()
{
	WANTED_DATE=$(echo "$2" | sed "s/-//g")
	DATES=$(grep "\s$1\s" "$WEDI_RC" | grep -o "\s[0-9]*" | sed '/^\s*$/d')
	for i in $DATES
	do
		if [ "$i" -ge "$WANTED_DATE" ]
		then
			DATES=$(echo "$DATES" | grep -v "$i" )
		fi
	done

	if [ "$DATES" = "" ]
	then
		echo "$DATES"
		return
	fi

	FILES=$(grep "\s$1\s" "$WEDI_RC" | grep "$DATES" | grep -o "\s\".*\"\s" )
	echo "$FILES"
}

# Function updates $WEDI_RC file, getting wedirc rid of files that doesnt exist anymore. #
# Function cat out the $WEDI_RC file, greps out the realpaths to the files. #
# After that, function cycles through the realpaths and tries if every realpath does exists. If some doesnt exists anymore, the line is deleted #
# from $WEDI_RC file. #
update_wedirc()
{
	FILE_PATHS=$(cat "$WEDI_RC" | grep ".*" | grep -o "\s/.*/.*\s/" | grep -o "/.*/*\s" | sed 's/.$//')

	echo "$FILE_PATHS" | while read line
	do
		if ! [ -f "$line" ]
		then
			line=$(echo $line | sed s/"\/"/\\\\\\//g)
			TEMP=$(cat "$WEDI_RC" | grep -v "$line")
			echo "$TEMP" > "$WEDI_RC"
		fi
	done
}

# Function checks out for conditions, which will cause WEDI to not run smoothly without errors. #
# Function checks out if utilities exists, also if WEDI_RC exists, if not, WEDI will creaty one itself. (also directories) #
error_check()
{
	if [ "$WEDI_RC" = "" ] || [ -d "$WEDI_RC" ]
	then
		echo "Error, WEDI_RC not exported."
		exit 1

	elif ! [ -f "$WEDI_RC" ]
	then
		if [ -d `dirname "$WEDI_RC"` ]
		then
			touch "$WEDI_RC"

		else
			mkdir `dirname "$WEDI_RC"`
			touch "$WEDI_RC"

		fi
	fi

	if ! type "realpath" > /dev/null
	then
		echo "Realpath utility, not found. You can install it by \"sudo apt-get install realpath\"~/." >&2
		exit 1
	fi

	if ! type "vi" > /dev/null
	then
		echo "Vi editor, not found. You can install it by \"sudo apt-get install vi\"~/." >&2
		exit 1
	fi
}

# Function sets out the users favourite editor, based on global variables $EDITOR. $VISUAL. #
# If these are not setted up, WEDI will use VI editor. #
editor_set()
{
	if ! [ "$EDITOR" = "" ]
	then
		echo "$EDITOR"

	elif ! [ "$VISUAL" = "" ]
	then
		echo "$VISUAL"

	else
		echo "vi"
	fi
}

#-----------------------------------Main-----------------------------------#
############################################################################
#--------------------------------------------------------------------------#

export POSIXLY_CORRECT=yes
error_check
CURRENT_DIR=`realpath .`
TEXT_EDITOR=$(editor_set)
update_wedirc

if  [ "$1" = "-m" ] # User wants mostly edited file.
then
	if [ "$2" != "" ] # Setting up the argument, if user input is empty, use current directory
	then
		ARG="`realpath "$2"`"
	else
		ARG="$CURRENT_DIR"
	fi

	if [ -d "$ARG" ] && grep -q "\s$ARG\s" "$WEDI_RC" # If directory exists and it was edited with wedi
	then
		FPATH=$(most_used $ARG)
		update "$FPATH"
		$TEXT_EDITOR "$FPATH"
	else
		echo "No file was ever edited in this folder." >&2
		exit 1
	fi

elif [ "$1" = "-l" ] # User wants list of edited files by WEDI in folder.
then
	if [ "$2" != "" ]
	then
		ARG="`realpath "$2"`"
	else
		ARG="$CURRENT_DIR"
	fi

	if [ -d "$ARG" ] && grep -q "\s$ARG\s" "$WEDI_RC"
	then
		FILES=$(files_list "$ARG")
		echo "$FILES"
	else
		echo "Cannot show list of files." >&2
		exit 1	
	fi

elif [ "$1" = "-a" ] && echo "$2" | grep -qE "[0-9]{4}-[0-9]{2}-[0-9]{2}" && [ "$2" != "" ] # User wants list of edited files after some date in some folder by WEDI.
then
	if [ "$3" = "" ]
	then
		ARG="$CURRENT_DIR"
	else
		ARG="`realpath "$3"`"
	fi

	FILES=$(get_after "$ARG" "$2")
	echo "$FILES"
	exit 0

elif [ "$1" = "-b" ] && echo "$2" | grep -qE "[0-9]{4}-[0-9]{2}-[0-9]{2}" && [ "$2" != "" ] # User wants list of edited files before some date in some folder by WEDI.
then
	if [ "$3" = "" ]
	then
		ARG="$CURRENT_DIR"
	else
		ARG="`realpath "$3"`"
	fi

	FILES=$(get_before "$ARG" "$2")
	echo "$FILES"
	exit 0

elif [ "$1" != "" ] && [ "$2" = "" ] # User wants to edit some file with WEDI
then
	ARG="`realpath "$1"`" # Setting up the arguments realpath 
	if grep -q "\s$ARG\s" "$WEDI_RC" && [ -f "$ARG" ] # If file exists and it has already been edited with WEDI
	then
		update "$ARG" # Update the record in $WEDI_RC
		$TEXT_EDITOR "$ARG" # Starts up the editor with file
	elif [ -f "$ARG" ] # If file exists and it is first time edited with WEDI
	then
		FNAME=`basename "$ARG"`
		FPATH=`realpath -q "$ARG"`
		echo "1 \"$FNAME\" $FPATH `dirname "$FPATH"` `date +%Y%m%d` " >> "$WEDI_RC" # Format of the new file added to the data, the format is: NumberTimesFileEdited FileName Path Date Time EpochTime 		
		$TEXT_EDITOR "$1"
	elif grep -q "\s$ARG\s" "$WEDI_RC" && [ -d "$ARG" ] # If argument is directory and some files had been edited inside the folder by WEDI (latest edited file)
	then
		FPATH=$(find_latest "$ARG") # Getting latest file realpath
		update "$FPATH" # Updating record in $WEDI_RC
		$TEXT_EDITOR "$FPATH"
	else # If nothing ŕequested exists
		echo "File nor folder found." >&2
		exit 1
	fi

elif [ "$1" = "" ] && grep -q "\s$CURRENT_DIR\s" "$WEDI_RC" # User wants to edit latest file in CURRENT folder
then
	FPATH=$(find_latest "$CURRENT_DIR")
	update "$FPATH"
	$TEXT_EDITOR "$FPATH"

else # No match 
	echo "Wrong input! Check manual." >&2
	exit 1
fi

exit 0