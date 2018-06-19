#!/usr/bin/perl -w

print "Begin to generate the Script.tcl file\n";
print "input keyword \n"; 

##====(1) define
my $key_code_name;
my $key_code_base_path_name;
my $key_code_name_value;
my $key_code_base_path_name_value;

##====(2) delete the first input character when it including the "/" or "." and so on.
$_=$ARGV[0];
s/\W+\s*$//g; ## delete the "/" when the character including the "/" or "." and so on.
$key_code_base_path_name_value = $_;

$_=$ARGV[1];
s/\W+\s*$//g; ## delete the "/" when the character including the "/" or "." and so on.
$key_code_name_value = $_;

$key_code_name = "codeName";
$key_code_base_path_name = "codeBasePath";

$_=$ARGV[2];
$file_path_name = "$_"; 
$file_path_name_tmp = "Script.tcl";


## Create the
if (! open RTL_FILE, "<",$file_path_name)
{
	die "Cann't open the $file_path_name:$!";
}

if (! open RTL_FILE_TMP, ">",$file_path_name_tmp)
{
	die "Cann't open the $file_path_name_tmp:$!";
}


## deal one line by one line
foreach ( <RTL_FILE> )
{
#	if( ($_ =~ /$key_code_name/) && !($_ =~ /\$/) ) 
    if( ($_ =~ /$key_code_name/) && ($_ !~ /\$/) ) 
	{
		print "This file [$file_path_name] has the key word = $key_code_name\n";
		print RTL_FILE_TMP "set $key_code_name $key_code_name_value\n";
	}
    elsif( ($_ =~ /$key_code_base_path_name/) && ($_ !~ /\$/) ) 
    {
		print "This file [$file_path_name] has the key word = $key_code_base_path_name\n";
		print RTL_FILE_TMP "set $key_code_base_path_name $key_code_base_path_name_value\n";
    }
	else
    {	
	    print RTL_FILE_TMP "$_";		
    }	
}
close RTL_FILE;
close RTL_FILE_TMP;

system("chmod 777 $file_path_name_tmp");

print "Finished to generate the Script.tcl file\n";

#print "send file in server done \n";


