#!/usr/bin/python3

import os
import argparse
import time
import subprocess
import tempfile

def create_argparser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--BuildDir", required=True)
    parser.add_argument("--CmakeListDir", required=True)
    parser.add_argument("--KWhost", required=True)
    parser.add_argument("--KWport", required=True)
    parser.add_argument("--KWName", required=True)
    parser.add_argument("--CmakeArg", required=True)
    return parser
    
def upload_error(code):
    if code != 0:
        print("kwadmin upload faild !")
    else:
        print("kwadmin upload finished  successfully")
        
def main():
    PIPE = subprocess.PIPE
    arguments = create_argparser().parse_args()
    TMP = tempfile.gettempdir()
    pl_script_path = os.path.dirname(os.path.realpath(__file__)) + "/kwbuild.pl"
    kw_log_path = "kwinject.out"
    bskw_log_path = arguments.BuildDir + "/" + kw_log_path
    kw_revision = time.strftime("0%d%m%H%M%S")
    print ("##teamcity[setParameter name='KWBuildName' value='%s']" % (kw_revision,))
    kw_inject = 'cd "{0}" && kwinject --output "{1}" cmake {3} "{2}" && kwinject --output "{1}" cmake --build .'
    kw_inject = kw_inject.format(arguments.BuildDir,
                                kw_log_path,
                                arguments.CmakeListDir,
                                arguments.CmakeArg)
    print("kwinject command: ", kw_inject)
    proc = subprocess.Popen(kw_inject, shell=True)
    output = proc.communicate()
    print("kwinject command result: ", output) 
    kw_args = 'perl "{0}" -kwhost "{1}" -kwport "{2}" -project "{3}" -bs {4} -b {5} 2>&1'.format(pl_script_path,
                            arguments.KWhost,
                            arguments.KWport,
                            arguments.KWName,
                            bskw_log_path,
                            kw_revision)
    print("kwbuild command: ", kw_args)
    proc = subprocess.Popen(kw_args, shell = True)
    output = proc.communicate()
    errcode = proc.returncode
    print("kwbuild command result: ", output)
    upload_error(errcode)
    
main()  
