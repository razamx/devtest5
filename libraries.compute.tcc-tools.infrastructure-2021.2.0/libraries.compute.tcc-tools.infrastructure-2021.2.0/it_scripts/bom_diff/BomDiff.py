import os
import sys
import logging
import argparse
import re
import shutil
import zipfile
import subprocess
import config


def cleanUpDir(d):
    try:
        shutil.rmtree(d)
    except Exception as ex:
        pass
    os.makedirs(d)


def cleanUpDirs(dirList):
    for d in dirList:
        cleanUpDir(d)

def CustomCopy(from_dir, to_dir, pattern='*.*', copy_args='/Z /R:2 /W:15 /NP /NC /NFL /NDL'):
    print("CustomCopy start")
    copyFormatString = r'robocopy {ARGS} {FROM} {TO} {PATTERN}'
    copyPackageCmd = copyFormatString.format(ARGS=copy_args, FROM=from_dir, TO=to_dir, PATTERN=pattern)
    retcode = runExternalProcess(copyPackageCmd)
    if retcode > 7:
        raise OSError("Cannot copy '" + pattern + "' from '" + from_dir + "' to '" + to_dir + "'")
    print("CustomCopy finish. Return " + str(retcode))
    return retcode

def runExternalProcess(callstr, interactivePipes=False, returnAlsoOutput=False, suppressWarnings=False, shell_ex=True):
    print("runExternalProcess start. Function args: interPipes: " + str(interactivePipes) + ", returnAlsoSTDOUT: " + str(returnAlsoOutput) + ", callstr: " + callstr)
    out = None
    err = None

    try:
        if interactivePipes:
            retcode = subprocess.call(callstr)
        else:
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            proc = subprocess.Popen(callstr, shell=shell_ex, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = proc.communicate() # DO not use proc.wait This will deadlock when using stdout=PIPE and/or stderr=PIPE
                                          #and the child process generates enough output to a pipe such that it blocks waiting
                                          #for the OS pipe buffer to accept more data.
            proc.poll()
            print("Return code: " + str(proc.returncode))
            outTypeToValue = {"Standard": out, "Error": err}
            for t, output in outTypeToValue.items():
                if output is not None:
                    # get rid of empty lines in output.
                    if isinstance(output, str):
                        output = '\n'.join([s for s in output.split('\n') if s.strip(' \t\n\r') != ""])
                    else:
                        output = b'\n'.join([s for s in output.split(b'\n') if s.strip(b' \t\n\r') != b""])
                    if output:
                        print("{TYPE} output: {OUT}".format(TYPE=t, OUT=output))
            retcode = proc.returncode
    except OSError as err:
        raise OSError("Failed to call the process. Error message: " + str(err))

    if retcode == 143 and not suppressWarnings:
        print("Process killed by TIMEOUT!!!")
        print("runExternalProcess finish for cmd: " + callstr + ". Process exited with " + str(retcode))
    if returnAlsoOutput:
        return retcode, str(out) + "\n" + str(err)
    else:
        return retcode

class BoomDiff:
    def __init__(self, logger):
        self.logger = logger
        self.args = BoomDiff.getParser().parse_args(sys.argv[1:])
        self.curdir = os.path.abspath(os.path.dirname(sys.argv[0])) + "\\"

    @staticmethod
    def getParser():
        parser = argparse.ArgumentParser()
        parser.add_argument('--old_packages', default="", help="Path to folder with old LT and LMT zip packages.")
        parser.add_argument('--new_packages', default="", help="Path to folder with new LT and LMT zip packages.")
        parser.add_argument('--log_dir', default="")
        parser.add_argument('--tmp_dir', default=os.getenv('TEMP'))
        parser.add_argument('--wait',    default=False, action='store_const', const=True)
        parser.add_argument('--download', action='store_const', const=True)
        parser.add_argument('--clean', action='store_const', const=True)
        parser.add_argument('--report_name', default="HtmlReport", help="Name of output archive with results")
        return parser

    def callComparation(self, old_folder, new_folder, version_name):
        beyond_exe = "\"c:\Program Files (x86)\Beyond Compare 3\BCompare.exe\" "

        for (suffix, bcConfig, _) in config.report_suffixes:
            cmd = "{BEYOND_EXE} /closescript @{CURDIR}{BC_CONFIG} {OLD_FOLDER} {NEW_FOLDER} {VERSION_NAME}{SUFFIX}".format(
                BEYOND_EXE=beyond_exe,
                CURDIR=self.curdir,
                BC_CONFIG=bcConfig,
                OLD_FOLDER=old_folder,
                NEW_FOLDER=new_folder,
                VERSION_NAME=version_name,
                SUFFIX=suffix,
            )
            proc = runExternalProcess(cmd, 0, False, False, False)
            if (proc != 0): raise Exception("Beyond compare error: '"+ str(proc)+"' ")

    def replaseVersion(self, path, version):
        temp = os.walk(path, topdown=False)
        for root, dirs, files in temp:
            for i in dirs:
                dir = os.path.join(root,i)
                rez = re.findall(r'\\v' + version + r'*$', dir)
                if ( rez ):
                    new_dir = dir.replace(rez[0], "\\vY")
                    self.logger.warning("Rename: " + dir + " to " + new_dir)
                    os.rename(dir, new_dir)

    def unzip(self, path_to_archive, unzip_to):
        zip_ref = zipfile.ZipFile(path_to_archive, 'r')
        zip_ref.extractall(unzip_to)
        zip_ref.close()

    def compareZipPackages(self, old_pack, new_pack, package_type):
        unpacked_old_path = os.path.join(self.args.tmp_dir, "unpacked_new")
        unpacked_new_path = os.path.join(self.args.tmp_dir, "unpacked_old")

        cleanUpDirs((unpacked_old_path,
                     unpacked_new_path))

        self.unzip(old_pack, unpacked_old_path)
        self.unzip(new_pack, unpacked_new_path)

        # Handle packages that include extra root folder after unpacking (align old and new to the same root level)
        def normalizeUnpackedStructure(path):
            #dirs = os.listdir(path)
            #if "bin" not in dirs and :
            #    path = os.path.join(path, dirs[0])
            return path
        unpacked_new_path = normalizeUnpackedStructure(unpacked_new_path)
        unpacked_old_path = normalizeUnpackedStructure(unpacked_old_path)

        # Call comparing
        self.callComparation(unpacked_old_path, unpacked_new_path, package_type)


    def main(self):
        os.chdir(self.curdir)
        path_to_old_packs = self.args.old_packages
        path_to_new_packs = self.args.new_packages
        print("NEW PACKS: " + path_to_new_packs)
        print("OLD PACKS: " + path_to_old_packs)
        comparedPackages = []

        def getFilesRecursively(path):
            files = []
            for dirpath, dirnames, filenames in os.walk(path):
                fullFilesNames = [os.path.join(dirpath, filename) for filename in filenames if filename.endswith(".zip")]
                files.extend(fullFilesNames)
            return files

        def get_packs(path):
            packs = dict()
            files = getFilesRecursively(path)
            for file_name in files:
                for package_type, params in config.supported_package_types.items():
                    if re.match(params, file_name):
                        packs[package_type] = dict(supported_package_types=params)
                        packs[package_type]['package_path'] = os.path.join(path, file_name)
            return packs

        new_packs = get_packs(path_to_new_packs)
        old_packs = get_packs(path_to_old_packs)

        for package_type, params in new_packs.items():
            if package_type not in old_packs.keys():
                print("WARNING: there is no old " + package_type + " package type available. Skipping comparison.")
                continue
            #
            comparedPackages.append(package_type)
            self.compareZipPackages(old_packs[package_type]['package_path'], new_packs[package_type]['package_path'], package_type)

        # Automatic generation HTML files instead BomCheck folder
        config.createMainHtmlFiles(mainView="Package_contents", comparedPackages=comparedPackages)
        config.createHtmlForPackageType("Package_contents")
        self.callComparation(path_to_old_packs, path_to_new_packs, "Package_contents")
        for packegeType in comparedPackages:
            config.createHtmlForPackageType(packegeType)

        #Clean Direcoriy before copying
        html_report_path = os.path.join(self.args.log_dir, self.args.report_name)
        cleanUpDir(html_report_path)

        #Copy to output dir
        CustomCopy(self.curdir, html_report_path, "*.html")
        CustomCopy(os.path.join(self.curdir, "BcImages"), os.path.join(html_report_path, "BcImages"), "*.*")

        #Create ZIP archive
        try:
            shutil.make_archive(html_report_path, "zip", html_report_path)
        except Exception as err:
            self.logger.warning("Failed to create archive: " + html_report_path + ".zip. Error message: " + str(err))

        if self.args.download and self.args.clean:
            os.remove(self.args.new_packages)
            os.remove(self.args.old_packages)

        self.logger.info("BoomDiff::main() was succesfully complete")

if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG) #filename="BoomDiff.log", filemode="w",
    base_logger = logging.getLogger()

    BoomDiff(base_logger).main()
