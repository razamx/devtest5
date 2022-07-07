#!/usr/bin/perl

my $exit_code = 0;

###########################################################################
#
# file: kwbuild.pl
#
# description:
# -----------
# This script is used to peform a Klocwork build on a server installation. 
# It should meet the build automation demands of the majority of Intel Klocwork users
# out-of-box. You can also use it as a basis to customize for more 
# specialized builds.  See the Klocwork Command Reference for details on
# all the various switches that can be used by the kwbuildproject application.
#
# TODO:
# 1) Rename it to a shorter name like: kwbuild.pl and copy it into your
#   <kw install dir>/bin dir (so that its on your system execution PATH).
#
# 2) Configure the fully qualified name of your KW Server name in the $KWHOST var below.
#	 (i.e., replace "<your server name>.amr.intel.com" with your server's name)
#
# 3) Done! It's now ready to use.
#
#
# running "kwbuild.pl"
# ---------------------
#
# examples:
#
# kwbuild.pl -version
#
# kwbuild.pl -help
#
# kwbuild.pl -project <projectname> -buildspec <buildspec> -buildname <buildname> -tables ~/kwtables/proj_A
#
# kwbuild.pl -project <projectname> -bs <buildspec-A> -bs <buildspec-B> -buildname <buildname> -tables ~/kwtables/proj_A
#
# kwbuild.pl -p <projectname> -bs <buildspec> -b <buildname> -debug
#
# kwbuild.pl -project <projectname> -buildspec <buildspec> -buildname <buildname> -tables <tables_dir path> -opts <string of comma separated compiler options, e.g., -I/....., -D/.....>
#
# kwbuild.pl -project <projectname> -buildspec <buildspec> -buildname <buildname> -tables <tables_dir path> -maxjobs 2 -V buildspecvar
#
#
#
# command line parameters/arguments:
# ----------------------------------------------------------------------------------------
#
#  -kwhost <kwhost>			: name of the KW host to use
#  -project <projectname>	: name of the KW project to use
#  -buildname <buildname>	: name to use for the build name. See NOTE1
#  -buildspec <buildspec>	: build spec file to process
#  -debug                   : run in debug mode, do not execute
#  -h or -help				: print out usage information
#  -t <path>				: Path to an existing tables dir
#  -v or -version           : print our version info
#  -mj <#CPUs>				: number of CPU cores available for concurrent scanning
#  -o <-Ipath,-Dstrng,...>	: string of comma separated -I include paths or -D macros
#  -V <string>				: build template variable name
#
#
# =========================================================================================
# NOTES:
#  1: If -project is not used specified, will look for an environment
#     var BT_PROJECT_NAME, if not found, will error and fail.
#     If -buildspec is not used specified, will look for an environment
#     var BT_BUILDSPECFILE, if not found, and -timestamp (-ts) is not
#     activated, then will display error and fail.  Same applies to 
#     buildname.
#
#  2: The following environment variables are checked for, and if
#     present, used:
#     BT_PROJECT		- the Klocwork project name
#     BT_BUILDNUMBER 	- the Klocwork buildname
#     BT_BUILDSPECFILE	- the build spec used as input
#
# ******************************************************************************************
#
# history
# =======
#
# 04-29-2008	ver 1.0 Initial starter script updated from a K7.7 script
# 07-28-2008	ver 1.1 Added --no-copy-tables to kwadmin load
# 08-25-2008	ver 1.2	Added --project-host, --project-port to kwbp
# 09-17-2008	Ver 1.3 Added --project $PROJ_NAME to kwbp command. Removed
#		        use of an explicit --errors-config option, so that the script
#       		will always import the pconf file associated to the project.
# 10-31-2008	Ver 1.4 Changed rmtree(“$KWTABLES”,1,1) to rmtree(“$KWTABLES”,0,1)
#				to make Clean silent (less screen noise).
#				Removed $KWSHARE and $KWCONFDIR mechanisms. They were optional
#				and never really used. 
# 01-09-2009	Ver 2.0 Major cleanup of script (no new major functionality added):
#					- Consolidated into a single script for both Windows and Linux
#					  by making user specifiy '-maxjobs <x>' on the command line
#					- Removed the Restart logic (not really useful)
#					- Clarified usage of a temp tables dir vs. an existing tables dir (--force)
#						Removed use of USERNAME in temp tables path name.
#					- Cleaned up Usage printout format
#					- Miscellaneous script format touchups
#
# 03-04-2010	Ver 2.1 Added the -opts/ -o arg handler to allow user to pass in 
#				special vars into kwbuildproject using the --add-compiler-options
#				switch.
#
# 05/13/2010	Ver 3.0 Updated to handle significant changes in KW9.x:
#					- Added the the --ssl arg to kwbuildproject and kwadmin cmds
#					for SSL enabled KW servers (SSL is the recommended config)
#					- Replaced --project-host and --project-port switches in kwbuildproject
#					with --host and --port.  The former switches were deprecated in KW9.x.
#					- Removed the --no-copy-tables arg in kwadmin command as this is now the 
#					default behavior in KW9.x
#					- Changed the license server paths as EC changed the FlexLM servers.
#					- Changed coding of PROJ_NAME comparison from:
#					  [if ($projects =~ m/^$PROJ_NAME\s*$/m)] to:
#					  [if ($projects =~ m/^$PROJ_NAME\b/m)] in order to find 'branched' names.
#
# 10/28/2010	Ver 3.1 
#					- Added support for -V (or -bsv) to use a build template file (.tpl) in place of a standard
#					  buildspec (.out). This allows you to specify the ENV VAR to substitute into the .tpl
#					- No longer support two scripts, one with --ssl and one without. Consolidated back
#					  to one script.  By default --ssl is not included in the kwbuildproject and kwadmin
#					  command strings.  End user will have to add that per notes below.
#					- Defaulted the $KWPORT var to 8080 (the KW9x default)
#
# 02/02/2011	Ver 3.2 
#					- Changed the default build type to --incremental from --force.  We are promoting
#					  incremental builds to optimize for speed.
# 02/10/2011	Ver 3.3
#					- Restored the logic that checks for the BT_PROJECT env var. This was inadvertently taken out in a previous version.
#					- Added more info on MAXJOBS var, i.e., that if not explicitly set in command that it defaults
#					  to Auto.
# 06/3/2011		Ver 3.4
#					- Added ability to use multiple buildspec by specifing multiple -bs (or -buildspec) args.
#					- Removed reference to the dp2slic103 license server. It is no longer used. 
# 12/09/11		Ver 3.5
#					- Changed the placement of $SPEC_FILE in the $KWBP command string. Put it last to correctly handle case of multiple spec files.
# 07/02/12		Ver 3.6
#					- Removed references to --copy-sources option as this has been removed from KW9.5.
#					  
##########################################################################################################################################
use Cwd;
use File::Copy;
use File::Path;

#
# Globals
# =======
#
my $VERSION = "3.6 by Intel ITS3 Klocwork Support (July 2, 2012)";
my $cmd = "";
my $rc = 0;

#
# Klocwork specific variables and paths
# -------------------------------------

#
# KWHOST       The hostname or IP address of the Klocwork Server (i.e., Web + Project)
#               
#my $KWHOST = $ENV{'KW_HOST'};  # Use this if you have already setup env vars
#my $KWHOST = "hasgtfsbld01.ger.corp.intel.com"; This line was commented out because the parameter was exposed to the user to input

#
# KWPORT       The port number on which the 9.x Klocwork Server
#              has been configured to listen/serve.
#my $KWPORT = $ENV{'KW_PORT'};  # Use this if you have already setup env vars
#my $KWPORT = "8072";

#
# KWLICHOST    The hostname of the FlexLM server.
#              Note: For balancing, if your Intel employee number ends in an even number use
#              the JF server, if odd, use the DP server.
#              
#my $KWLICHOST = $ENV{'KW_LIC_HOST'}; # Use this if you have already setup env vars
#my $KWLICHOST = "jf4slic002.amr.corp.intel.com";  # EC KW FlexLM license server
my $KWLICHOST = "klocwork03p.elic.intel.com";  # EC KW FlexLM license server
                                
#
# KWLICPORT    The port number on which the FlexLM license manager has
#              been configured to listen/serve.
#my $KWLICPORT = $ENV{'KW_LIC_PORT'}; # Use this if you have already setup env vars
my $KWLICPORT = "7500";

#
# KWBIN        Path to the Klocwork \bin directory (only needed if you have not setup PATH)
#
# my $KWBIN    = $ENV{'KW_BIN_PATH'};
#my $KWBIN    = "\"C:/Program Files/Klocwork/Klocwork 8.0 Server/bin/"";
#my $KWBIN    = "/usr/klocwork/v8_SR1/bin";  # $KWBIN is not needed if you have already mapped /bin in your PATH



# User input
# ----------
my $KWHOST			=		"";
my $KWPORT = "";
my $PROJ_NAME		=		"";
my $BUILD_NAME		=		"";
my $COMPILER_OPTS	=		"";         # empty by default. Used for the --add-compiler-options
my $BUILDSPECVAR	= 		"";         # empty by default. Used for -V arg for build template
my $SPEC_FILE 		=		"";
my $DEBUG			=		0;			# OFF by default
my $MAXJOBS			=		0;			# OFF by default. Means that --jobs-num auto will be used
my $BUILDTIME		=		0;			# OFF by default
my $KWTABLES		=		"";			# No Permanent tables dir assumed by default
my $TMP_TABLES_DIR	=		1;			# ON by default. If -tables is specified it is turned off

#my $TMP = "D:/klocwork";
my $TMP = $ENV{'TMP'};  # Need to have an ENV VAR defined for tmp dir if not using permanent tables dir
#$TMP="/tmp";

#
# main
# ====
#

#
# Init Perl's random number generater
#
srand;

#
# Process command line args
# =========================
#
$rc = processArgs();

#
# If there were any problems detected processing the user provided
# input, report an error condition and exit
#
if ($rc != 0)
{
  print "ERROR:\n";
  print "\tMissing or invalid parameter(s) detected.\n";
  print "\tPlease check your command syntax and options.\n";
  usage();
  exit(1);
}

#
# Make sure that the user has either TMP or TEMP Environment Var's set
#

if ($TMP eq "")
{
	$TMP = "/tmp";
}

if ($TMP eq "")
{
  print "ERROR:\n";
  print "\tTMP or TEMP environment variable must be set for\n";
  print "\tplacement of the Klocwork Tables during the build\n";
  print "\tPlease set one of these variables before running again\n";
  exit(1);
}


#
# Set tables dir. This is the place where a new, build specific, 
# directory will be created and the output from the linker written
# to. This is output is what is referred to as "tables data" or
# "tables dat files" or "Klocwork .dat files".
#
if ($KWTABLES eq "")
{
	$KWTABLES = $TMP."/kw_tables";
}

#
# If no host name was provided at all we CANNOT proceed. Report an error
# condition and exit
#
if ($KWHOST eq "")
{
  print "ERROR:\n";
  print "\tHost must be specified on the command line\n";
  usage();
  exit(1);
}

#
# KWADMIN      The kwadmin command [If your KW server is SSL enabled. Add the'--ssl' arg before ending quote]
#
#my $KWADMIN  = $KWBIN."\\kwadmin --host $KWHOST --port $KWPORT -ssl";
my $KWADMIN  = "kwadmin --url https://klocwork-inn6.devtools.intel.com:8105"; # Use this if you KW\bin in your PATH

#
# KWBP         The kwbuildproject command [If your KW server is SSL enabled. Add the'--ssl' arg before ending quote]
#
#my $KWBP     = $KWBIN."\\kwbuildproject --license-host $KWLICHOST --license-port $KWLICPORT --host $KWHOST --port $KWPORT";
my $KWBP     = "kwbuildproject --license-host $KWLICHOST --license-port $KWLICPORT --url https://klocwork-inn6.devtools.intel.com:8105";  #Use this if you have KW\bin in your PATH


#
# If a project name was NOT specified on command line, check env var
#
if ($PROJ_NAME eq "")
{
	$PROJ_NAME = $ENV{'BT_PROJECT'};
}

print "\n\n";  # Insert a couple of lines after the initial command line

#
# If no project name was provided at all we CANNOT proceed. Report an error
# condition and exit
#
if ($PROJ_NAME eq "")
{
  print "ERROR:\n";
  print "\tProject must be specified either on the command line\n";
  print "\tor via env var BT_PROJECT.\n";
  usage();
  exit(1);
}

print "Project specified is: [$PROJ_NAME]\n";

#
# If a build name was NOT specified on command line, check env var
#
if ($BUILD_NAME eq "")
{
	$BUILD_NAME = $ENV{'BT_BUILDNUMBER'};
}


#
# If a build-timestamp was requested tack that on
# Thus, if the buildname was not supplied, the timestamp becomes the buildname
#
if ($BUILDTIME)
{
	($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
	my $ts = sprintf("%04d%02d%02d_%02d_%02d",$year+1900,$mon+1,$mday,$hour,$min);
	$BUILD_NAME = $BUILD_NAME."_".$ts;
} # END - find build name

#
# If no buildname was provided, nor -ts option, then we CANNOT proceed. 
# Report an error condition and exit
#
if ($BUILD_NAME eq "")
{
  print "ERROR:\n";
  print "\tBuildname must be specified either on the command line\n";
  print "\tor via env var BT_BUILDNUMBER or with the timestamp option.\n";
  usage();
  exit(1);
}


#
# If a buildspec file was NOT specified on command line, check env var
#
if ($SPEC_FILE eq "")
{
	$SPEC_FILE = $ENV{'BT_BUILDSPECFILE'};
}

#
# If no buildspec file was provided we CANNOT proceed. Report an error
# condition and exit, unless a directory was provided
#
if ($SPEC_FILE eq "")
{
    print "ERROR:\n";
    print "\tBuildspec file must be specified either on the command line\n";
    print "\tor via env var BT_BUILDSPECFILE.\n";
    usage();
    exit(1);
}


print "Starting Klocwork build...\n";
print "==========================\n";

timeStamp(1);

#
# Validate that a buildspec was specified
#
if ($SPEC_FILE eq "")
{
	  print "----------------------------------------------------------------\n";
	  print "ERROR:\n";
	  print "Buildspec not specified.\n";
	  print "----------------------------------------------------------------\n";
	  exit(1);
}

#
# Everything appears to be good to go...
# 
print "Host is					[$KWHOST]\n";
print "Project is               [$PROJ_NAME]\n";
print "Build name is            [$BUILD_NAME]\n";
print "Buildspec file is        [$SPEC_FILE]\n";

if ($MAXJOBS == 0)	
	{print "CPUs used                [Auto]\n";}
else				
	{print "CPUs used                [$MAXJOBS]\n";;}

if ($TMP_TABLES_DIR == 1)	
	{print "Tables Dir is            [Temporary]\n";}
else				
	{print "Tables Dir is            [$KWTABLES]\n";}

if ($DEBUG == 1)	
	{print "Debug is                 [ON]\n";}
else				
	{print "Debug is                 [OFF]\n";}
print "\n";

#
# Query the KW server to see if the project exists. If the project
# specified does NOT exist, we will create it.
#
print "Checking to see if project [$PROJ_NAME] exists...\n";
my $projects = `$KWADMIN list-projects`;
print "$KWADMIN list-projects\n";
print "[$projects]\n\n\n";
if ($projects =~ m/^$PROJ_NAME\b/m)
{
  print "Found project [$PROJ_NAME]\n\n";
}
else
{
  print "Project [$PROJ_NAME] does not exist. Suggest that you use KMC to create one.\n";
  exit(1);
}


#
# Query the KW server to see if the buildname is available for use.
# If the buildname already exists for the specified project, we CANNOT
# proceed, report an error condition and exit
#
print "Checking to see if build_name [$BUILD_NAME] exists...\n";
my $builds = `$KWADMIN list-builds "$PROJ_NAME"`;
if ($builds =~ m/^$BUILD_NAME$/m)
{
  print "----------------------------------------------------------------\n";
  print "ERROR:\n";
  print "Build name [$BUILD_NAME] is already in use\n";
  print "----------------------------------------------------------------\n";
  if($DEBUG == 0) {
    exit(1);
  } else {
    print "======== DEBUG MODE ========\n\n";
  }
}
else
{
  print "Build name [$BUILD_NAME] is available for use\n\n";
}


#
# Everything seems to be in place, let's start the build process
# ==============================================================
#

#
# If caller did not supply a permanent tables dir then
# set our tmp tables dir. Create a unique directory name, using build name
# and a random generated number if starting from scratch
#
#
my $rand = rand();
$rand =~ s/0\.//s;
if ($TMP_TABLES_DIR == 1)
{
  $KWTABLES = $KWTABLES."/".$BUILD_NAME."_".$rand;
}


#
# =======================================
# Stage 1 - Build (Invoke kwbuildproject)
# =======================================
#

print "Starting Stage 1 (Compile/Linking) of Klocwork build...\n";
print "=======================================================\n";

if ($TMP_TABLES_DIR == 1)
{
	# Make the directory for the tables data
	$cmd = "mkdir -p \"$KWTABLES\"";
	print "[$cmd]\n\n";
	if($DEBUG == 0) { $exit_code = system($cmd); }else{print "======== DEBUG MODE ========\n\n";}
        exit(1) if(($exit_code >> 8) != 0);
}

# Create the kwbuildproject command
$cmd = "$KWBP";
$cmd = $cmd."/".$PROJ_NAME." --tables-directory \"$KWTABLES\"";


if($MAXJOBS != 0) 
{
  $cmd = $cmd." --jobs-num $MAXJOBS";
}

# If using a permanent tables dir, use --incremental (else default to --force)
if ($TMP_TABLES_DIR == 0)
{
  $cmd = $cmd." --incremental";

}
else
{
  $cmd = $cmd." --force";
}

# Extra compiler options for kwcc
if ($COMPILER_OPTS && $COMPILER_OPTS ne "") {
    my @compiler_opts = split(",", $COMPILER_OPTS);
    $cmd = $cmd." -add-compiler-options \"";
    foreach (@compiler_opts) {
        $cmd = $cmd." $_";
    }
    $cmd = $cmd."\"";
}

# Denote if we are using a build template (.tpl file)
if ($BUILDSPECVAR && $BUILDSPECVAR ne "") {
    my @bsv = split(",", $BUILDSPECVAR);
    $cmd = $cmd." -V \"" . join("\" -V \"", @bsv) . "\"";
}

# Add buildspec file(s) at the end of the command
$cmd = $cmd." $SPEC_FILE";

print "kwbuildproject command is: \n";
print "[$cmd]\n\n\n";

timeStamp(1);
if($DEBUG == 0) { $exit_code = system($cmd); }else{print "======== DEBUG MODE ========\n\n";}
timeStamp(0);
#print ";;New;; ERROR: Invalid exit of kwbuildproject!\n" if(($exit_code >> 8) != 0);


#
# =========================================================
# Stage 2 - Uploading  (Invoking kwadmin load command)
# ========================================================
#

print "Starting Stage 2 (Data Upload) of Klocwork build...\n";
print "=====================================================\n";
$cmd = "$KWADMIN";
$cmd = $cmd." load \"$PROJ_NAME\" \"$KWTABLES\" --name $BUILD_NAME";
print "kwadmin command is: \n";
print "[$cmd]\n\n\n";

timeStamp(1);
if($DEBUG == 0) { $exit_code = system($cmd); }else{print "======== DEBUG MODE ========\n\n";}
timeStamp(0);
exit(1) if(($exit_code >> 8) != 0);


#
#==================================
# Cleanup temp data if needed
#==================================
#
if ($TMP_TABLES_DIR == 1)
{
	print "===========================================\n";
	print "Cleaning up temporary files and data ... \n\n";
	print "Removing: $KWTABLES\n";
	print "===========================================\n";

	if($DEBUG == 0) {rmtree("$KWTABLES",0,1);}else{print "======= DEBUG MODE =======\n\n";}

	if($DEBUG == 0) {unlink();}else{print "======= DEBUG MODE =======\n\n";}

	print "\n\n";
}

print "Klocwork upload and report generation completed\n";
print "=======================================================\n";

timeStamp(0);

exit(1) if(($exit_code >> 8) != 0);


############################################################################
# processArgs
############################################################################
sub processArgs
{
	# No commmand line args are mandatory, so set to zero right away
  my $retVal = 0;

  for ($i = 0;$i <= $#ARGV;$i++)
  {
    if (($ARGV[$i] eq "-h") || ($ARGV[$i] eq "-help"))
    {
      usage();
      exit(0);
    }
    if (($ARGV[$i] eq "-v") || ($ARGV[$i] eq "-version"))
    {
	    print "kwbuild.pl Version [$VERSION].\n\n";
      exit(0);
    }
    elsif ($ARGV[$i] eq "-kwhost")
    {
      $KWHOST = $ARGV[$i+1];
      $i++;
      if (($KWHOST eq "") || ($KWHOST =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif ($ARGV[$i] eq "-kwport")
    {
      $KWPORT = $ARGV[$i+1];
      $i++;
      if (($KWPORT eq "") || ($KWPORT =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-p") || ($ARGV[$i] eq "-project"))
    {
      $PROJ_NAME = $ARGV[$i+1];
      $i++;
      if (($PROJ_NAME eq "") || ($PROJ_NAME =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-bs") || ($ARGV[$i] eq "-buildspec"))
    {
      if ($SPEC_FILE ne "")
      {
		$SPEC_FILE = join " ",$SPEC_FILE, $ARGV[$i+1];
		$i++;
      }
      else
      {
		$SPEC_FILE = $ARGV[$i+1];
		$i++;
	  }
		
      if (($SPEC_FILE eq "") || ($SPEC_FILE =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-b") || ($ARGV[$i] eq "-buildname"))
    {
      $BUILD_NAME = $ARGV[$i+1];
      $i++;
      if (($BUILD_NAME eq "") || ($BUILD_NAME =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-o") || ($ARGV[$i] eq "-opts"))
    {
      $COMPILER_OPTS = $ARGV[$i+1];
      $i++;
      if (($COMPILER_OPTS eq "") || ($COMPILER_OPTS =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-V") || ($ARGV[$i] eq "-bsv"))
    {
      $BUILDSPECVAR = $ARGV[$i+1];
      $i++;
      if (($BUILDSPECVAR eq ""))
      {
        $retVal++;
      }
    }
    
    elsif (($ARGV[$i] eq "-t") || ($ARGV[$i] eq "-tables"))
    {
      $KWTABLES = $ARGV[$i+1];
      $TMP_TABLES_DIR = 0;
      $i++;
      if (($KWTABLES eq "") || ($KWTABLES =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-r") || ($ARGV[$i] eq "-restart"))
    {
      # $RESTART = $ARGV[$i+1];
      $RESTART = 0;  # Force it to full build mode
      
      $i++;
      if (($RESTART eq "") || ($RESTART =~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-mj") || ($ARGV[$i] eq "-maxjobs"))
    {
      $MAXJOBS = $ARGV[$i+1];
      $i++;
      if (($MAXJOBS eq "") || ($MAXJOBS=~ m/^-/m))
      {
        $retVal++;
      }
    }
    elsif (($ARGV[$i] eq "-d") ||($ARGV[$i] eq "-debug") )
    {
      $DEBUG = 1;
    }
    elsif (($ARGV[$i] eq "-ts") ||($ARGV[$i] eq "-timestamp") )
    {
      $BUILDTIME = 1;
    }
    else
    {
      print "[$ARGV[$i]]   Unrecognized parameter!!!\n\n";
      return (-1);
    }
  }
  return ($retVal);
}


############################################################################
# timeStamp
############################################################################
sub timeStamp
{
  ($start) = @_;

  # Time and date stamp
  ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);

  if ($start == 1)
  {
    print "Started [";
  }
  else
  {
    print "Stopped [";
  }
  printf ("%02d-%02d-%04d  %02d:%02d:%02d",$mon+1,$mday,$year+1900,$hour,$min,$sec);
  print "]\n\n";

}


############################################################################
# usage
############################################################################
sub usage
{
  print "\n\n";
  print "kwbuild -kwhost <HostName> -project <PROJECT_NAME> -buildspec <BUILDSPEC> ";
  print "-buildname <BUILD_NAME> [OPTIONS]\n\n";
  print "Where:\n";
  print "\n";
  print "Required parameters: (unless provided in ENV vars)\n";
  print "  -kwhost <HostName>             - KW host to use\n";
  print "  -kwport <Port>                 - KW port to use\n";
  print "  -p or -project <PROJECT_NAME>  - KW project to use\n";
  print "  -bs or -buildspec <BUILDSPEC>  - buildspec to use. Can use multiple times\n";
  print "  -b or -buildname <BUILD_NAME>  - name for this build\n";
  print "\n";
  print "Optional parameters:\n";
  print "  -d or -debug                   - run through the steps, do NOT execute\n";
  print "  -mj or -maxjobs                - perform concurrent jobs per # of CPU's\n";
  print "       If not specified it defaults to Auto CPU Detection\n";
  print "  -h or -help                    - print this info\n";
  print "  -t or -tables <PATH>           - reuse an existing tables dir\n";
  print "     default: \%TMP\%/KW_TABLES/<BUILD_NAME>_<random> \n";
  print "  -ts or -timestamp              - timestamp on buildname\n";
  print "  -v or -version                 - print version info\n";
  print "  -o or -opts <quoted string of comma separated compiler options>\n";
  print "       e.g., -opts \"-I some_path, -D some_env_var\"\n";
  print "  -V or -bsv                     - VAR=PATH to use when using a build template\n";
  print "\n\nExamples:\n\n";
  print "kwbuild -project <PROJECT_NAME> -buildspec <BUILDSPEC> ";
  print "-buildname <BUILD_NAME> -tables <PATH>\n";
  print "\n\n";  
  print "kwbuild -p <PROJECT_NAME> -bs <BUILDSPEC> -b <BUILD_NAME> ";
  print "-t <PATH> -mj 2 -o \"-I C:\\unique\\sysheader -Dmy_env_var=1\" -mj 4\n";
  print "\n\n";  
  print "kwbuild -p <PROJECT_NAME> -bs <BUILDSPEC> -b <BUILD_NAME> ";
  print "-t <PATH> -mj 2 -bsv SRC_ROOT=c:\\my\\local\\tree\n";
  print "\n\n";
  print "kwbuild -p <PROJECT_NAME> -bs <BUILDSPEC-A> -bs <BUILDSPEC-B> -b <BUILD_NAME> ";
  print "-tables <PATH>\n"; 
  print "\n\nNotes:\n";
  print " The --incremental arg is ON by default. To force an occassional full build\n";
  print " delete the .o files in the tables dir then run a scan.\n";
  print " If no existing tables specified, temp dir created and later removed.\n\n";
}

